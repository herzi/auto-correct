/* This file is part of auto-correct
 *
 * AUTHORS
 *     Sven Herzberg  <sven@imendio.com>
 *
 * Copyright (C) 2008  Sven Herzberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <libxml/xmlversion.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "ac-auto-correction.h"

#include <glib/gi18n.h>

#define AUTO_CORRECT_NOT_AVAILABLE _("Auto-correction doesn't work here, yet")
#define AUTO_CORRECT_AVAILABLE _("Auto-correction works here")

static gboolean text_inserted = FALSE;
static gboolean auto_complete = FALSE;

static GtkWidget* dialog_entry_before = NULL;
static GtkWidget* dialog_entry_after  = NULL;
static GtkWidget* dialog_button_add   = NULL;

static AcAutoCorrection* languages[4] = { /* FIXME: symbolic names */
                NULL, NULL, NULL, NULL
};

static gboolean save_to_file (AcAutoCorrection* ac,
                              GError          **error);

static void
entry_cursor_position_changed (GtkEntry  * entry,
                               GParamSpec* pspec     G_GNUC_UNUSED,
                               gpointer    user_data)
{
        AcAutoCorrection* ac = AC_AUTO_CORRECTION (user_data);
        gint cursor = gtk_editable_get_position (GTK_EDITABLE (entry));

        if (text_inserted) {
                gchar const* text = gtk_entry_get_text (entry);
                gchar const* text_cursor = text;
                gsize        i;
                GList      * completion;

                /* avoid recursion */
                text_inserted = FALSE;

                for (i = 0; i < cursor; i++) {
                        text_cursor = g_utf8_next_char (text_cursor);
                }

                auto_complete = TRUE;

                for (completion = ac_auto_correction_get_corrections (ac); completion; completion = g_list_next (completion)) {
                        AutoCompletion auto_completion = *((AutoCompletion*)(completion->data));
                        if (text_cursor - text >= strlen (auto_completion.before)) {
                                gsize j;
                                for (j = 0; j < strlen (auto_completion.before); j++) {
                                        if (text_cursor[j-strlen (auto_completion.before)] != auto_completion.before[j]) {
                                                break;
                                        }
                                }
                                if ((auto_completion.flags & AUTO_COMPLETION_AFTER_WHITESPACE) != 0) {
                                        if (text_cursor - strlen (auto_completion.before) - text <= 0) {
                                        } else {
                                                gchar* maybe_whitespace = g_utf8_prev_char (&text_cursor[-strlen (auto_completion.before)]);
                                                gunichar maybe_whitespace_c = g_utf8_get_char (maybe_whitespace);

                                                if (!g_unichar_isspace (maybe_whitespace_c)) {
                                                        j = 0;
                                                }
                                        }
                                }
                                if (j >= strlen (auto_completion.before)) {
                                        GString* string = g_string_new (text);

                                        if (strlen (auto_completion.before) == strlen (auto_completion.after)) {
                                                g_string_overwrite (string,
                                                                    text_cursor - strlen (auto_completion.before) - text,
                                                                    auto_completion.after);
                                        } else if (strlen (auto_completion.before) > strlen (auto_completion.after)) {
                                                g_string_overwrite (string,
                                                                    text_cursor - strlen (auto_completion.before) - text,
                                                                    auto_completion.after);
                                                g_string_erase (string,
                                                                text_cursor - strlen (auto_completion.before) + strlen (auto_completion.after) - text,
                                                                strlen (auto_completion.after) - strlen (auto_completion.before));
                                        } else {
                                                /* strlen (auto_completion.before) < strlen (auto_completion.after) */
                                                g_string_overwrite_len (string,
                                                                        text_cursor - strlen (auto_completion.before) - text,
                                                                        auto_completion.after,
                                                                        strlen (auto_completion.before));
                                                g_string_insert (string,
                                                                 text_cursor - text,
                                                                 auto_completion.after + strlen (auto_completion.before));
                                        }

                                        gtk_entry_set_text (entry, string->str);

                                        g_string_free (string, TRUE);

                                        gtk_editable_set_position (GTK_EDITABLE (entry),
                                                                   cursor -
                                                                   g_utf8_strlen (auto_completion.before, -1) +
                                                                   g_utf8_strlen (auto_completion.after, -1));

                                        /* FIXME: add a mark to display some stuff later */
                                }
                        }
                }

                auto_complete = FALSE;
        }

        /* FIXME: check for mark and display/hide it */
}

