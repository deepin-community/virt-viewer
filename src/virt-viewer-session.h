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

#pragma once

#include <gtk/gtk.h>

#include "virt-viewer-app.h"
#include "virt-viewer-file.h"
#include "virt-viewer-display.h"

#define VIRT_VIEWER_TYPE_SESSION virt_viewer_session_get_type()
G_DECLARE_DERIVABLE_TYPE(VirtViewerSession,
                         virt_viewer_session,
                         VIRT_VIEWER,
                         SESSION,
                         GObject);

typedef struct _VirtViewerSessionChannel VirtViewerSessionChannel;

enum {
    VIRT_VIEWER_SESSION_VM_ACTION_QUIT,
    VIRT_VIEWER_SESSION_VM_ACTION_RESET,
    VIRT_VIEWER_SESSION_VM_ACTION_POWER_DOWN,
    VIRT_VIEWER_SESSION_VM_ACTION_PAUSE,
    VIRT_VIEWER_SESSION_VM_ACTION_CONTINUE,
};


struct _VirtViewerSessionClass {
    GObjectClass parent_class;

    /* virtual methods */
    void (* close) (VirtViewerSession* session);
    gboolean (* open_fd) (VirtViewerSession* session, int fd);
    gboolean (* open_host) (VirtViewerSession* session, const gchar *host, const gchar *port, const gchar *tlsport);
    gboolean (* open_uri) (VirtViewerSession* session, const gchar *uri, GError **error);
    gboolean (* channel_open_fd) (VirtViewerSession* session, VirtViewerSessionChannel *channel, int fd);
    void (* usb_device_selection) (VirtViewerSession* session, GtkWindow *parent);
    void (* usb_device_reset) (VirtViewerSession* session);
    void (* smartcard_insert) (VirtViewerSession* session);
    void (* smartcard_remove) (VirtViewerSession* session);
    const gchar* (* mime_type) (VirtViewerSession* session);

    /* monitors = GHashTable<int, GdkRectangle*> */
    void (*apply_monitor_geometry)(VirtViewerSession *session, GHashTable* monitors);
    gboolean (*can_share_folder)(VirtViewerSession *session);
    gboolean (*can_retry_auth)(VirtViewerSession *session);
    void (*vm_action)(VirtViewerSession *session, gint action);
    gboolean (*has_vm_action)(VirtViewerSession *session, gint action);
};

GType virt_viewer_session_get_type(void);

const gchar* virt_viewer_session_mime_type(VirtViewerSession *session);

void virt_viewer_session_add_display(VirtViewerSession *session,
                                     VirtViewerDisplay *display);
void virt_viewer_session_remove_display(VirtViewerSession *session,
                                        VirtViewerDisplay *display);
void virt_viewer_session_clear_displays(VirtViewerSession *session);
void virt_viewer_session_update_displays_geometry(VirtViewerSession *session);

void virt_viewer_session_close(VirtViewerSession* session);
gboolean virt_viewer_session_open_fd(VirtViewerSession* session, int fd);
gboolean virt_viewer_session_open_host(VirtViewerSession* session, const gchar *host, const gchar *port, const gchar *tlsport);
GObject* virt_viewer_session_get(VirtViewerSession* session);
gboolean virt_viewer_session_channel_open_fd(VirtViewerSession* session,
                                             VirtViewerSessionChannel* channel, int fd);
gboolean virt_viewer_session_open_uri(VirtViewerSession *session, const gchar *uri, GError **error);

void virt_viewer_session_set_auto_usbredir(VirtViewerSession* session, gboolean auto_usbredir);
gboolean virt_viewer_session_get_auto_usbredir(VirtViewerSession* session);

void virt_viewer_session_set_has_usbredir(VirtViewerSession* session, gboolean has_usbredir);
gboolean virt_viewer_session_get_has_usbredir(VirtViewerSession *self);

void virt_viewer_session_usb_device_selection(VirtViewerSession *self, GtkWindow *parent);
void virt_viewer_session_usb_device_reset(VirtViewerSession *self);
void virt_viewer_session_smartcard_insert(VirtViewerSession *self);
void virt_viewer_session_smartcard_remove(VirtViewerSession *self);
VirtViewerApp* virt_viewer_session_get_app(VirtViewerSession *self);
gchar* virt_viewer_session_get_uri(VirtViewerSession *self);
void virt_viewer_session_set_file(VirtViewerSession *self, VirtViewerFile *file);
VirtViewerFile* virt_viewer_session_get_file(VirtViewerSession *self);
gboolean virt_viewer_session_can_share_folder(VirtViewerSession *self);
gboolean virt_viewer_session_can_retry_auth(VirtViewerSession *self);

void virt_viewer_session_vm_action(VirtViewerSession *self, gint action);
gboolean virt_viewer_session_has_vm_action(VirtViewerSession *self, gint action);
