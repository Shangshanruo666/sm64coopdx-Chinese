#include <unistd.h>
#include <sys/stat.h>
#include "mods.h"
#include "mods_utils.h"
#include "pc/debuglog.h"

void mods_size_enforce(struct Mods* mods) {
    for (int i = 0; i < mods->entryCount; i++) {
        struct Mod* mod = mods->entries[i];
        if (mod->size >= MAX_MOD_SIZE) {
            mod->enabled = false;
            mod->selectable = false;
        }
    }
}

static bool mods_incompatible_match(struct Mod* a, struct Mod* b) {
    if (a->incompatible == NULL || b->incompatible == NULL) {
        return false;
    }
    if (strlen(a->incompatible) == 0 || strlen(b->incompatible) == 0) {
        return false;
    }

    char* ai = a->incompatible;
    char* bi = b->incompatible;
    char* atoken = NULL;
    char* btoken = NULL;
    char* arest = NULL;
    char* brest = NULL;

    for (atoken = strtok_r(ai, " ", &arest); atoken != NULL; atoken = strtok_r(NULL, " ", &arest)) {
        for (btoken = strtok_r(bi, " ", &brest); btoken != NULL; btoken = strtok_r(NULL, " ", &brest)) {
            if (!strcmp(atoken, btoken)) {
                return true;
            }
        }
    }

    return false;
}

void mods_update_selectable(void) {
    // reset selectable value
    for (int i = 0; i < gLocalMods.entryCount; i++) {
        struct Mod* mod = gLocalMods.entries[i];
        mod->selectable = true;
    }

    // figure out which ones to deselect
    for (int i = 0; i < gLocalMods.entryCount; i++) {
        struct Mod* mod = gLocalMods.entries[i];
        if (mod->enabled) { continue; }

        for (int j = 0; j < gLocalMods.entryCount; j++) {
            if (j == i) { continue; }
            struct Mod* mod2 = gLocalMods.entries[j];
            if (!mod2->enabled) { continue; }

            if (mods_incompatible_match(mod, mod2)) {
                mod->selectable = false;
                break;
            }
        }
    }

    mods_size_enforce(&gLocalMods);
}

static void mods_delete_folder(char* path) {
    LOG_INFO("Deleting tmp folder '%s'", path);
    struct dirent* dir;
    DIR* d = opendir(path);
    if (!d) { return; }

    char fullPath[SYS_MAX_PATH] = { 0 };
    while ((dir = readdir(d)) != NULL) {
        if (!strcmp(dir->d_name, ".")) { continue; }
        if (!strcmp(dir->d_name, "..")) { continue; }
        if (!concat_path(fullPath, path, dir->d_name)) { continue; }

        if (is_directory(fullPath)) {
            mods_delete_folder(fullPath);
        } else if (fs_sys_file_exists(fullPath)) {
            if (unlink(fullPath) == -1) {
                LOG_ERROR("Failed to remove tmp file '%s'", fullPath);
                continue;
            }
        }

    }

    closedir(d);
    rmdir(path);
}

void mods_delete_tmp(void) {
    // ensure tmpPath exists
    char tmpPath[SYS_MAX_PATH] = { 0 };
    if (snprintf(tmpPath, SYS_MAX_PATH - 1, "%s", fs_get_write_path(TMP_DIRECTORY)) < 0) {
        LOG_ERROR("Failed to concat tmp path");
        return;
    }

    // sanity
    if (strlen(tmpPath) < 1) { return; }

    // delete
    mods_delete_folder(tmpPath);
}
//////////////////////////////////////////////////////////////////////////////////////////

bool mod_file_full_path(char* destination, struct Mod* mod, struct ModFile* modFile) {
    if (!concat_path(destination, mod->basePath, modFile->relativePath)) { return false; }
    normalize_path(destination);
    return true;
}

bool mod_file_create_directories(struct Mod* mod, struct ModFile* modFile) {
    char path[SYS_MAX_PATH] = { 0 };
    if (!mod_file_full_path(path, mod, modFile)) {
        return false;
    }

    // sanity
    if (strlen(path) < 1) { return false; }

    char tmpPath[SYS_MAX_PATH] = { 0 };
    char* p = path;
    u16 index = 0;
    while (*p != '\0') {
        if (*p == '/' || *p == '\\') {
            if (snprintf(tmpPath, index + 1, "%s", path) < 0) { }
            if (!fs_sys_dir_exists(tmpPath)) { fs_sys_mkdir(tmpPath); }
        }
        index++;
        p++;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

bool str_ends_with(char* string, char* suffix) {
    if (string == NULL || suffix == NULL) { return false; }

    size_t stringLength = strlen(string);
    size_t suffixLength = strlen(suffix);

    if (suffixLength > stringLength) { return false; }

    return !strcmp(&string[stringLength - suffixLength], suffix);
}

//////////////////////////////////////////////////////////////////////////////////////////

char* extract_lua_field(char* fieldName, char* buffer) {
    size_t length = strlen(fieldName);
    if (strncmp(fieldName, buffer, length) == 0) {
        char* s = &buffer[length];
        while (*s == ' ' || *s == '\t') { s++; }
        return s;
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////

bool path_exists(char* path) {
    struct stat sb = { 0 };
    return (stat(path, &sb) == 0);
}

bool is_directory(char* path) {
    struct stat sb = { 0 };
    return (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode));
}

void normalize_path(char* path) {
    // replace slashes
    char* p = path;
    while (*p) {
#if defined(_WIN32)
        if (*p == '/') { *p = '\\'; }
#else
        if (*p == '\\') { *p = '/'; }
#endif
        p++;
    }
}

bool concat_path(char* destination, char* path, char* fname) {
    return (snprintf(destination, SYS_MAX_PATH - 1, "%s/%s", path, fname) >= 0);
}

char* path_basename(char* path) {
    char* base = path;
    while (*path != '\0') {
        if (*(path + 1) != '\0') {
            if (*path == '\\' || *path == '/') {
                base = path + 1;
            }
        }
        path++;
    }
    return base;
}

bool directory_sanity_check(struct dirent* dir, char* dirPath, char* outPath) {
    // skip anything that contains \ or /
    if (strchr(dir->d_name, '/') != NULL)  { return false; }
    if (strchr(dir->d_name, '\\') != NULL) { return false; }

    // skip anything that starts with .
    if (dir->d_name == NULL || dir->d_name[0] == '.') { return false; }

    // build path
    if (!concat_path(outPath, dirPath, dir->d_name)) {
        LOG_ERROR("Failed to concat path '%s' + '%s'", dirPath, dir->d_name);
        return false;
    }
    normalize_path(outPath);

    // sanity check
    if (!path_exists(outPath)) {
        LOG_ERROR("Path doesn't exist: '%s'", outPath);
        return false;
    }

    return true;
}