/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Red Hat, Inc.
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *        Colin Walters <walters@verbum.org>
 *        Marco Trevisan <marco@ubuntu.com>
 */

#include <config.h>

#include <stdlib.h>

#include <glib/gi18n.h>

#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
#include <gtk/gtkx.h>
#endif

#include "gsm-fail-whale-dialog.h"

#include "gsm-icon-names.h"

struct _GsmFailWhaleDialog
{
        GtkWindow parent;

        gboolean debug_mode;
        gboolean allow_logout;
        gboolean extensions;
        GdkMonitor *monitor;
        GdkRectangle geometry;
};

G_DEFINE_TYPE (GsmFailWhaleDialog, gsm_fail_whale_dialog, GTK_TYPE_WINDOW);

/* derived from tomboy */
static void
_window_override_user_time (GsmFailWhaleDialog *window)
{
        guint32 ev_time = gtk_get_current_event_time ();
        GdkWindow *gdk_window = gtk_widget_get_window (GTK_WIDGET (window));

#ifdef GDK_WINDOWING_X11
        if (!GDK_IS_X11_WINDOW (gdk_window))
                return;

        if (ev_time == 0) {
                gint ev_mask = gtk_widget_get_events (GTK_WIDGET (window));
                if (!(ev_mask & GDK_PROPERTY_CHANGE_MASK)) {
                        gtk_widget_add_events (GTK_WIDGET (window),
                                               GDK_PROPERTY_CHANGE_MASK);
                }

                /*
                 * NOTE: Last resort for D-BUS or other non-interactive
                 *       openings.  Causes roundtrip to server.  Lame.
                 */
                ev_time = gdk_x11_get_server_time (gdk_window);
        }

        gdk_x11_window_set_user_time (gdk_window, ev_time);
#endif
}

static void
_window_move_resize_window (GsmFailWhaleDialog *window,
                            gboolean  move,
                            gboolean  resize)
{
        if (window->debug_mode)
                return;

        g_debug ("Move and/or resize window x=%d y=%d w=%d h=%d",
                 window->geometry.x,
                 window->geometry.y,
                 window->geometry.width,
                 window->geometry.height);

        if (resize) {
                gtk_window_resize (GTK_WINDOW (window),
                                   window->geometry.width,
                                   window->geometry.height);
        }

        if (move) {
                gtk_window_move (GTK_WINDOW (window),
                                 window->geometry.x,
                                 window->geometry.y);
        }
}

static void
update_geometry (GsmFailWhaleDialog *fail_dialog)
{
        gdk_monitor_get_geometry (fail_dialog->monitor, &fail_dialog->geometry);
}

static void
on_screen_size_changed (GdkScreen          *screen,
                        GsmFailWhaleDialog *fail_dialog)
{
        gtk_widget_queue_resize (GTK_WIDGET (fail_dialog));
}

static void
gsm_fail_whale_dialog_realize (GtkWidget *widget)
{
        if (GTK_WIDGET_CLASS (gsm_fail_whale_dialog_parent_class)->realize) {
                GTK_WIDGET_CLASS (gsm_fail_whale_dialog_parent_class)->realize (widget);
        }

        _window_override_user_time (GSM_FAIL_WHALE_DIALOG (widget));
        update_geometry (GSM_FAIL_WHALE_DIALOG (widget));
        _window_move_resize_window (GSM_FAIL_WHALE_DIALOG (widget), TRUE, TRUE);

        g_signal_connect (gtk_window_get_screen (GTK_WINDOW (widget)),
                          "size_changed",
                          G_CALLBACK (on_screen_size_changed),
                          widget);
}

static void
gsm_fail_whale_dialog_unrealize (GtkWidget *widget)
{
        g_signal_handlers_disconnect_by_func (gtk_window_get_screen (GTK_WINDOW (widget)),
                                              on_screen_size_changed,
                                              widget);

        if (GTK_WIDGET_CLASS (gsm_fail_whale_dialog_parent_class)->unrealize) {
                GTK_WIDGET_CLASS (gsm_fail_whale_dialog_parent_class)->unrealize (widget);
        }
}

static void
gsm_fail_whale_dialog_size_request (GtkWidget      *widget,
                                    GtkRequisition *requisition)
{
        GsmFailWhaleDialog *fail_dialog;
        GdkRectangle   old_geometry;
        int            position_changed = FALSE;
        int            size_changed = FALSE;

        fail_dialog = GSM_FAIL_WHALE_DIALOG (widget);

        old_geometry = fail_dialog->geometry;

        update_geometry (fail_dialog);

        requisition->width  = fail_dialog->geometry.width;
        requisition->height = fail_dialog->geometry.height;

        if (!gtk_widget_get_realized (widget)) {
                return;
        }

        if (old_geometry.width  != fail_dialog->geometry.width ||
            old_geometry.height != fail_dialog->geometry.height) {
                size_changed = TRUE;
        }

        if (old_geometry.x != fail_dialog->geometry.x ||
            old_geometry.y != fail_dialog->geometry.y) {
                position_changed = TRUE;
        }

        _window_move_resize_window (fail_dialog,
                                    position_changed, size_changed);
}

