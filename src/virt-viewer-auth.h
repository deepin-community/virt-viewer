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

#pragma once

#include "config.h"

#include "virt-viewer-session.h"

#define VIRT_VIEWER_TYPE_AUTH virt_viewer_auth_get_type()
G_DECLARE_FINAL_TYPE(VirtViewerAuth,
                     virt_viewer_auth,
                     VIRT_VIEWER,
                     AUTH,
                     GtkDialog)

GType virt_viewer_auth_get_type(void) G_GNUC_CONST;

VirtViewerAuth *virt_viewer_auth_new(GtkWindow *parent);

gboolean virt_viewer_auth_collect_credentials(VirtViewerAuth *auth,
                                              const char *type,
                                              const char *address,
                                              char **username,
                                              char **password);
