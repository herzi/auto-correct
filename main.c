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

int
main (int   argc,
      char**argv)
{
        GtkWidget* box;
        GtkWidget* entry;
        GtkWidget* scrolled;
        GtkWidget* view;
        GtkWidget* window;

        gtk_init (&argc, &argv);

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title (GTK_WINDOW (window), _("Auto-correction demo..."));
        gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);
        g_signal_connect (window, "destroy",
                          G_CALLBACK (gtk_main_quit), NULL);

        box = gtk_vbox_new (FALSE, 0);

        entry = gtk_entry_new ();
        gtk_widget_show (entry);

        gtk_box_pack_start (GTK_BOX (box), entry,
                            FALSE, FALSE, 0);

        scrolled = gtk_scrolled_window_new (NULL, NULL);

        view = gtk_text_view_new ();

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

