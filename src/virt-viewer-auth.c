/*
 * Virt Viewer: A virtual machine console viewer
 *
 * Copyright (C) 2007-2012 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>

#ifdef HAVE_GTK_VNC
#include <vncdisplay.h>
#endif

#include "virt-viewer-auth.h"
#include "virt-viewer-util.h"


struct _VirtViewerAuth
{
    GtkDialog parent;
    GtkWidget *credUsername;
    GtkWidget *credPassword;
    GtkWidget *promptUsername;
    GtkWidget *promptPassword;
    GtkWidget *message;
};

G_DEFINE_TYPE(VirtViewerAuth, virt_viewer_auth, GTK_TYPE_DIALOG)

static void
virt_viewer_auth_class_init(VirtViewerAuthClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    gtk_widget_class_set_template_from_resource(widget_class,
                                                VIRT_VIEWER_RESOURCE_PREFIX "/ui/virt-viewer-auth.ui");

    gtk_widget_class_bind_template_child(widget_class,
                                         VirtViewerAuth,
                                         message);
    gtk_widget_class_bind_template_child(widget_class,
                                         VirtViewerAuth,
                                         credUsername);
    gtk_widget_class_bind_template_child(widget_class,
                                         VirtViewerAuth,
                                         credPassword);
    gtk_widget_class_bind_template_child(widget_class,
                                         VirtViewerAuth,
                                         promptUsername);
    gtk_widget_class_bind_template_child(widget_class,
                                         VirtViewerAuth,
                                         promptPassword);
}

static void
virt_viewer_auth_init(VirtViewerAuth *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));

    gtk_dialog_set_default_response(GTK_DIALOG(self), GTK_RESPONSE_OK);
}

VirtViewerAuth *virt_viewer_auth_new(GtkWindow *parent)
{
    return g_object_new(VIRT_VIEWER_TYPE_AUTH,
                        "transient-for", parent,
                        NULL);
}

static void
show_password(GtkEntry *entry,
              GtkEntryIconPosition pos G_GNUC_UNUSED,
              GdkEvent event G_GNUC_UNUSED)
{
    gboolean visible = gtk_entry_get_visibility(entry);
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                      visible ?
                                      "eye-not-looking-symbolic" :
                                      "eye-open-negative-filled-symbolic");
    gtk_entry_set_visibility(entry, !visible);
}

/* NOTE: if username is provided, and *username is non-NULL, the user input
 * field will be pre-filled with this value. The existing string will be freed
 * before setting the output parameter to the user-entered value.
 */
gboolean
virt_viewer_auth_collect_credentials(VirtViewerAuth *self,
                                     const char *type,
                                     const char *address,
                                     char **username,
                                     char **password)
{
    int response;
    char *message;

    gtk_entry_set_text(GTK_ENTRY(self->credUsername), "");
    gtk_entry_set_text(GTK_ENTRY(self->credPassword), "");
    if (username) {
        gtk_widget_show(self->credUsername);
        gtk_widget_show(self->promptUsername);
        if (*username) {
            gtk_entry_set_text(GTK_ENTRY(self->credUsername), *username);
            /* if username is pre-filled, move focus to password field */
            if (password) {
                gtk_widget_grab_focus(self->credPassword);
            }
        }
    } else {
        gtk_widget_hide(self->credUsername);
        gtk_widget_hide(self->promptUsername);
    }
    if (password) {
        gtk_widget_show(self->credPassword);
        gtk_widget_show(self->promptPassword);
    } else {
        gtk_widget_hide(self->credPassword);
        gtk_widget_hide(self->promptPassword);
    }

    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(self->credPassword),
                                      GTK_ENTRY_ICON_SECONDARY,
                                      "eye-not-looking-symbolic");
    gtk_entry_set_icon_sensitive(GTK_ENTRY(self->credPassword),
                                 GTK_ENTRY_ICON_SECONDARY,
                                 TRUE);
    gtk_entry_set_icon_activatable(GTK_ENTRY(self->credPassword),
                                   GTK_ENTRY_ICON_SECONDARY,
                                   TRUE);
    gtk_entry_set_icon_tooltip_text(GTK_ENTRY(self->credPassword),
                                    GTK_ENTRY_ICON_SECONDARY,
                                    _("Show / hide password text"));

    g_signal_connect(self->credPassword, "icon-press",
                     G_CALLBACK(show_password), NULL);

    if (address) {
        message = g_strdup_printf(_("Authentication is required for the %s connection to:\n\n<b>%s</b>\n\n"),
                                  type,
                                  address);
    } else {
        message = g_strdup_printf(_("Authentication is required for the %s connection:\n"),
                                  type);
    }

    gtk_label_set_markup(GTK_LABEL(self->message), message);
    g_free(message);

    gtk_widget_show(GTK_WIDGET(self));
    response = gtk_dialog_run(GTK_DIALOG(self));
    gtk_widget_hide(GTK_WIDGET(self));

    if (response == GTK_RESPONSE_OK) {
        if (username) {
            g_free(*username);
            *username = g_strdup(gtk_entry_get_text(GTK_ENTRY(self->credUsername)));
        }
        if (password)
            *password = g_strdup(gtk_entry_get_text(GTK_ENTRY(self->credPassword)));
    }

    return response == GTK_RESPONSE_OK;
}