static void
gsm_fail_whale_dialog_get_preferred_width (GtkWidget *widget,
                                           gint      *minimal_width,
                                           gint      *natural_width)
{
        GtkRequisition requisition;

        gsm_fail_whale_dialog_size_request (widget, &requisition);

        *minimal_width = *natural_width = requisition.width;
}

static void
gsm_fail_whale_dialog_get_preferred_width_for_height (GtkWidget *widget,
                                                      gint       for_height,
                                                      gint      *minimal_width,
                                                      gint      *natural_width)
{
        GtkRequisition requisition;

        gsm_fail_whale_dialog_size_request (widget, &requisition);

        *minimal_width = *natural_width = requisition.width;
}

static void
gsm_fail_whale_dialog_get_preferred_height (GtkWidget *widget,
                                            gint      *minimal_height,
                                            gint      *natural_height)
{
        GtkRequisition requisition;

        gsm_fail_whale_dialog_size_request (widget, &requisition);

        *minimal_height = *natural_height = requisition.height;
}

static void
gsm_fail_whale_dialog_get_preferred_height_for_width (GtkWidget *widget,
                                                      gint       for_width,
                                                      gint      *minimal_height,
                                                      gint      *natural_height)
{
        GtkRequisition requisition;

        gsm_fail_whale_dialog_size_request (widget, &requisition);

        *minimal_height = *natural_height = requisition.height;
}

static void
gsm_fail_whale_dialog_class_init (GsmFailWhaleDialogClass *klass)
{
        GtkWidgetClass *widget_class;

        widget_class = GTK_WIDGET_CLASS (klass);

        widget_class->realize = gsm_fail_whale_dialog_realize;
        widget_class->unrealize = gsm_fail_whale_dialog_unrealize;
        widget_class->get_preferred_width = gsm_fail_whale_dialog_get_preferred_width;
        widget_class->get_preferred_height = gsm_fail_whale_dialog_get_preferred_height;
        widget_class->get_preferred_width_for_height = gsm_fail_whale_dialog_get_preferred_width_for_height;
        widget_class->get_preferred_height_for_width = gsm_fail_whale_dialog_get_preferred_height_for_width;
}

static void
on_logout_clicked (GtkWidget          *button,
                   GsmFailWhaleDialog *fail_dialog)
{
        if (!fail_dialog->debug_mode) {
                g_spawn_command_line_async ("budgie-session-quit --force", NULL);
        }
        gtk_main_quit ();
}

static void
setup_window (GsmFailWhaleDialog *fail_dialog)
{
        GtkWidget *box;
        GtkWidget *image;
        GtkWidget *label;
        GtkWidget *message_label;
        GtkWidget *button_box;
        GtkWidget *button;
        GdkPixbuf *fail_icon;
        GdkDisplay *display;
        char *markup;
        int scale_factor;
        int i;

        gtk_window_set_title (GTK_WINDOW (fail_dialog), "");
        gtk_window_set_icon_name (GTK_WINDOW (fail_dialog), GSM_ICON_COMPUTER_FAIL);

        gtk_window_set_skip_taskbar_hint (GTK_WINDOW (fail_dialog), TRUE);
        gtk_window_set_keep_above (GTK_WINDOW (fail_dialog), TRUE);
        gtk_window_stick (GTK_WINDOW (fail_dialog));
        gtk_window_set_position (GTK_WINDOW (fail_dialog), GTK_WIN_POS_CENTER_ALWAYS);

        /* only works if there is a window manager which is unlikely */
        display = gtk_widget_get_display (GTK_WIDGET (fail_dialog));
        for (i = 0; i < gdk_display_get_n_monitors (display); i++) {
                if (gdk_display_get_monitor (display, i) == fail_dialog->monitor) {
                        GdkScreen *screen;

                        screen = gtk_widget_get_screen (GTK_WIDGET (fail_dialog));
                        gtk_window_fullscreen_on_monitor (GTK_WINDOW (fail_dialog),
                                                          screen, i);
                        break;
                }
        }

        box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_valign (box, GTK_ALIGN_CENTER);
        gtk_widget_show (box);

        gtk_container_add (GTK_CONTAINER (fail_dialog), box);

        scale_factor = gdk_monitor_get_scale_factor (fail_dialog->monitor);
        fail_icon = gtk_icon_theme_load_icon_for_scale (gtk_icon_theme_get_default (),
                                                        GSM_ICON_COMPUTER_FAIL,
                                                        128,
                                                        scale_factor,
                                                        0,
                                                        NULL);
        if (fail_icon != NULL) {
                image = gtk_image_new_from_pixbuf (fail_icon);
                gtk_widget_show (image);
                gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);
                g_object_unref (fail_icon);
        }

        label = gtk_label_new (NULL);
        markup = g_strdup_printf ("<b><big>%s</big></b>", _("Oh no!  Something has gone wrong."));
        gtk_label_set_markup (GTK_LABEL (label), markup);
        g_free (markup);
        gtk_widget_show (label);
        gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

        if (!fail_dialog->allow_logout)
                message_label = gtk_label_new (_("A problem has occurred and the system can’t recover. Please contact a system administrator"));
        else if (fail_dialog->extensions)
                message_label = gtk_label_new (_("A problem has occurred and the system can’t recover. All extensions have been disabled as a precaution."));
        else
                message_label = gtk_label_new (_("A problem has occurred and the system can’t recover.\nPlease log out and try again."));

        gtk_label_set_justify (GTK_LABEL (message_label), GTK_JUSTIFY_CENTER);
        gtk_label_set_line_wrap (GTK_LABEL (message_label), TRUE);
        gtk_widget_show (message_label);
        gtk_box_pack_start (GTK_BOX (box),
                            message_label, FALSE, FALSE, 0);

        button_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
        gtk_container_set_border_width (GTK_CONTAINER (button_box), 20);
        gtk_widget_show (button_box);
        gtk_box_pack_end (GTK_BOX (box),
                          button_box, FALSE, FALSE, 0);

        if (fail_dialog->allow_logout) {
                button = gtk_button_new_with_mnemonic (_("_Log Out"));
                gtk_widget_show (button);
                gtk_box_pack_end (GTK_BOX (button_box),
                                  button, FALSE, FALSE, 0);
                g_signal_connect (button, "clicked",
                                  G_CALLBACK (on_logout_clicked), fail_dialog);
        }
}

