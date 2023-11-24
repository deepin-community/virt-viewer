/*
 * Virt Viewer: A virtual machine console viewer
 *
 * Copyright (C) 2018 Red Hat, Inc.
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
 * Author: Marc-André Lureau <marcandre.lureau@redhat.com>
 */

#pragma once

#include <glib-object.h>

#include "virt-viewer-display.h"

#define VIRT_VIEWER_TYPE_DISPLAY_VTE virt_viewer_display_vte_get_type()
G_DECLARE_FINAL_TYPE(VirtViewerDisplayVte,
                     virt_viewer_display_vte,
                     VIRT_VIEWER,
                     DISPLAY_VTE,
                     VirtViewerDisplay)

GType virt_viewer_display_vte_get_type(void);

GtkWidget* virt_viewer_display_vte_new(VirtViewerSession *session, const char *name);

void virt_viewer_display_vte_feed(VirtViewerDisplayVte *vte, gpointer data, int size);

void virt_viewer_display_vte_zoom_reset(VirtViewerDisplayVte *vte);
void virt_viewer_display_vte_zoom_in(VirtViewerDisplayVte *vte);
void virt_viewer_display_vte_zoom_out(VirtViewerDisplayVte *vte);

