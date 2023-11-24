/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <config.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "virt-viewer-app.h"

#define VIRT_VIEWER_TEST_TYPE virt_viewer_test_get_type()
G_DECLARE_FINAL_TYPE(VirtViewerTest,
                     virt_viewer_test,
                     VIRT_VIEWER,
                     TEST,
                     VirtViewerApp)

struct _VirtViewerTest {
    VirtViewerApp parent;
};

GType virt_viewer_test_get_type (void);

G_DEFINE_TYPE (VirtViewerTest, virt_viewer_test, VIRT_VIEWER_TYPE_APP)

VirtViewerTest *virt_viewer_test_new (void);

static void
virt_viewer_test_class_init (VirtViewerTestClass *klass G_GNUC_UNUSED)
{
}

static void
virt_viewer_test_init(VirtViewerTest *self G_GNUC_UNUSED)
{
}

static void
test_hotkeys_good(void)
{
    const gchar *hotkeys[] = {
        "toggle-fullscreen=shift+f11",
        "release-cursor=shift+f12,secure-attention=ctrl+shift+b",
        "zoom-in=shift+f2,zoom-out=shift+f3,zoom-reset=shift+f4",
        // Setting the smartcard hotkeys causes
        // gtk_application_get_accels_for_action() in
        // virt_viewer_update_smartcard_accels() to call
        // gdk_keymap_get_for_display(), which fails if called within
        // the CI pipeline because it has no X11 display.
        //"smartcard-insert=shift+I,smartcard-remove=shift+R",
    };

    guint i;

    VirtViewerTest *viewer = g_object_new(VIRT_VIEWER_TEST_TYPE, NULL);
    VirtViewerApp *app = VIRT_VIEWER_APP(viewer);
    for (i = 0; i < G_N_ELEMENTS(hotkeys); i++) {
        virt_viewer_app_set_hotkeys(app, hotkeys[i]);
    }
    g_object_unref(viewer);
}

static void
test_hotkeys_bad(void)
{
    const struct {
        const gchar *hotkey_str;
        const GLogLevelFlags log_level;
        const gchar *message;
    } hotkeys[] = {
        {
            "no_value",
            G_LOG_LEVEL_WARNING,
            "Missing value for hotkey 'no_value'"
        },{
            "smartcard-insert=",
            G_LOG_LEVEL_WARNING,
            "Missing value for hotkey 'smartcard-insert'"
        },{
            "toggle-fullscreen=A,unknown_command=B",
            G_LOG_LEVEL_WARNING,
            "Unknown hotkey name unknown_command"
        },{
            "secure-attention=value",
            G_LOG_LEVEL_WARNING,
            "Invalid hotkey 'value' for 'secure-attention'"
        },
    };

    guint i;

    VirtViewerTest *viewer = g_object_new(VIRT_VIEWER_TEST_TYPE, NULL);
    VirtViewerApp *app = VIRT_VIEWER_APP(viewer);
    for (i = 0; i < G_N_ELEMENTS(hotkeys); i++) {
        g_test_expect_message(G_LOG_DOMAIN, hotkeys[i].log_level, hotkeys[i].message);
        virt_viewer_app_set_hotkeys(app, hotkeys[i].hotkey_str);
        g_test_assert_expected_messages();
    }
    g_object_unref(viewer);
}

int main(int argc, char* argv[])
{
    gtk_init_check(&argc, &argv);
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/virt-viewer/good-hotkeys", test_hotkeys_good);
    g_test_add_func("/virt-viewer/bad-hotkeys", test_hotkeys_bad);

    return g_test_run();
}
