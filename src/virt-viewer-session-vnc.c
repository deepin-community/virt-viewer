/*
 * Virt Viewer: A virtual machine console viewer
 *
 * Copyright (C) 2007-2012 Red Hat, Inc.
 * Copyright (C) 2009-2012 Daniel P. Berrange
 * Copyright (C) 2010 Marc-André Lureau
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

/* gtk-vnc uses deprecated API, so disable warnings for this file */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <gvnc.h>

#include "virt-viewer-auth.h"
#include "virt-viewer-session-vnc.h"
#include "virt-viewer-display-vnc.h"

#include <glib/gi18n.h>
#include <libxml/uri.h>

#ifndef VNC_CHECK_VERSION
# define VNC_CHECK_VERSION(a, b, c) 0
#endif
#if VNC_CHECK_VERSION(1, 2, 0)
# define HAVE_VNC_POWER_CONTROL
#endif

struct _VirtViewerSessionVnc {
    VirtViewerSession parent;
    GtkWindow *main_window;
    VirtViewerAuth *auth;
    /* XXX we should really just have a VncConnection */
    VncDisplay *vnc;
    gboolean auth_dialog_cancelled;
    gchar *error_msg;
    gboolean power_control;
};

G_DEFINE_TYPE(VirtViewerSessionVnc, virt_viewer_session_vnc, VIRT_VIEWER_TYPE_SESSION)

static void virt_viewer_session_vnc_close(VirtViewerSession* session);
static gboolean virt_viewer_session_vnc_open_fd(VirtViewerSession* session, int fd);
static gboolean virt_viewer_session_vnc_open_host(VirtViewerSession* session, const gchar *host, const gchar *port, const gchar *tlsport);
static gboolean virt_viewer_session_vnc_open_uri(VirtViewerSession* session, const gchar *uri, GError **error);
static gboolean virt_viewer_session_vnc_channel_open_fd(VirtViewerSession* session,
                                                        VirtViewerSessionChannel* channel, int fd);
static void virt_viewer_session_vnc_vm_action(VirtViewerSession *self, gint action);
static gboolean virt_viewer_session_vnc_has_vm_action(VirtViewerSession *self, gint action);


static void
virt_viewer_session_vnc_finalize(GObject *obj)
{
    VirtViewerSessionVnc *self = VIRT_VIEWER_SESSION_VNC(obj);

    if (self->vnc) {
        vnc_display_close(self->vnc);
        g_object_unref(self->vnc);
    }
    gtk_widget_destroy(GTK_WIDGET(self->auth));
    if (self->main_window)
        g_object_unref(self->main_window);
    g_free(self->error_msg);

    G_OBJECT_CLASS(virt_viewer_session_vnc_parent_class)->finalize(obj);
}

static const gchar*
virt_viewer_session_vnc_mime_type(VirtViewerSession *self G_GNUC_UNUSED)
{
    return "application/x-vnc";
}

static void virt_viewer_session_vnc_vm_action(VirtViewerSession *session G_GNUC_UNUSED,
                                              gint action G_GNUC_UNUSED)
{
#ifdef HAVE_VNC_POWER_CONTROL
    VirtViewerSessionVnc *self = VIRT_VIEWER_SESSION_VNC(session);
    VncConnection *conn = vnc_display_get_connection(self->vnc);

    switch (action) {
    case VIRT_VIEWER_SESSION_VM_ACTION_POWER_DOWN:
        vnc_connection_power_control(conn,
                                     VNC_CONNECTION_POWER_ACTION_SHUTDOWN);
        break;
    case VIRT_VIEWER_SESSION_VM_ACTION_RESET:
        vnc_connection_power_control(conn,
                                     VNC_CONNECTION_POWER_ACTION_RESET);
        break;
    }
#endif
}

static gboolean virt_viewer_session_vnc_has_vm_action(VirtViewerSession *session, gint action)
{
    VirtViewerSessionVnc *self = VIRT_VIEWER_SESSION_VNC(session);

    switch (action) {
    case VIRT_VIEWER_SESSION_VM_ACTION_POWER_DOWN:
    case VIRT_VIEWER_SESSION_VM_ACTION_RESET:
        return self->power_control;
    }

    return FALSE;
}