static void
entry_text_inserted (GtkEntry  * entry,
                     GParamSpec* pspec     G_GNUC_UNUSED,
                     gpointer    user_data G_GNUC_UNUSED)
{
        if (!auto_complete) {
                /* FIXME: store on the GtkEntry */
                text_inserted = TRUE;
        }
}

static void
entry_text_deleted (GtkEntry  * entry,
                    GParamSpec* pspec     G_GNUC_UNUSED,
                    gpointer    user_data G_GNUC_UNUSED)
{
        /* FIXME: potentially check for mark and undo completion */
}

static void
render_before_column (GtkTreeViewColumn* column,
                      GtkCellRenderer  * renderer,
                      GtkTreeModel     * model,
                      GtkTreeIter      * iter,
                      gpointer           user_data)
{
        AutoCompletion* cmp = NULL;

        gtk_tree_model_get (model, iter,
                            0, &cmp, /* FIXME: symbolic names */
                            -1);

        g_object_set (renderer,
                      "text", cmp->before,
                      NULL);
}

static void
render_after_column (GtkTreeViewColumn* column,
                     GtkCellRenderer  * renderer,
                     GtkTreeModel     * model,
                     GtkTreeIter      * iter,
                     gpointer           user_data)
{
        AutoCompletion* cmp = NULL;

        gtk_tree_model_get (model, iter,
                            0, &cmp, /* FIXME: symbolic names */
                            -1);

        g_object_set (renderer,
                      "text", cmp->after,
                      NULL);
}

static void
render_after_whitespace_column (GtkTreeViewColumn* column,
                                GtkCellRenderer  * renderer,
                                GtkTreeModel     * model,
                                GtkTreeIter      * iter,
                                gpointer           user_data)
{
        AutoCompletion* cmp = NULL;

        gtk_tree_model_get (model, iter,
                            0, &cmp, /* FIXME: symbolic names */
                            -1);

        g_object_set (renderer,
                      "active", (cmp->flags & AUTO_COMPLETION_AFTER_WHITESPACE) != 0,
                      NULL);
}

static gchar*
ac_xml_escape (gchar const* string)
{
        GString* gstring;
        gchar const* end = NULL;
        gchar* result;

        if (!g_utf8_validate (string, -1, &end)) {
                result = g_strdup_printf ("invalid utf-8 byte at offset %d: 0x%x",
                                          end - string,
                                          *end);
                g_warning ("%s", result);
                return result;
        }

        gstring = g_string_new ("");
        for (; *string; string = g_utf8_next_char (string)) {
                gunichar c = g_utf8_get_char (string);

                if (c > 127) {
                        g_string_append_printf (gstring, "&#x%x;", c);
                        continue;
                }

                switch (c) {
                case '"':
                        g_string_append_printf (gstring, "&quot;");
                        break;
                case '\'':
                        g_string_append_printf (gstring, "&apos;");
                        break;
                case '<':
                        g_string_append_printf (gstring, "&lt;");
                        break;
                case '>':
                        g_string_append_printf (gstring, "&gt;");
                        break;
                case '&':
                        g_string_append_printf (gstring, "&amp;");
                        break;
                default:
                        g_string_append_unichar (gstring, c);
                        break;
                }
        }

        result = gstring->str;
        g_string_free (gstring, FALSE);
        return result;
}

static void
click_it (GtkButton* button)
{
        if (GTK_WIDGET_SENSITIVE (button)) {
                g_signal_emit_by_name (button, "activate");
        }
}

static void
update_button_sensitivity (void)
{
        gtk_widget_set_sensitive (dialog_button_add,
                                  gtk_entry_get_text_length (GTK_ENTRY (dialog_entry_before)) > 0 &&
                                  gtk_entry_get_text_length (GTK_ENTRY (dialog_entry_after)));
}

static void
update_remove_sensitivity (GtkTreeSelection* selection,
                           GtkWidget       * button)
{
        gtk_widget_set_sensitive (button,
                                  gtk_tree_selection_count_selected_rows (selection) > 0);
}

