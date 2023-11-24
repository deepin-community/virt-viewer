/*
 * Virt Viewer: A virtual machine console viewer
 *
 * Copyright (C) 2016 Red Hat, Inc.
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
 */

#pragma once

#include <gtk/gtk.h>
#include <spice-client.h>

#define VIRT_VIEWER_TYPE_FILE_TRANSFER_DIALOG virt_viewer_file_transfer_dialog_get_type()
G_DECLARE_FINAL_TYPE(VirtViewerFileTransferDialog,
                     virt_viewer_file_transfer_dialog,
                     VIRT_VIEWER,
                     FILE_TRANSFER_DIALOG,
                     GtkDialog)

GType virt_viewer_file_transfer_dialog_get_type(void) G_GNUC_CONST;

VirtViewerFileTransferDialog *virt_viewer_file_transfer_dialog_new(GtkWindow *parent);
void virt_viewer_file_transfer_dialog_add_task(VirtViewerFileTransferDialog *self,
                                               SpiceFileTransferTask *task);
