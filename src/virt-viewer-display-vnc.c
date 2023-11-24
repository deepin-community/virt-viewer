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

#include <gvnc.h>

#include "virt-viewer-auth.h"
#include "virt-viewer-display-vnc.h"
#include "virt-viewer-util.h"

#include <glib/gi18n.h>

#ifndef VNC_CHECK_VERSION
# define VNC_CHECK_VERSION(a, b, c) 0
#endif
#if VNC_CHECK_VERSION(1, 2, 0)
# define HAVE_VNC_REMOTE_RESIZE
#endif

struct _VirtViewerDisplayVnc {
    VirtViewerDisplay parent;
    VncDisplay *vnc;
};

G_DEFINE_TYPE(VirtViewerDisplayVnc, virt_viewer_display_vnc, VIRT_VIEWER_TYPE_DISPLAY)

static void virt_viewer_display_vnc_send_keys(VirtViewerDisplay* display, const guint *keyvals, int nkeyvals);
static GdkPixbuf *virt_viewer_display_vnc_get_pixbuf(VirtViewerDisplay* display);
static void virt_viewer_display_vnc_close(VirtViewerDisplay *display);

static void
virt_viewer_display_vnc_finalize(GObject *obj)
{
    VirtViewerDisplayVnc *self = VIRT_VIEWER_DISPLAY_VNC(obj);

    g_object_unref(self->vnc);

    G_OBJECT_CLASS(virt_viewer_display_vnc_parent_class)->finalize(obj);
}


static void
virt_viewer_display_vnc_release_cursor(VirtViewerDisplay *display)
{
    VirtViewerDisplayVnc *self = VIRT_VIEWER_DISPLAY_VNC(display);

    vnc_display_force_grab(self->vnc, FALSE);
}

static void
virt_viewer_display_vnc_class_init(VirtViewerDisplayVncClass *klass)
{
    VirtViewerDisplayClass *dclass = VIRT_VIEWER_DISPLAY_CLASS(klass);
    GObjectClass *oclass = G_OBJECT_CLASS(klass);

    oclass->finalize = virt_viewer_display_vnc_finalize;

    dclass->send_keys = virt_viewer_display_vnc_send_keys;
    dclass->get_pixbuf = virt_viewer_display_vnc_get_pixbuf;
    dclass->close = virt_viewer_display_vnc_close;
    dclass->release_cursor = virt_viewer_display_vnc_release_cursor;
}

static void
virt_viewer_display_vnc_init(VirtViewerDisplayVnc *self G_GNUC_UNUSED)
{
}


static void
virt_viewer_display_vnc_mouse_grab(VncDisplay *vnc G_GNUC_UNUSED,
                                   VirtViewerDisplay *display)
{
    g_signal_emit_by_name(display, "display-pointer-grab");
}


static void
virt_viewer_display_vnc_mouse_ungrab(VncDisplay *vnc G_GNUC_UNUSED,
                                     VirtViewerDisplay *display)
{
    g_signal_emit_by_name(display, "display-pointer-ungrab");
}

static void
virt_viewer_display_vnc_key_grab(VncDisplay *vnc G_GNUC_UNUSED,
                                 VirtViewerDisplay *display)
{
    g_signal_emit_by_name(display, "display-keyboard-grab");
}

static void
virt_viewer_display_vnc_key_ungrab(VncDisplay *vnc G_GNUC_UNUSED,
                                   VirtViewerDisplay *display)
{
    g_signal_emit_by_name(display, "display-keyboard-ungrab");
}

static void
virt_viewer_display_vnc_initialized(VncDisplay *vnc G_GNUC_UNUSED,
                                    VirtViewerDisplay *display)
{
    gchar *name = NULL;
    gchar *uuid = NULL;

    VirtViewerSession *session = virt_viewer_display_get_session(display);
    VirtViewerApp *app = virt_viewer_session_get_app(session);

    g_object_get(app, "guest-name", &name, "uuid", &uuid, NULL);
    if (name == NULL || *name == '\0') {
        const gchar * vnc_name = vnc_display_get_name(vnc);
        if (vnc_name != NULL) {
            g_object_set(app, "guest-name", vnc_name, NULL);
        }
    }
    if (uuid == NULL || *uuid == '\0') {
        g_object_set(app, "uuid", _("VNC does not provide GUID"), NULL);
    }

    virt_viewer_display_set_enabled(display, TRUE);
    virt_viewer_display_set_show_hint(display,
                                      VIRT_VIEWER_DISPLAY_SHOW_HINT_READY, TRUE);

    g_free(name);
    g_free(uuid);
}

static void
virt_viewer_display_vnc_send_keys(VirtViewerDisplay* display,
                                  const guint *keyvals,
                                  int nkeyvals)
{
    VirtViewerDisplayVnc *self = VIRT_VIEWER_DISPLAY_VNC(display);

    g_return_if_fail(self != NULL);
    g_return_if_fail(keyvals != NULL);
    g_return_if_fail(self->vnc != NULL);

    vnc_display_send_keys(self->vnc, keyvals, nkeyvals);
}


static GdkPixbuf *
virt_viewer_display_vnc_get_pixbuf(VirtViewerDisplay* display)
{
    VirtViewerDisplayVnc *self = VIRT_VIEWER_DISPLAY_VNC(display);

    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(self->vnc != NULL, NULL);

    return vnc_display_get_pixbuf(self->vnc);
}


/*
 * Called when desktop size changes.
 *
 * It either tries to resize the main window, or it triggers
 * recalculation of the display within existing window size
 */
