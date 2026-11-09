#ifndef PREFS_H
#define PREFS_H

#include <stdbool.h>
#include <stdint.h>

/* key=value preferences in one small file. Pure C, no SDL, no heap: the
 * frontend passes the path (SDL_GetPrefPath). Unknown keys survive a save. */

#define PREFS_MAX      32
#define PREFS_KEY_LEN  32
#define PREFS_VAL_LEN  1024
#define PREFS_PATH_LEN 1024

typedef struct { char key[PREFS_KEY_LEN]; char val[PREFS_VAL_LEN]; } PrefsEntry;
typedef struct {
    PrefsEntry items[PREFS_MAX];
    uint32_t count;
    char path[PREFS_PATH_LEN];   /* "" = never touches disk */
} Prefs;

void prefs_init(Prefs* p, const char* path);
bool prefs_load(Prefs* p);        /* false when missing or unreadable; entries kept as read */
bool prefs_save(const Prefs* p);  /* temp file + rename */
const char* prefs_get(const Prefs* p, const char* key);   /* NULL when absent */
bool prefs_get_bool(const Prefs* p, const char* key, bool fallback);
bool prefs_set(Prefs* p, const char* key, const char* val);  /* false when full, too long or has '=' / newline */
bool prefs_set_bool(Prefs* p, const char* key, bool v);

/* Moves prefs.ini, keymap.ini and library.txt from old_dir to new_dir, skipping
 * any file new_dir already has. Both dirs end in a separator, like SDL_GetPrefPath. */
void prefs_migrate_dir(const char* old_dir, const char* new_dir);

#endif // PREFS_H
