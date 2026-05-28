/*
 * menu_registry — dynamic self-registration system for POSEIDON features.
 *
 * Instead of hardcoding every menu entry in menu.cpp, feature modules
 * call MenuRegistry::add() at file scope via the REGISTER_FEATURE macro.
 * The registry builds the menu_node_t arrays at boot and hands them to
 * the existing terminal + carousel renderers unchanged.
 *
 * Benefits:
 *   - Adding a feature = one REGISTER_FEATURE() line in your .cpp
 *   - No merge conflicts in menu.cpp
 *   - Features are self-contained compilation units
 *   - Menu tree is still compile-time-known (static constructors run
 *     before setup()), so the renderer sees a stable tree.
 */
#pragma once

#include "menu.h"

/* Maximum items per submenu. Sized for headroom — we're at ~15 root
 * entries and ~17 in WiFi today. */
#define MENU_MAX_CHILDREN 32

/* Path separator for nested menus: "WiFi/Scan" registers under WiFi. */
#define MENU_PATH_SEP '/'

class MenuRegistry {
public:
    /* Register a leaf feature node under a parent path.
     * path:   "WiFi" or "WiFi/SubMenu" — creates intermediates as needed.
     * hotkey: single-char mnemonic
     * label:  display name
     * hint:   one-line description
     * action: function pointer
     * info:   long-form help text (optional, can be nullptr)
     */
    static void add(const char *path, char hotkey, const char *label,
                    const char *hint, menu_action_fn action,
                    const char *info = nullptr);

    /* Register a submenu node (no action, just a container).
     * Used for top-level categories like "WiFi", "Bluetooth", etc.
     * hotkey: mnemonic for this submenu in its parent
     * label/hint/info: display strings
     * parent_path: where this submenu lives ("" = root)
     */
    static void add_submenu(const char *parent_path, char hotkey,
                            const char *label, const char *hint,
                            const char *info = nullptr);

    /* Finalize the tree — converts the flat registration list into
     * menu_node_t arrays. Called once from setup() before menu_run(). */
    static void build();

    /* Access the built tree root. */
    static const menu_node_t *root();
};

/*
 * Convenience macro for feature self-registration.
 * Place at file scope in your feature .cpp:
 *
 *   REGISTER_FEATURE("WiFi", 's', "Scan", "Scan nearby APs", feat_wifi_scan,
 *       "Detailed help text...");
 *
 * The __attribute__((constructor)) ensures registration happens before
 * setup() runs, so the tree is ready when menu_run() starts.
 */
#define REGISTER_FEATURE(path, hotkey, label, hint, action, ...) \
    static void _reg_##action(void) __attribute__((constructor)); \
    static void _reg_##action(void) { \
        MenuRegistry::add(path, hotkey, label, hint, action, ##__VA_ARGS__); \
    }

#define REGISTER_SUBMENU(parent, hotkey, label, hint, ...) \
    static void _regsub_##hotkey(void) __attribute__((constructor)); \
    static void _regsub_##hotkey(void) { \
        MenuRegistry::add_submenu(parent, hotkey, label, hint, ##__VA_ARGS__); \
    }