static void
virt_viewer_session_vnc_class_init(VirtViewerSessionVncClass *klass)
{
    VirtViewerSessionClass *dclass = VIRT_VIEWER_SESSION_CLASS(klass);
    GObjectClass *oclass = G_OBJECT_CLASS(klass);

    oclass->finalize = virt_viewer_session_vnc_finalize;

    dclass->close = virt_viewer_session_vnc_close;
    dclass->open_fd = virt_viewer_session_vnc_open_fd;
    dclass->open_host = virt_viewer_session_vnc_open_host;
    dclass->open_uri = virt_viewer_session_vnc_open_uri;
    dclass->channel_open_fd = virt_viewer_session_vnc_channel_open_fd;
    dclass->mime_type = virt_viewer_session_vnc_mime_type;
    dclass->vm_action = virt_viewer_session_vnc_vm_action;
    dclass->has_vm_action = virt_viewer_session_vnc_has_vm_action;
}

static void
virt_viewer_session_vnc_init(VirtViewerSessionVnc *self G_GNUC_UNUSED)
{
}

#ifdef HAVE_VNC_POWER_CONTROL
static void
virt_viewer_session_vnc_init_power_control(VncDisplay *vnc G_GNUC_UNUSED,
                                           VirtViewerSessionVnc *self)
{
    self->power_control = TRUE;

    g_object_set(virt_viewer_session_get_app(VIRT_VIEWER_SESSION(self)),
                 "vm-ui", TRUE, NULL);
}
#endif

static void
virt_viewer_session_vnc_connected(VncDisplay *vnc G_GNUC_UNUSED,
                                  VirtViewerSessionVnc *self)
{
    GtkWidget *display = virt_viewer_display_vnc_new(self, self->vnc);
    VirtViewerApp *app = virt_viewer_session_get_app(VIRT_VIEWER_SESSION(self));

    self->auth_dialog_cancelled = FALSE;

    virt_viewer_window_set_display(virt_viewer_app_get_main_window(app),
                                   VIRT_VIEWER_DISPLAY(display));

    g_signal_emit_by_name(self, "session-connected");
    virt_viewer_session_add_display(VIRT_VIEWER_SESSION(self),
                                    VIRT_VIEWER_DISPLAY(display));
}

static void
virt_viewer_session_vnc_disconnected(VncDisplay *vnc G_GNUC_UNUSED,
                                     VirtViewerSessionVnc *self)
{
    GtkWidget *display;
    if (self->auth_dialog_cancelled)
        return;

    virt_viewer_session_clear_displays(VIRT_VIEWER_SESSION(self));
    display = virt_viewer_display_vnc_new(self, self->vnc);
    g_debug("Disconnected");
    g_signal_emit_by_name(self, "session-disconnected", self->error_msg);
    virt_viewer_display_set_enabled(VIRT_VIEWER_DISPLAY(display), FALSE);
    virt_viewer_display_set_show_hint(VIRT_VIEWER_DISPLAY(display),
                                      VIRT_VIEWER_DISPLAY_SHOW_HINT_READY, FALSE);
}

static void
virt_viewer_session_vnc_error(VncDisplay *vnc G_GNUC_UNUSED,
                              const gchar* msg,
                              VirtViewerSessionVnc *session)
{
    g_warning("vnc-session: got vnc error %s", msg);
    /* "vnc-error" is always followed by "vnc-disconnected",
     * so save the error for that signal */
    g_free(session->error_msg);
    session->error_msg = g_strdup(msg);
}

static void
virt_viewer_session_vnc_initialized(VncDisplay *vnc G_GNUC_UNUSED,
                                    VirtViewerSessionVnc *session)
{
    g_signal_emit_by_name(session, "session-initialized");
}

static void
virt_viewer_session_vnc_cut_text(VncDisplay *vnc G_GNUC_UNUSED,
                                 const gchar *text,
                                 VirtViewerSession *session)
{
    g_signal_emit_by_name(session, "session-cut-text", text);
}

