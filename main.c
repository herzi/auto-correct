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

int
main (int   argc,
      char**argv)
{
        GtkTextIter  iter;
        GtkWidget* box;
        GtkWidget* entry;
        GtkWidget* scrolled;
        GtkWidget* view;
        GtkWidget* window;
        GString    * string;
        gsize        i;
        AutoCompletion auto_completion[] = {
                {"...", "…"},
                {"\"",  "»",  AUTO_COMPLETION_AFTER_WHITESPACE}, /* or this one: "„" */
                {"\"",  "«"},                                    /* or this one: "“" */
                {"-- ", "— ", AUTO_COMPLETION_AFTER_WHITESPACE},
                {"?? ", "⁇ "},
                {"?! ", "⁈ "},
                {"!? ", "⁉ "},
                {"!! ", "‼ "},
                {"'", "‘",    AUTO_COMPLETION_AFTER_WHITESPACE},
                {"'", "’"},
                {"°C", "℃"},
                {"°F", "℉"},
                {"c/o", "℅"},
                {"(c)", "©"},
                {"(R)", "®"},
                {"\n- ", "\n• "},
                {"->",  "→"},
                {":-)", "☺"}
        };

        gtk_init (&argc, &argv);

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title (GTK_WINDOW (window), _("Auto-correction demo..."));
        gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);
        g_signal_connect (window, "destroy",
                          G_CALLBACK (gtk_main_quit), NULL);

        box = gtk_vbox_new (FALSE, 0);

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
        for (i = 0; i < G_N_ELEMENTS (auto_completion); i++) {
                completions = g_list_prepend (completions, &auto_completion[i]);
                g_string_set_size (string, 0);

                g_string_append_printf (string,
                                        _("\n\"%s\" => \"%s\"%s"),
                                        auto_completion[i].before,
                                        auto_completion[i].after,
                                        auto_completion[i].flags == AUTO_COMPLETION_AFTER_WHITESPACE ? _("(after whitespace)") : "");

                gtk_text_buffer_insert (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)),
                                        &iter,
                                        string->str,
                                        -1);
        }
        completions = g_list_reverse (completions);
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

