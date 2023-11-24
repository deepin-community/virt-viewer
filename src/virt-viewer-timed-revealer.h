/*
 * Virt Viewer: A virtual machine console viewer
 *
 * Copyright (c) 2016 Red Hat, Inc.
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
 * Author: Cole Robinson <crobinso@redhat.com>
 * Author: Fabiano Fidêncio <fidencio@redhat.com>
 */

#pragma once

#include <glib-object.h>
#include <gtk/gtk.h>

#define VIRT_VIEWER_TYPE_TIMED_REVEALER virt_viewer_timed_revealer_get_type()
G_DECLARE_FINAL_TYPE(VirtViewerTimedRevealer,
		     virt_viewer_timed_revealer,
		     VIRT_VIEWER,
		     TIMED_REVEALER,
		     GtkEventBox)

GType virt_viewer_timed_revealer_get_type (void);

VirtViewerTimedRevealer *
virt_viewer_timed_revealer_new(GtkWidget *toolbar);

void
virt_viewer_timed_revealer_force_reveal(VirtViewerTimedRevealer *self,
                                        gboolean fullscreen);
