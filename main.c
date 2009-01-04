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

#include <libxml/parser.h>
#include <libxml/xmlversion.h>
#include <gtk/gtk.h>

#include <glib/gi18n.h>

#define AUTO_CORRECT_NOT_AVAILABLE _("Auto-correction doesn't work here, yet")
#define AUTO_CORRECT_AVAILABLE _("Auto-correction works here")

static gboolean text_inserted = FALSE;
static gboolean auto_complete = FALSE;

typedef enum {
        AUTO_COMPLETION_NONE = 0,
        AUTO_COMPLETION_AFTER_WHITESPACE = 1
} AutoCompletionFlags;

typedef struct _AutoCompletion AutoCompletion;

struct _AutoCompletion {
        gchar const        * before;
        gchar const        * after;
        AutoCompletionFlags  flags;
};

static GList* completions = NULL;

static void
entry_cursor_position_changed (GtkEntry  * entry,
                               GParamSpec* pspec     G_GNUC_UNUSED,
                               gpointer    user_data G_GNUC_UNUSED)
{
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

                for (completion = completions; completion; completion = g_list_next (completion)) {
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
start_element_ns (gpointer      ctxt,
                  guchar const* local_name,
                  guchar const* prefix,
                  guchar const* uri,
                  int           n_namespaces,
                  guchar const**namespaces,
                  int           n_attributes,
                  int           n_defaulted,
                  guchar const**attributes)
{
        gint i;

        g_return_if_fail (g_strcmp0 ("http://www.adeal.eu/auto-correct/0.0.1", (gchar const*)uri) == 0);

        if (!strcmp ("entry", (gchar const*)local_name)) {
                AutoCompletion* cmp = g_slice_new0 (AutoCompletion);
                for (i = 0; i < n_attributes; i++) {
                        if (g_strcmp0 ("before", (gchar const*)attributes[5*i]) == 0) {
                                g_return_if_fail (!cmp->before);
                                cmp->before = g_strndup ((gchar*)attributes[5*i+3],
                                                         attributes[5*i+4] - attributes[5*i+3]);
                        } else if (g_strcmp0 ("after", (gchar const*)attributes[5*i]) == 0) {
                                g_return_if_fail (!cmp->after);
                                cmp->after = g_strndup ((gchar*)attributes[5*i+3],
                                                        attributes[5*i+4] - attributes[5*i+3]);
                        } else if (g_strcmp0 ("flags", (gchar const*)attributes[5*i]) == 0) {
                                gchar* value = g_strndup ((gchar*)attributes[5*i+3],
                                                          attributes[5*i+4] - attributes[5*i+3]);
                                gchar**values = g_strsplit (value, " ", 0);
                                gchar**iter;
                                for (iter = values; *iter; iter++) {
                                        if (g_strcmp0 ("after-whitespace", *iter) == 0) {
                                                cmp->flags |= AUTO_COMPLETION_AFTER_WHITESPACE;
                                        } else {
                                                g_assert_not_reached ();
                                        }
                                }
                                g_strfreev (values);
                                g_free (value);
                        } else {
                                g_assert_not_reached ();
                        }
                }

                g_return_if_fail (cmp->before != NULL);
                g_return_if_fail (cmp->after  != NULL);

                completions = g_list_prepend (completions, cmp);
        } else if (!strcmp ("auto-correction", (gchar const*)local_name)) {
#if 0
                for (i = 0; i < n_namespaces; i++) {
                        g_print ("\t%s:%s\n", namespaces[2*i], namespaces[2*i+1]);
                }
#endif
        } else {
                g_assert_not_reached ();
        }
}

static void
end_element_ns (gpointer      ctxt,
                  guchar const* local_name,
                  guchar const* prefix,
                  guchar const* uri)
{
        g_return_if_fail (g_strcmp0 ("http://www.adeal.eu/auto-correct/0.0.1", (gchar const*)uri) == 0);

        if (!g_strcmp0 ("entry", (gchar const*)local_name)) {
        } else if (!g_strcmp0 ("auto-correction", (gchar const*)local_name)) {
                completions = g_list_reverse (completions);
        }
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
display_dialog (GtkAction* action,
                GtkWidget* window)
{
        GtkListStore* store;
        GtkWidget   * scrolled;
        GtkWidget   * tree;
        GtkWidget   * dialog = gtk_dialog_new_with_buttons (_("Preferences - Auto Correction"),
                                                            GTK_WINDOW (window),
                                                            GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
                                                            GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT,
                                                            NULL);
        GList       * iter;

        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_ACCEPT);
        gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 300);

        tree = gtk_tree_view_new ();
        gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree), -1,
                                                    _("Before"), gtk_cell_renderer_text_new (),
                                                    render_before_column, NULL,
                                                    NULL);
        gtk_widget_show (tree);

        scrolled = gtk_scrolled_window_new (NULL, NULL);
        gtk_container_add (GTK_CONTAINER (scrolled),
                           tree);
        gtk_widget_show (scrolled);

        gtk_box_pack_start_defaults (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                                     scrolled);

        store = gtk_list_store_new (1, G_TYPE_POINTER); /* FIXME: symbolic names */
        for (iter = completions; iter; iter = g_list_next (iter)) {
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
}

int
main (int   argc,
      char**argv)
{
        GtkActionEntry  entries[] = {
                {"File", NULL, N_("_Auto Correction"),
                 NULL, NULL,
                 NULL},
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
        xmlSAXHandler   sax;

        gtk_init (&argc, &argv);
        LIBXML_TEST_VERSION;

        xmlSAXVersion (&sax, 2);
        sax.startElementNs = start_element_ns;
        sax.endElementNs   = end_element_ns;

        xmlSAXParseFileWithData (&sax, "auto-correct.xml", 0, NULL);

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

        actions = gtk_action_group_new ("main-window");
        gtk_action_group_add_actions (actions,
                                      entries,
                                      G_N_ELEMENTS (entries),
                                      window);

        manager = gtk_ui_manager_new ();
        gtk_ui_manager_insert_action_group (manager,
                                            actions,
                                            0);
        if (!gtk_ui_manager_add_ui_from_string (manager,
                                                "<menubar name='menus'><menu action='File'>"
                                                  "<menuitem action='Preferences' />"
                                                  "<separator />"
                                                  "<menuitem action='Quit' />"
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
                          G_CALLBACK (entry_cursor_position_changed), NULL);
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
        for (completion = completions; completion; completion = g_list_next (completion)) {
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
        return 0;
}

