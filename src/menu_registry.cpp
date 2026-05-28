/*
 * menu_registry.cpp — dynamic menu builder.
 *
 * Stores registrations in flat arrays, then build() wires them into
 * menu_node_t trees that the existing renderers consume unmodified.
 */
#include "menu_registry.h"
#include <string.h>
#include <Arduino.h>

/* ---- internal storage ---- */

struct reg_submenu_t {
    char     parent_path[64];   /* "" = root */
    char     hotkey;
    char     label[32];
    char     hint[80];
    char     info[512];
    bool     used;
};

struct reg_feature_t {
    char           path[64];    /* parent submenu path */
    char           hotkey;
    char           label[32];
    char           hint[80];
    char           info[512];
    menu_action_fn action;
    bool           used;
};

#define MAX_SUBMENUS 32
#define MAX_FEATURES 128

static reg_submenu_t s_submenus[MAX_SUBMENUS];
static int           s_n_submenus = 0;

static reg_feature_t s_features[MAX_FEATURES];
static int           s_n_features = 0;

/* Built menu_node_t arrays. Each submenu gets its own array of children
 * plus a terminator. We allocate them statically — no heap, no fragmentation. */
static menu_node_t s_built_children[MAX_SUBMENUS][MENU_MAX_CHILDREN + 1];
static menu_node_t s_root_children[MENU_MAX_CHILDREN + 1];
static menu_node_t s_root_node;

static bool s_built = false;

/* ---- registration API ---- */

void MenuRegistry::add(const char *path, char hotkey, const char *label,
                       const char *hint, menu_action_fn action,
                       const char *info)
{
    if (s_n_features >= MAX_FEATURES) {
        Serial.printf("[menu_reg] WARN: feature overflow, skipping %s\n", label);
        return;
    }
    reg_feature_t &f = s_features[s_n_features++];
    strncpy(f.path, path, sizeof(f.path) - 1);
    f.hotkey = hotkey;
    strncpy(f.label, label, sizeof(f.label) - 1);
    strncpy(f.hint, hint, sizeof(f.hint) - 1);
    if (info) strncpy(f.info, info, sizeof(f.info) - 1);
    else f.info[0] = '\0';
    f.action = action;
    f.used = true;
}

void MenuRegistry::add_submenu(const char *parent_path, char hotkey,
                               const char *label, const char *hint,
                               const char *info)
{
    if (s_n_submenus >= MAX_SUBMENUS) {
        Serial.printf("[menu_reg] WARN: submenu overflow, skipping %s\n", label);
        return;
    }
    reg_submenu_t &s = s_submenus[s_n_submenus++];
    strncpy(s.parent_path, parent_path, sizeof(s.parent_path) - 1);
    s.hotkey = hotkey;
    strncpy(s.label, label, sizeof(s.label) - 1);
    strncpy(s.hint, hint, sizeof(s.hint) - 1);
    if (info) strncpy(s.info, info, sizeof(s.info) - 1);
    else s.info[0] = '\0';
    s.used = true;
}

/* ---- tree builder ---- */

/* Find which submenu index owns the given path (e.g. "WiFi"), or -1. */
static int find_submenu(const char *path)
{
    for (int i = 0; i < s_n_submenus; ++i) {
        /* Build the full path of submenu i. For top-level menus the path
         * is just the label; for nested ones it would be parent/label. */
        char full[96];
        if (s_submenus[i].parent_path[0]) {
            snprintf(full, sizeof(full), "%s/%s", s_submenus[i].parent_path,
                     s_submenus[i].label);
        } else {
            strncpy(full, s_submenus[i].label, sizeof(full) - 1);
        }
        if (strcasecmp(full, path) == 0) return i;
    }
    return -1;
}

/* Build the children array for a submenu at index `si`, or for root if si == -1. */
static void build_children(int si)
{
    menu_node_t *out;
    int count = 0;
    const char *my_path;
    char my_full_path[96] = "";

    if (si < 0) {
        out = s_root_children;
        my_path = "";
    } else {
        out = s_built_children[si];
        if (s_submenus[si].parent_path[0]) {
            snprintf(my_full_path, sizeof(my_full_path), "%s/%s",
                     s_submenus[si].parent_path, s_submenus[si].label);
        } else {
            strncpy(my_full_path, s_submenus[si].label, sizeof(my_full_path) - 1);
        }
        my_path = my_full_path;
    }

    /* First, add child submenus. */
    for (int i = 0; i < s_n_submenus && count < MENU_MAX_CHILDREN; ++i) {
        /* A submenu belongs here if its parent_path matches my_path. */
        bool match;
        if (si < 0) {
            match = (s_submenus[i].parent_path[0] == '\0');
        } else {
            match = (strcasecmp(s_submenus[i].parent_path, my_path) == 0);
        }
        if (!match) continue;

        /* Recursively build this child submenu's children first. */
        build_children(i);

        menu_node_t &n = out[count++];
        n.hotkey   = s_submenus[i].hotkey;
        n.label    = s_submenus[i].label;
        n.hint     = s_submenus[i].hint;
        n.children = s_built_children[i];
        n.action   = nullptr;
        n.info     = s_submenus[i].info[0] ? s_submenus[i].info : nullptr;
    }

    /* Then, add leaf features. */
    for (int i = 0; i < s_n_features && count < MENU_MAX_CHILDREN; ++i) {
        bool match;
        if (si < 0) {
            match = (s_features[i].path[0] == '\0');
        } else {
            match = (strcasecmp(s_features[i].path, my_path) == 0);
        }
        if (!match) continue;

        menu_node_t &n = out[count++];
        n.hotkey   = s_features[i].hotkey;
        n.label    = s_features[i].label;
        n.hint     = s_features[i].hint;
        n.children = nullptr;
        n.action   = s_features[i].action;
        n.info     = s_features[i].info[0] ? s_features[i].info : nullptr;
    }

    /* Null terminator. */
    memset(&out[count], 0, sizeof(menu_node_t));
}

void MenuRegistry::build()
{
    if (s_built) return;

    Serial.printf("[menu_reg] building tree: %d submenus, %d features\n",
                  s_n_submenus, s_n_features);

    /* Build all children arrays starting from root. */
    build_children(-1);

    /* Wire up the root node. */
    s_root_node.hotkey   = '/';
    s_root_node.label    = "POSEIDON";
    s_root_node.hint     = "press letter to enter";
    s_root_node.children = s_root_children;
    s_root_node.action   = nullptr;
    s_root_node.info     = "POSEIDON — keyboard-first pentesting firmware for M5Stack Cardputer.";

    s_built = true;
    Serial.println("[menu_reg] tree built OK");
}

const menu_node_t *MenuRegistry::root()
{
    return &s_root_node;
}