static void
virt_viewer_display_vnc_resize_desktop(VncDisplay *vnc G_GNUC_UNUSED,
                                       int width, int height,
                                       VirtViewerDisplay *display)
{
    g_debug("desktop resize %dx%d", width, height);

    virt_viewer_display_set_desktop_size(display, width, height);
}


static void
release_cursor_display_hotkey_changed(VirtViewerApp *app,
                                      GParamSpec *pspec G_GNUC_UNUSED,
                                      VncDisplay *vnc)
{
    gboolean kiosk;
    gchar *hotkey;
    g_object_get(app, "kiosk", &kiosk, NULL);
    hotkey = virt_viewer_app_get_release_cursor_display_hotkey(app);

    if(kiosk || hotkey == NULL) {
        /* disable default grab sequence */
        VncGrabSequence *seq = vnc_grab_sequence_new(0, NULL);
        vnc_display_set_grab_keys(vnc, seq);
        vnc_grab_sequence_free(seq);
    } else {
        hotkey = spice_hotkey_to_display_hotkey(hotkey);
        VncGrabSequence *seq = vnc_grab_sequence_new_from_string(hotkey);
        g_free(hotkey);
        vnc_display_set_grab_keys(vnc, seq);
        vnc_grab_sequence_free(seq);
    }
}


#ifdef HAVE_VNC_REMOTE_RESIZE
static void
resize_policy_changed(VirtViewerDisplayVnc *self,
                      GParamSpec *pspec G_GNUC_UNUSED,
                      VncDisplay *vnc)
{
    gboolean enable = virt_viewer_display_get_auto_resize(VIRT_VIEWER_DISPLAY(self));

    vnc_display_set_allow_resize(vnc, enable);
}

static void
zoom_level_changed(VirtViewerDisplay *display,
                   GParamSpec *pspec G_GNUC_UNUSED,
                   VncDisplay *vnc)
{
    guint zoom = virt_viewer_display_get_zoom_level(display);

    vnc_display_set_zoom_level(vnc, zoom);
}
#endif


GtkWidget *
virt_viewer_display_vnc_new(VirtViewerSessionVnc *session,
                            VncDisplay *vnc)
{
    VirtViewerDisplayVnc *self;
    VirtViewerApp *app;

    self = g_object_new(VIRT_VIEWER_TYPE_DISPLAY_VNC, "session", session, NULL);

    g_object_ref(vnc);
    self->vnc = vnc;

    gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(self->vnc));
    vnc_display_set_keyboard_grab(self->vnc, TRUE);
    vnc_display_set_pointer_grab(self->vnc, TRUE);

    /*
     * In auto-resize mode we have things setup so that we always
     * automatically resize the top level window to be exactly the
     * same size as the VNC desktop, except when it won't fit on
     * the local screen, at which point we let it scale down.
     * The upshot is, we always want scaling enabled.
     * We disable force_size because we want to allow user to
     * manually size the widget smaller too
     */
    vnc_display_set_force_size(self->vnc, FALSE);
    vnc_display_set_scaling(self->vnc, TRUE);

#ifdef HAVE_VNC_REMOTE_RESIZE
    vnc_display_set_keep_aspect_ratio(self->vnc, TRUE);
    g_object_set(self, "force-aspect", FALSE, NULL);
#endif

    /* When VNC desktop resizes, we have to resize the containing widget */
    g_signal_connect(self->vnc, "vnc-desktop-resize",
                     G_CALLBACK(virt_viewer_display_vnc_resize_desktop), self);

    g_signal_connect(self->vnc, "vnc-pointer-grab",
                     G_CALLBACK(virt_viewer_display_vnc_mouse_grab), self);
    g_signal_connect(self->vnc, "vnc-pointer-ungrab",
                     G_CALLBACK(virt_viewer_display_vnc_mouse_ungrab), self);
    g_signal_connect(self->vnc, "vnc-keyboard-grab",
                     G_CALLBACK(virt_viewer_display_vnc_key_grab), self);
    g_signal_connect(self->vnc, "vnc-keyboard-ungrab",
                     G_CALLBACK(virt_viewer_display_vnc_key_ungrab), self);
    g_signal_connect(self->vnc, "vnc-initialized",
                     G_CALLBACK(virt_viewer_display_vnc_initialized), self);

    app = virt_viewer_session_get_app(VIRT_VIEWER_SESSION(session));
    virt_viewer_signal_connect_object(app, "notify::release-cursor-display-hotkey",
                                      G_CALLBACK(release_cursor_display_hotkey_changed), self->vnc, 0);
    release_cursor_display_hotkey_changed(app, NULL, self->vnc);

#ifdef HAVE_VNC_REMOTE_RESIZE
    virt_viewer_signal_connect_object(self, "notify::auto-resize",
                                      G_CALLBACK(resize_policy_changed), self->vnc, 0);
    virt_viewer_signal_connect_object(self, "notify::zoom-level",
                                      G_CALLBACK(zoom_level_changed), self->vnc, 0);
    resize_policy_changed(self, NULL, self->vnc);
#endif

    return GTK_WIDGET(self);
}


static void
virt_viewer_display_vnc_close(VirtViewerDisplay *display)
{
    VirtViewerDisplayVnc *self = VIRT_VIEWER_DISPLAY_VNC(display);

    /* We're not the real owner, so we shouldn't be letting the container
     * destroy the widget. There are still signals that need to be
     * propagated to the VirtViewerSession
     */
    gtk_container_remove(GTK_CONTAINER(display), GTK_WIDGET(self->vnc));
}