static void
virt_viewer_session_vnc_bell(VncDisplay *vnc G_GNUC_UNUSED,
                             VirtViewerSession *session)
{
    g_signal_emit_by_name(session, "session-bell");
}

static void
virt_viewer_session_vnc_auth_unsupported(VncDisplay *vnc G_GNUC_UNUSED,
                                         unsigned int authType,
                                         VirtViewerSessionVnc *session)
{
    g_clear_pointer(&session->error_msg, g_free);
    gchar *msg = g_strdup_printf(_("Unsupported authentication type %u"),
                                 authType);
    g_signal_emit_by_name(session, "session-auth-unsupported", msg);
    g_free(msg);
}

static void
virt_viewer_session_vnc_auth_failure(VncDisplay *vnc G_GNUC_UNUSED,
                                     const gchar *reason,
                                     VirtViewerSessionVnc *session)
{
    g_clear_pointer(&session->error_msg, g_free);
    g_signal_emit_by_name(session, "session-auth-refused", reason);
}



static gboolean
virt_viewer_session_vnc_open_fd(VirtViewerSession* session,
                                int fd)
{
    VirtViewerSessionVnc *self = VIRT_VIEWER_SESSION_VNC(session);

    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(self->vnc != NULL, FALSE);

    return vnc_display_open_fd(self->vnc, fd);
}

static gboolean
virt_viewer_session_vnc_channel_open_fd(VirtViewerSession* session G_GNUC_UNUSED,
                                        VirtViewerSessionChannel* channel G_GNUC_UNUSED,
                                        int fd G_GNUC_UNUSED)
{
    g_warning("channel_open_fd is not supported by VNC");
    return FALSE;
}

static gboolean
virt_viewer_session_vnc_open_host(VirtViewerSession* session,
                                  const gchar *host,
                                  const gchar *port,
                                  const gchar *tlsport G_GNUC_UNUSED)
{
    VirtViewerSessionVnc *self = VIRT_VIEWER_SESSION_VNC(session);

    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(self->vnc != NULL, FALSE);

    return vnc_display_open_host(self->vnc, host, port);
}

static gboolean
virt_viewer_session_vnc_open_uri(VirtViewerSession* session,
                                 const gchar *uristr,
                                 GError **error)
{
    VirtViewerSessionVnc *self = VIRT_VIEWER_SESSION_VNC(session);
    VirtViewerFile *file = virt_viewer_session_get_file(session);
    VirtViewerApp *app = virt_viewer_session_get_app(session);
    gchar *portstr;
    gchar *hoststr = NULL;
    gboolean ret;

    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(self->vnc != NULL, FALSE);

    if (file) {
        g_return_val_if_fail(virt_viewer_file_is_set(file, "port"), FALSE);
        g_return_val_if_fail(virt_viewer_file_is_set(file, "host"), FALSE);

        portstr = g_strdup_printf("%d", virt_viewer_file_get_port(file));
        hoststr = g_strdup(virt_viewer_file_get_host(file));

        if (!virt_viewer_file_fill_app(file, app, error))
            return FALSE;
    } else {
        xmlURIPtr uri = NULL;
        if (!(uri = xmlParseURI(uristr)))
            return FALSE;

        portstr = g_strdup_printf("%d", uri->port);

        if (uri->server) {
            if (uri->server[0] == '[') {
                gchar *tmp;
                hoststr = g_strdup(uri->server + 1);
                if ((tmp = strchr(hoststr, ']')))
                    *tmp = '\0';
            } else {
                hoststr = g_strdup(uri->server);
            }
        }

        xmlFreeURI(uri);
    }

    ret = vnc_display_open_host(self->vnc,
                                hoststr,
                                portstr);
    g_free(portstr);
    g_free(hoststr);
    return ret;
}