static void
add_button_clicked (GtkButton  * button,
                    GtkTreeView* treeview)
{
        AutoCompletion* cmp = g_slice_new0 (AutoCompletion);
        GtkListStore  * store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
        GtkTreePath   * path;
        GtkTreeIter     iter;

        cmp->before = g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog_entry_before)));
        cmp->after  = g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog_entry_after)));

        ac_auto_correction_prepend (AC_AUTO_CORRECTION (g_object_get_data (G_OBJECT (button), "ac")), cmp);

        gtk_list_store_insert_after (store, &iter, NULL);
        gtk_list_store_set          (store, &iter,
                                     0, cmp, /* FIXME: symbolic names */
                                     -1);

        path = gtk_tree_model_get_path (GTK_TREE_MODEL (store),
                                        &iter);

        gtk_tree_view_scroll_to_cell (treeview,
                                      path,
                                      NULL,
                                      FALSE,
                                      0.0, 0.0);

        gtk_tree_path_free (path);
}

static void
remove_button_clicked (GtkButton  * button,
                       GtkTreeView* treeview)
{
        AutoCompletion* cmp;
        GtkTreeModel  * model;
        GtkTreeIter     iter;

        g_return_if_fail (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (treeview), &model, &iter));

        gtk_tree_model_get (model, &iter,
                            0, &cmp, /* FIXME: symbolic names */
                            -1);

        gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

        ac_auto_correction_remove (AC_AUTO_CORRECTION (g_object_get_data (G_OBJECT (button), "ac")), cmp);

        g_slice_free (AutoCompletion, cmp);
}

