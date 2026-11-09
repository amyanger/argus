#include "test_harness.h"
#include "frontend/prefs.h"
#include <stdio.h>
#include <string.h>

#define TEST_PATH "test_prefs.ini"

static void write_file(const char* text) {
    FILE* f = fopen(TEST_PATH, "w");
    if (f) {
        fputs(text, f);
        fclose(f);
    }
}

TEST(prefs_round_trip) {
    static Prefs a, b;
    remove(TEST_PATH);
    prefs_init(&a, TEST_PATH);
    ASSERT_TRUE(!prefs_load(&a));
    ASSERT_TRUE(!prefs_get_bool(&a, "overworld_xray", false));
    ASSERT_TRUE(prefs_set_bool(&a, "overworld_xray", true));
    ASSERT_TRUE(prefs_save(&a));
    prefs_init(&b, TEST_PATH);
    ASSERT_TRUE(prefs_load(&b));
    ASSERT_TRUE(prefs_get_bool(&b, "overworld_xray", false));
    remove(TEST_PATH);
}

TEST(prefs_unknown_keys_survive_save) {
    static Prefs p;
    write_file("future_setting = hello world\noverworld_xray=0\n");
    prefs_init(&p, TEST_PATH);
    ASSERT_TRUE(prefs_load(&p));
    prefs_set_bool(&p, "overworld_xray", true);
    ASSERT_TRUE(prefs_save(&p));
    prefs_init(&p, TEST_PATH);
    prefs_load(&p);
    ASSERT_TRUE(prefs_get(&p, "future_setting") != NULL);
    ASSERT_STR_EQ(prefs_get(&p, "future_setting"), "hello world");
    ASSERT_STR_EQ(prefs_get(&p, "overworld_xray"), "1");
    remove(TEST_PATH);
}

TEST(prefs_garbage_lines_ignored) {
    static Prefs p;
    write_file("no equals here\n=novalue\n# comment=1\noverworld_xray=maybe\n\n");
    prefs_init(&p, TEST_PATH);
    ASSERT_TRUE(prefs_load(&p));
    ASSERT_EQ(p.count, 1);   /* only overworld_xray=maybe */
    ASSERT_TRUE(prefs_get_bool(&p, "overworld_xray", true));    /* not a bool: fallback */
    ASSERT_TRUE(!prefs_get_bool(&p, "overworld_xray", false));
    remove(TEST_PATH);
}

TEST(prefs_long_line_does_not_split) {
    static Prefs p;
    char text[700];
    memset(text, 'a', 600);
    memcpy(text + 600, "x=1\nok=1\n", 10);   /* a tail that looks like a pair must not parse */
    write_file(text);
    prefs_init(&p, TEST_PATH);
    ASSERT_TRUE(prefs_load(&p));
    ASSERT_TRUE(prefs_get(&p, "x") == NULL);
    ASSERT_TRUE(prefs_get(&p, "ok") != NULL);
    ASSERT_STR_EQ(prefs_get(&p, "ok"), "1");
    remove(TEST_PATH);
}

TEST(prefs_set_rejects_bad_input_and_full_table) {
    static Prefs p;
    char key[16];
    prefs_init(&p, "");
    ASSERT_TRUE(!prefs_set(&p, "a=b", "1"));
    ASSERT_TRUE(!prefs_set(&p, "a", "line\nbreak"));
    for (int i = 0; i < PREFS_MAX; i++) {
        snprintf(key, sizeof(key), "k%d", i);
        ASSERT_TRUE(prefs_set(&p, key, "1"));
    }
    ASSERT_TRUE(!prefs_set(&p, "one_more", "1"));
    ASSERT_TRUE(prefs_set(&p, "k3", "2"));   /* replacing still works when full */
    ASSERT_STR_EQ(prefs_get(&p, "k3"), "2");
}

TEST(prefs_long_value_round_trip) {
    static Prefs p, q;
    prefs_init(&p, "gba_prefs_long.ini");
    char path[900];
    memset(path, 'a', sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';
    ASSERT_TRUE(prefs_set(&p, "bios_path", path));
    ASSERT_TRUE(prefs_save(&p));
    prefs_init(&q, "gba_prefs_long.ini");
    ASSERT_TRUE(prefs_load(&q));
    ASSERT_STR_EQ(prefs_get(&q, "bios_path"), path);
    remove("gba_prefs_long.ini");
}

TEST(prefs_save_fails_on_bad_path) {
    static Prefs p;
    prefs_init(&p, "no_such_dir_for_prefs/prefs.ini");
    prefs_set_bool(&p, "overworld_xray", true);
    ASSERT_TRUE(!prefs_save(&p));
    prefs_init(&p, "");
    ASSERT_TRUE(!prefs_save(&p));
    ASSERT_TRUE(!prefs_load(&p));
}

static bool file_has(const char* path, const char* text) {
    char buf[64] = "";
    FILE* f = fopen(path, "r");
    if (!f) return false;
    bool ok = fgets(buf, sizeof(buf), f) && strcmp(buf, text) == 0;
    fclose(f);
    return ok;
}

TEST(prefs_migrate_moves_old_files_and_keeps_new_ones) {
    FILE* f;
    remove("mig_new_prefs.ini");
    remove("mig_new_library.txt");
    remove("mig_new_keymap.ini");
    remove("mig_old_keymap.ini");
    if ((f = fopen("mig_old_prefs.ini", "w"))) { fputs("old", f); fclose(f); }
    if ((f = fopen("mig_old_library.txt", "w"))) { fputs("old", f); fclose(f); }
    if ((f = fopen("mig_new_library.txt", "w"))) { fputs("new", f); fclose(f); }
    prefs_migrate_dir("mig_old_", "mig_new_");
    ASSERT_TRUE(file_has("mig_new_prefs.ini", "old"));     // moved
    ASSERT_TRUE(!file_has("mig_old_prefs.ini", "old"));
    ASSERT_TRUE(file_has("mig_new_library.txt", "new"));   // newer file kept
    ASSERT_TRUE(file_has("mig_old_library.txt", "old"));   // old one left alone
    ASSERT_TRUE(!file_has("mig_new_keymap.ini", ""));      // nothing to move, nothing made
    remove("mig_new_prefs.ini");
    remove("mig_new_library.txt");
    remove("mig_old_library.txt");
}

void run_prefs_tests(void) {
    RUN_TEST(prefs_migrate_moves_old_files_and_keeps_new_ones);
    RUN_TEST(prefs_round_trip);
    RUN_TEST(prefs_unknown_keys_survive_save);
    RUN_TEST(prefs_garbage_lines_ignored);
    RUN_TEST(prefs_long_line_does_not_split);
    RUN_TEST(prefs_set_rejects_bad_input_and_full_table);
    RUN_TEST(prefs_long_value_round_trip);
    RUN_TEST(prefs_save_fails_on_bad_path);
}