static void
virt_viewer_session_vnc_auth_credential(GtkWidget *src G_GNUC_UNUSED,
                                        GValueArray *credList,
                                        VirtViewerSession *session)
{
    VirtViewerSessionVnc *self = VIRT_VIEWER_SESSION_VNC(session);
    char *username = NULL, *password = NULL;
    gboolean wantPassword = FALSE, wantUsername = FALSE;
    VirtViewerFile *file = NULL;
    int i;

    g_debug("Got VNC credential request for %u credential(s)", credList->n_values);

    for (i = 0 ; i < credList->n_values ; i++) {
        GValue *cred = g_value_array_get_nth(credList, i);
        switch (g_value_get_enum(cred)) {
        case VNC_DISPLAY_CREDENTIAL_USERNAME:
            wantUsername = TRUE;
            break;
        case VNC_DISPLAY_CREDENTIAL_PASSWORD:
            wantPassword = TRUE;
            break;
        case VNC_DISPLAY_CREDENTIAL_CLIENTNAME:
            break;
        default:
            g_debug("Unsupported credential type %d", g_value_get_enum(cred));
            vnc_display_close(self->vnc);
            goto cleanup;
        }
    }

    file = virt_viewer_session_get_file(VIRT_VIEWER_SESSION(self));
    if (file != NULL) {
        if (wantUsername) {
            if (virt_viewer_file_is_set(file, "username")) {
                username = virt_viewer_file_get_username(file);
                wantUsername = FALSE;
            } else {
                username = g_strdup(g_get_user_name());
            }
        }
        if (wantPassword && virt_viewer_file_is_set(file, "password")) {
            password = virt_viewer_file_get_password(file);
            wantPassword = FALSE;
        }
    }

    if (wantUsername || wantPassword) {
        gboolean ret = virt_viewer_auth_collect_credentials(self->auth,
                                                            "VNC", NULL,
                                                            wantUsername ? &username : NULL,
                                                            wantPassword ? &password : NULL);

        if (!ret) {
            vnc_display_close(self->vnc);
            self->auth_dialog_cancelled = TRUE;
            g_signal_emit_by_name(self, "session-cancelled");
            goto cleanup;
        }
    }

    for (i = 0 ; i < credList->n_values ; i++) {
        GValue *cred = g_value_array_get_nth(credList, i);
        switch (g_value_get_enum(cred)) {
        case VNC_DISPLAY_CREDENTIAL_USERNAME:
            if (!username ||
                vnc_display_set_credential(self->vnc,
                                           g_value_get_enum(cred),
                                           username)) {
                g_debug("Failed to set credential type %d", g_value_get_enum(cred));
                vnc_display_close(self->vnc);
            }
            break;
        case VNC_DISPLAY_CREDENTIAL_PASSWORD:
            if (!password ||
                vnc_display_set_credential(self->vnc,
                                           g_value_get_enum(cred),
                                           password)) {
                g_debug("Failed to set credential type %d", g_value_get_enum(cred));
                vnc_display_close(self->vnc);
            }
            break;
        case VNC_DISPLAY_CREDENTIAL_CLIENTNAME:
            if (vnc_display_set_credential(self->vnc,
                                           g_value_get_enum(cred),
                                           "libvirt")) {
                g_debug("Failed to set credential type %d", g_value_get_enum(cred));
                vnc_display_close(self->vnc);
            }
            break;
        default:
            g_debug("Unsupported credential type %d", g_value_get_enum(cred));
            vnc_display_close(self->vnc);
        }
    }

 cleanup:
    g_free(username);
    g_free(password);
}