static void
display_dialog (GtkAction* action,
                GtkWidget* window)
{
        GtkTreeViewColumn* column;
        AcAutoCorrection * ac;
        PangoAttribute   * attribute;
        PangoAttrList    * attributes;
        GtkListStore     * store;
        GtkWidget        * alignment;
        GtkWidget        * box;
        GtkWidget        * button_remove;
        GtkWidget        * frame;
        GtkWidget        * scrolled;
        GtkWidget        * table;
        GtkWidget        * tree;
        GtkWidget        * dialog = gtk_dialog_new_with_buttons (_("Preferences - Auto Correction"),
                                                                 GTK_WINDOW (window),
                                                                 GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
                                                                 GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT,
                                                                 NULL);
        GError           * error = NULL;
        GList            * iter;
        gint               columns;

        ac = g_object_get_data (G_OBJECT (window), "ac");

        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_ACCEPT);
        gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 300);

        /* FIXME: add insert and remove stuff */

        box = gtk_vbox_new (FALSE, 0);

        table = gtk_table_new (2, 3, FALSE);

        /* FIXME: connect entry sizes to the column sizes... */
        dialog_entry_before = gtk_entry_new ();
        dialog_entry_after = gtk_entry_new ();
        g_signal_connect_swapped (dialog_entry_before, "activate",
                                  G_CALLBACK (gtk_widget_grab_focus), dialog_entry_after);
        gtk_widget_show (dialog_entry_before);
        gtk_table_attach (GTK_TABLE (table), dialog_entry_before,
                          0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

        dialog_button_add = gtk_button_new_from_stock (GTK_STOCK_ADD);
        g_object_set_data (G_OBJECT (dialog_button_add),
                           "ac",
                           ac);
        g_signal_connect_swapped (dialog_entry_after, "activate",
                                  G_CALLBACK (click_it), dialog_button_add);
        gtk_widget_show (dialog_entry_after);
        gtk_table_attach (GTK_TABLE (table), dialog_entry_after,
                          1, 3, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
        gtk_widget_show (dialog_button_add);

        g_signal_connect (dialog_entry_before, "notify::text",
                          G_CALLBACK (update_button_sensitivity), NULL);
        g_signal_connect (dialog_entry_after,  "notify::text",
                          G_CALLBACK (update_button_sensitivity), NULL);
        update_button_sensitivity ();

        tree = gtk_tree_view_new ();
        g_signal_connect (dialog_button_add, "clicked",
                          G_CALLBACK (add_button_clicked), tree);

        alignment = gtk_alignment_new (1.0, 0.5, 0.0, 1.0);
        gtk_container_add (GTK_CONTAINER (alignment), dialog_button_add);
        gtk_widget_show (alignment);
        gtk_table_attach (GTK_TABLE (table), alignment,
                          1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

        button_remove = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
        g_object_set_data (G_OBJECT (button_remove),
                           "ac",
                           ac);
        g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree)), "changed",
                          G_CALLBACK (update_remove_sensitivity), button_remove);
        g_signal_connect (button_remove, "clicked",
                          G_CALLBACK (remove_button_clicked), tree);

        gtk_widget_show (button_remove);
        gtk_table_attach (GTK_TABLE (table), button_remove,
                          2, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
        gtk_widget_show (table);

        gtk_box_pack_start (GTK_BOX (box), table,
                            FALSE, FALSE, 0);

        /* FIXME: make editable; make draggable; ... */
        columns = gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree), -1,
                                                              _("Replace"), gtk_cell_renderer_text_new (),
                                                              render_before_column, NULL,
                                                              NULL);
        column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), columns - 1);
        gtk_tree_view_column_set_expand (column, TRUE);
        columns = gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree), -1,
                                                              _("With"), gtk_cell_renderer_text_new (),
                                                              render_after_column, NULL,
                                                              NULL);
        column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), columns - 1);
        gtk_tree_view_column_set_expand (column, TRUE);
        columns = gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree), -1,
                                                              _("Follows Whitespace"), gtk_cell_renderer_toggle_new (),
                                                              render_after_whitespace_column, NULL,
                                                              NULL);
        column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), columns - 1);
        gtk_tree_view_column_set_expand (column, FALSE);
        gtk_widget_show (tree);

        scrolled = gtk_scrolled_window_new (NULL, NULL);
        gtk_container_add (GTK_CONTAINER (scrolled),
                           tree);
        gtk_widget_show (scrolled);

        gtk_container_add (GTK_CONTAINER (box), scrolled);
        gtk_widget_show (box);

        frame = gtk_frame_new (_("Current Replacement List"));
        attributes = pango_attr_list_new ();
        attribute = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
        pango_attr_list_change (attributes,
                                attribute);
        gtk_label_set_attributes (GTK_LABEL (gtk_frame_get_label_widget (GTK_FRAME (frame))),
                                  attributes);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
        gtk_container_add (GTK_CONTAINER (frame),
                           box);
        gtk_widget_show (frame);

        gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                           frame);

        /* FIXME: add a label "To change the values, just double-click into a field" */

        store = gtk_list_store_new (1, G_TYPE_POINTER); /* FIXME: symbolic names */
        for (iter = ac_auto_correction_get_corrections (ac); iter; iter = g_list_next (iter)) {
                GtkTreeIter  tree_iter;

                gtk_list_store_append (store, &tree_iter);
                gtk_list_store_set    (store, &tree_iter,
                                       0, iter->data, /* FIXME: symbolic names */
                                       -1);
        }
        gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
        g_object_unref (store);

        gtk_dialog_run (GTK_DIALOG (dialog));

        gtk_widget_destroy (dialog);

        /* now save the stuff */
        save_to_file (ac, &error);
}

static void
fd_printf (int          fd,
           GError     **error,
           gchar const* format,
           ...)
{
        va_list  argv;
        ssize_t  written = 0;
        size_t   length;
        char     stack_buffer[512];
        char   * heap_buffer = NULL;

        g_return_if_fail (!error || !*error);
        g_return_if_fail (format);

        /* try a small buffer from the stack to not fragment the heap too much */
        va_start (argv, format);
        length = vsnprintf (stack_buffer, sizeof (stack_buffer), format, argv);
        va_end (argv);

        if (length < sizeof (stack_buffer) - 1) { /* length doesn't include \0, sizeof(buffer) does */
                va_start (argv, format);
                heap_buffer = g_strdup_vprintf (format, argv);
                va_end (argv);

                length = strlen (heap_buffer);
        }

        written = write (fd, heap_buffer ? heap_buffer : stack_buffer, length);

        if (written == -1 || written != length) {
                /* FIXME: set error */
                g_assert_not_reached ();
        }

        g_free (heap_buffer); /* already includes %NULL check */
}

