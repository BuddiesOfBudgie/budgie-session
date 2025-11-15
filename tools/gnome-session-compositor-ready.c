/*
 * This file is part of budgie-session.
 *
 * Copyright Budgie Desktop Developers
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
 */
#include <wayland-client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

static bool have_compositor = false;
static bool have_seat = false;
static bool have_xdg_wm_base = false;

static void handle_global(void *data,
                          struct wl_registry *registry,
                          uint32_t name,
                          const char *interface,
                          uint32_t version) {
    if (strcmp(interface, "wl_compositor") == 0) {
        have_compositor = true;
        printf("✅ Found wl_compositor\n");
    } else if (strcmp(interface, "wl_seat") == 0) {
        have_seat = true;
        printf("✅ Found wl_seat\n");
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        have_xdg_wm_base = true;
        printf("✅ Found xdg_wm_base\n");
    }
}

static void handle_global_remove(void *data,
                                 struct wl_registry *registry,
                                 uint32_t name) {
    // Nothing needed here for readiness check
}

static const struct wl_registry_listener registry_listener = {
    .global = handle_global,
    .global_remove = handle_global_remove,
};

int main(int argc, char *argv[]) {
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "❌ Failed to connect to Wayland display\n");
        return 1;
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    if (!registry) {
        fprintf(stderr, "❌ Failed to get Wayland registry\n");
        wl_display_disconnect(display);
        return 1;
    }

    wl_registry_add_listener(registry, &registry_listener, NULL);

    // Roundtrip once to get initial globals
    wl_display_roundtrip(display);

    // Poll until we have everything or timeout
    const int timeout_sec = 30;
    time_t start = time(NULL);

    while (!(have_compositor && have_seat && have_xdg_wm_base)) {
        if (time(NULL) - start > timeout_sec) {
            fprintf(stderr, "❌ Timeout waiting for compositor readiness\n");
            wl_display_disconnect(display);
            return 2;
        }
        wl_display_dispatch(display);
    }

    printf("🎉 Compositor is fully initialized!\n");
    wl_display_disconnect(display);
    return 0;
}