static void
virt_viewer_session_vnc_close(VirtViewerSession* session)
{
    VirtViewerSessionVnc *self = VIRT_VIEWER_SESSION_VNC(session);

    g_return_if_fail(self != NULL);

    g_debug("close vnc=%p", self->vnc);
    if (self->vnc != NULL) {
        gtk_dialog_response(GTK_DIALOG(self->auth),
                            GTK_RESPONSE_CANCEL);

        virt_viewer_session_clear_displays(session);
        vnc_display_close(self->vnc);
        g_object_unref(self->vnc);
    }

    self->vnc = VNC_DISPLAY(vnc_display_new());
    g_object_ref_sink(self->vnc);

    g_signal_connect_object(self->vnc, "vnc-connected",
                            G_CALLBACK(virt_viewer_session_vnc_connected), session, 0);
    g_signal_connect_object(self->vnc, "vnc-initialized",
                            G_CALLBACK(virt_viewer_session_vnc_initialized), session, 0);
    g_signal_connect_object(self->vnc, "vnc-disconnected",
                            G_CALLBACK(virt_viewer_session_vnc_disconnected), session, 0);
    g_signal_connect_object(self->vnc, "vnc-error",
                            G_CALLBACK(virt_viewer_session_vnc_error), session, 0);

    g_signal_connect_object(self->vnc, "vnc-bell",
                            G_CALLBACK(virt_viewer_session_vnc_bell), session, 0);
    g_signal_connect_object(self->vnc, "vnc-auth-failure",
                            G_CALLBACK(virt_viewer_session_vnc_auth_failure), session, 0);
    g_signal_connect_object(self->vnc, "vnc-auth-unsupported",
                            G_CALLBACK(virt_viewer_session_vnc_auth_unsupported), session, 0);
    g_signal_connect_object(self->vnc, "vnc-server-cut-text",
                            G_CALLBACK(virt_viewer_session_vnc_cut_text), session, 0);

    g_signal_connect_object(self->vnc, "vnc-auth-credential",
                            G_CALLBACK(virt_viewer_session_vnc_auth_credential), session, 0);

#ifdef HAVE_VNC_POWER_CONTROL
    g_signal_connect(self->vnc, "vnc-power-control-initialized",
                     G_CALLBACK(virt_viewer_session_vnc_init_power_control), session);
#endif
}

VirtViewerSession *
virt_viewer_session_vnc_new(VirtViewerApp *app, GtkWindow *main_window)
{
    VirtViewerSessionVnc *self;

    self = g_object_new(VIRT_VIEWER_TYPE_SESSION_VNC, "app", app, NULL);

    self->vnc = VNC_DISPLAY(vnc_display_new());
    g_object_ref_sink(self->vnc);
    self->main_window = g_object_ref(main_window);
    self->auth = virt_viewer_auth_new(self->main_window);

    vnc_display_set_shared_flag(self->vnc,
                                virt_viewer_app_get_shared(app));
    vnc_display_set_pointer_local(self->vnc,
                                  virt_viewer_app_get_cursor(app) == VIRT_VIEWER_CURSOR_LOCAL);

    g_signal_connect_object(self->vnc, "vnc-connected",
                            G_CALLBACK(virt_viewer_session_vnc_connected), self, 0);
    g_signal_connect_object(self->vnc, "vnc-initialized",
                            G_CALLBACK(virt_viewer_session_vnc_initialized), self, 0);
    g_signal_connect_object(self->vnc, "vnc-disconnected",
                            G_CALLBACK(virt_viewer_session_vnc_disconnected), self, 0);
    g_signal_connect_object(self->vnc, "vnc-error",
                            G_CALLBACK(virt_viewer_session_vnc_error), self, 0);

    g_signal_connect_object(self->vnc, "vnc-bell",
                            G_CALLBACK(virt_viewer_session_vnc_bell), self, 0);
    g_signal_connect_object(self->vnc, "vnc-auth-failure",
                            G_CALLBACK(virt_viewer_session_vnc_auth_failure), self, 0);
    g_signal_connect_object(self->vnc, "vnc-auth-unsupported",
                            G_CALLBACK(virt_viewer_session_vnc_auth_unsupported), self, 0);
    g_signal_connect_object(self->vnc, "vnc-server-cut-text",
                            G_CALLBACK(virt_viewer_session_vnc_cut_text), self, 0);

    g_signal_connect_object(self->vnc, "vnc-auth-credential",
                            G_CALLBACK(virt_viewer_session_vnc_auth_credential), self, 0);

#ifdef HAVE_VNC_POWER_CONTROL
    g_signal_connect(self->vnc, "vnc-power-control-initialized",
                     G_CALLBACK(virt_viewer_session_vnc_init_power_control), self);
#endif

    return VIRT_VIEWER_SESSION(self);
}