static gboolean
save_to_file (AcAutoCorrection* ac,
              GError          **error)
{
                struct flock fl = {F_WRLCK, SEEK_SET, 0, 0, 0};
                int fd = -1;
                GList* iter;
                gchar* path;

                fl.l_pid = getpid ();

                path = g_build_filename (g_get_home_dir(),
                                         ".local",
                                         "share",
                                         "auto-correct.xml",
                                         NULL);
                fd = g_open (path, O_WRONLY | O_TRUNC | O_CREAT, 0644);

                if (fd == -1) {
                        int my_errno = errno;
                        GError* my_error = g_error_new (G_FILE_ERROR,
                                                        g_file_error_from_errno (my_errno),
                                                        _("Couldn't open file \"%s\" for writing"),
                                                        path);

                        g_free (path);

                        g_propagate_error (error, my_error);
                        return FALSE;
                }
                g_free (path);

                if (fcntl (fd, F_SETLK, &fl) == -1) {
                        perror ("fcntl");
                        exit (1); /* FIXME: recover nicely */
                }

                fd_printf (fd, error, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
                fd_printf (fd, error, "<!-- vim:set sw=2: -->\n");
                fd_printf (fd, error, "<auto-correction xmlns=\"http://www.adeal.eu/auto-correct/0.0.1\">\n");
                for (iter = ac_auto_correction_get_corrections (ac); iter; iter = g_list_next (iter)) {
                        AutoCompletion* cmp = iter->data;
                        gchar* before = ac_xml_escape (cmp->before);
                        gchar* after  = ac_xml_escape (cmp->after);
                        gchar const* flags = (cmp->flags & AUTO_COMPLETION_AFTER_WHITESPACE) != 0 ?
                                             " flags=\"after-whitespace\"" : "";

                        fd_printf (fd, error,
                                   "  <entry before=\"%s\" after=\"%s\"%s />\n",
                                   before, after, flags);

                        g_free (before);
                        g_free (after);
                }
                fd_printf (fd, error, "</auto-correction>\n");

                close (fd);

        return TRUE;
}

static void
language_selected (GtkAction* action,
                   GtkAction* current)
{
        gchar* label = NULL;
        g_object_get (current, "label", &label, NULL);
        g_print ("%s\n", label);
        g_free (label);
}

int
main (int   argc,
      char**argv)
{
        GtkRadioActionEntry  radio_entries[] = {
                {"None", NULL, N_("None"),
                 NULL, NULL, // FIXME: add tooltip
                 0}, /* FIXME: symbolic name */
                {"English", NULL, N_("English"),
                 NULL, NULL, // FIXME: add tooltip
                 1}, /* FIXME: symbolic name */
                {"German", NULL, N_("German"),
                 NULL, NULL, // FIXME: add tooltip
                 2}, /* FIXME: symbolic name */
                {"French", NULL, N_("French"),
                 NULL, NULL, // FIXME: add tooltip
                 3}  /* FIXME: symbolic name */
        };
        AcAutoCorrection   * ac;
        GtkActionEntry  entries[] = {
                {"File", NULL, N_("_Auto Correction")},
                {"Language", NULL, N_("_Language")},
                {"Preferences", GTK_STOCK_PREFERENCES, NULL,
                 "<Ctrl>P", NULL, // FIXME: add tooltip
                 G_CALLBACK (display_dialog)},
                {"Quit", GTK_STOCK_QUIT, NULL,
                 NULL, NULL, // FIXME: add tooltip
                 G_CALLBACK (gtk_main_quit)}
        };
        GtkActionGroup* actions;
        GtkUIManager  * manager;
        GtkTextIter     iter;
        GtkWidget    * box;
        GtkWidget* entry;
        GtkWidget* scrolled;
        GtkWidget* view;
        GtkWidget    * window;
        GString      * string;
        GError       * error = NULL;
        GList         * completion;

        gtk_init (&argc, &argv);
        LIBXML_TEST_VERSION;

        ac = ac_auto_correction_new_from_path ("auto-correct.xml", &error);
        languages[1] = ac_auto_correction_new (); /* FIXME: symbolic names */
        languages[2] = ac_auto_correction_new_from_path ("auto-correct-de.xml", &error); /* FIXME: symbolic names */
        languages[3] = ac_auto_correction_new_from_path ("auto-correct-fr.xml", &error); /* FIXME: symbolic names */

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        g_object_set_data (G_OBJECT (window),
                           "ac",
                           ac);

        actions = gtk_action_group_new ("main-window");
        gtk_action_group_add_actions (actions,
                                      entries,
                                      G_N_ELEMENTS (entries),
                                      window);
        gtk_action_group_add_radio_actions (actions,
                                            radio_entries,
                                            G_N_ELEMENTS (radio_entries),
                                            0, /* FIXME: symbolic name */
                                            G_CALLBACK (language_selected),
                                            NULL);

        manager = gtk_ui_manager_new ();
        gtk_ui_manager_insert_action_group (manager,
                                            actions,
                                            0);
        if (!gtk_ui_manager_add_ui_from_string (manager,
                                                "<menubar name='menus'><menu action='File'>"
                                                  "<menuitem action='Preferences' />"
                                                  "<separator />"
                                                  "<menuitem action='Quit' />"
                                                "</menu><menu action='Language'>"
                                                  "<menuitem action='None'/>"
                                                  "<separator />"
                                                  "<menuitem action='English'/>"
                                                  "<menuitem action='German'/>"
                                                  "<menuitem action='French'/>"
                                                "</menu></menubar>",
                                                -1,
                                                &error))
        {
                g_warning ("Error building main window: %s",
                           error->message);
                g_error_free (error);

                return 1;
        }

        gtk_window_set_title (GTK_WINDOW (window), _("Auto-correction demo..."));
        gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);
        gtk_window_add_accel_group  (GTK_WINDOW (window),
                                     gtk_ui_manager_get_accel_group (manager));
        g_signal_connect (window, "destroy",
                          G_CALLBACK (gtk_main_quit), NULL);

        box = gtk_vbox_new (FALSE, 0);

        gtk_widget_show (gtk_ui_manager_get_widget (manager, "/ui/menus"));
        gtk_box_pack_start (GTK_BOX (box), gtk_ui_manager_get_widget (manager, "/ui/menus"),
                            FALSE, FALSE, 0);

        entry = gtk_entry_new ();
        g_signal_connect (entry, "notify::cursor-position",
                          G_CALLBACK (entry_cursor_position_changed), ac);
        g_signal_connect_after (entry, "insert-text",
                                G_CALLBACK (entry_text_inserted), NULL);
        g_signal_connect_after (entry, "delete-text",
                                G_CALLBACK (entry_text_deleted), NULL);
        gtk_entry_set_text (GTK_ENTRY (entry), AUTO_CORRECT_AVAILABLE);
        gtk_widget_show (entry);

        gtk_box_pack_start (GTK_BOX (box), entry,
                            FALSE, FALSE, 0);

        scrolled = gtk_scrolled_window_new (NULL, NULL);

        view = gtk_text_view_new ();
        gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)),
                                  AUTO_CORRECT_NOT_AVAILABLE,
                                  -1);
        gtk_text_buffer_get_end_iter (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)),
                                      &iter);
        gtk_text_buffer_insert (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)),
                                &iter,
                                _("\nPotential replacements:\n"),
                                -1);

        string = g_string_new ("");
        for (completion = ac_auto_correction_get_corrections (ac); completion; completion = g_list_next (completion)) {
                AutoCompletion* auto_completion = completion->data;
                g_string_set_size (string, 0);

                g_string_append_printf (string,
                                        _("\n\"%s\" => \"%s\"%s"),
                                        auto_completion->before,
                                        auto_completion->after,
                                        auto_completion->flags == AUTO_COMPLETION_AFTER_WHITESPACE ? _("(after whitespace)") : "");

                gtk_text_buffer_insert (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)),
                                        &iter,
                                        string->str,
                                        -1);
        }
        g_string_free (string, TRUE);

        gtk_widget_show (view);
        gtk_container_add (GTK_CONTAINER (scrolled), view);

        gtk_widget_show (scrolled);
        gtk_container_add (GTK_CONTAINER (box), scrolled);

        gtk_widget_show (box);
        gtk_container_add (GTK_CONTAINER (window), box);

        gtk_widget_show (window);

        gtk_main ();

        g_object_unref (ac);
        return 0;
}