static void
gsm_fail_whale_dialog_init (GsmFailWhaleDialog *fail_dialog)
{
}

static gboolean debug_mode = FALSE;
static gboolean allow_logout = FALSE;
static gboolean extensions = FALSE;
static GList *dialogs = NULL;

static void
create_fail_dialog (GdkMonitor *monitor)
{
        GsmFailWhaleDialog *fail_dialog;

        fail_dialog = g_object_new (GSM_TYPE_FAIL_WHALE_DIALOG, NULL);
        fail_dialog->debug_mode = debug_mode;
        fail_dialog->allow_logout = allow_logout;
        fail_dialog->extensions = extensions;
        fail_dialog->monitor = monitor;

        setup_window (fail_dialog);

        g_signal_connect (fail_dialog, "destroy",
                          G_CALLBACK (gtk_main_quit), NULL);

        gtk_widget_show (GTK_WIDGET (fail_dialog));

        dialogs = g_list_prepend (dialogs, fail_dialog);
}

static void
on_monitor_added (GdkDisplay  *display,
                  GdkMonitor  *monitor)
{
        create_fail_dialog (monitor);
}

static void
on_monitor_removed (GdkDisplay  *display,
                    GdkMonitor  *monitor)
{
        GList *l;

        for (l = dialogs; l;) {
                GList *next = l->next;
                GsmFailWhaleDialog *fail_dialog = l->data;

                if (fail_dialog->monitor == monitor) {
                        dialogs = g_list_delete_link (dialogs, l);
                        gtk_widget_destroy (GTK_WIDGET (fail_dialog));
                }
                l = next;
        }
}

int main (int argc, char *argv[])
{
        GOptionEntry entries[] = {
                 { "debug", 0, 0, G_OPTION_ARG_NONE, &debug_mode, N_("Enable debugging code"), NULL },
                 { "allow-logout", 0, 0, G_OPTION_ARG_NONE, &allow_logout, N_("Allow logout"), NULL },
                 { "extensions", 0, 0, G_OPTION_ARG_NONE, &extensions, N_("Show extension warning"), NULL },
                { NULL, 0, 0, 0, NULL, NULL, NULL }
        };

        GError *error = NULL;
        GdkDisplay *display;
        int i;

        bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        if (!gtk_init_with_args (&argc, &argv, " - fail whale",
                                 entries, GETTEXT_PACKAGE,
                                 &error)) {
            if (error != NULL) {
                g_warning ("%s", error->message);
                exit (1);
            }

            /* display server probably went away. Could be for legitimate reasons, could be for
             * unexpected reasons.  If it went away unexpectantly, that's logged elsewhere, so
             * let's not add noise by logging here.
             */
            return 0;
        }

        /* Force-off allow_logout when running inside GDM, this is needed
         * because the systemd service always passes --allow-logout
         */
        if (g_strcmp0 (g_getenv ("RUNNING_UNDER_GDM"), "true") == 0)
                allow_logout = FALSE;

        display = gdk_display_get_default ();
        for (i = 0; i < gdk_display_get_n_monitors (display); i++) {
                create_fail_dialog (gdk_display_get_monitor (display, i));
        }

        g_signal_connect (display, "monitor-added",
                          G_CALLBACK (on_monitor_added), &dialogs);
        g_signal_connect (display, "monitor-removed",
                          G_CALLBACK (on_monitor_removed), &dialogs);

        gtk_main ();

        return 0;
}

