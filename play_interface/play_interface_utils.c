//
// Created by Leo on 2018/8/28.
//

#include <regex.h>
#include "play_interface.h"
#include "Zend/zend_exceptions.h"
#include "../play_lib/uthash/utarray.h"

static UT_array *lock_file_array = NULL;
static const unsigned char *z_type_name[11] = {"UNDEF", "NULL", "bool", "bool", "int", "double", "string", "array", "object","resource", "reference"};

const char *play_interface_utils_get_zval_type_name(int idx)
{
    return z_type_name[idx];
}

void play_interface_utils_trigger_exception(int type, char *format, ...)
{
    va_list args;
    char *message;

    va_start(args, format);
    vspprintf(&message, 0, format, args);
    va_end(args);

    zend_throw_exception(zend_hash_str_find_ptr(EG(class_table), "exception", 9), message, type);
    efree(message);
}

void play_interface_utils_append_crontab_mutex_file(char *lock_file)
{
    if (lock_file_array == NULL) {
        utarray_new(lock_file_array, &ut_str_icd);
        utarray_push_back(lock_file_array, &lock_file);
    } else {
        int find = 0;
        char **p = NULL;
        while ( (p = (char **)utarray_next(lock_file_array, p))) {
            if (memcmp(lock_file, *p, strlen(lock_file)) == 0) {
                find = 1;break;
            }
        }
        if (!find) {
            utarray_push_back(lock_file_array, &lock_file);
        }
    }
}

void play_interface_utils_clean_crontab_mutex_file()
{
    if (lock_file_array != NULL) {
        char **p = NULL;
        while ( (p = (char **)utarray_next(lock_file_array, p))) {
            remove(*p);
            // free(*p);
        }
        utarray_free(lock_file_array);
    }
}

int play_interface_utils_valid(zval *val, char *pattern)
{
    if (Z_TYPE_P(val) == IS_ARRAY) {
        zval *z_item = NULL;
        int count = zend_hash_num_elements(Z_ARRVAL_P(val));
        zend_hash_internal_pointer_reset(Z_ARRVAL_P(val));
        int i;
        for (i = 0; i < count; i ++) {
            z_item = zend_hash_get_current_data(Z_ARRVAL_P(val));
            if (!play_interface_utils_valid(z_item, pattern)) {
                return 0;
            }
            zend_hash_move_forward(Z_ARRVAL_P(val));
        }
        return 1;
    }

    int status;
    int cflags = REG_EXTENDED;
    regmatch_t pmatch[1];
    const size_t nmatch = 1;
    regex_t reg;
    int patternLen = (int)strlen(pattern);

    if (pattern[0] == '/' && pattern[patternLen-1] == '/') {
        char p[256] = {0};
        memcpy(p, pattern+1, patternLen-2);
        regcomp(&reg, p, cflags);
    } else {
        regcomp(&reg, pattern, cflags);
    }
    status = regexec(&reg, Z_STRVAL_P(val), nmatch, pmatch, 0);
    regfree(&reg);

    if (status == 0) {
        return 1;
    }
    return 0;
}

int play_interface_utils_find_target_file(char *path, char *file_list[], int *file_list_count, char *target)
{
    if (access(path, X_OK) != 0) {
        return -1;
    }

    DIR *pDir;
    size_t namelen = 0;
    struct dirent *ent = NULL;
    pDir = opendir(path);
    size_t filepathlen = strlen(path);
    while (NULL != (ent = readdir(pDir))) {
        if (ent->d_type == 8) {
            namelen = strlen(ent->d_name);
            if (namelen > 4 && memcmp(ent->d_name+(namelen - 4), ".php", 4) == 0 && memcmp(ent->d_name, target, strlen(target)) == 0) {
                file_list[*file_list_count] = calloc(strlen(ent->d_name)+filepathlen+2, 1);
                sprintf(file_list[*file_list_count], "%s/%s", path, ent->d_name);
                (*file_list_count)++;
            }
            continue;
        }
        if (ent->d_type == 4) {
            if (memcmp(ent->d_name, ".", 1) != 0) {
                char file[512];
                snprintf(file, 512, "%s/%s", path, ent->d_name);
                play_interface_utils_find_target_file(file, file_list, file_list_count, target);
            }
        }
    }
    closedir(pDir);
    return 0;
}

int play_interface_utils_load_config(char *path, zval *result) {
    zend_file_handle file_handle;
    zend_op_array *op_array;
    char realpath[MAXPATHLEN];

    if (!VCWD_REALPATH(path, realpath)) {
        return 0;
    }

    file_handle.filename = path;
    file_handle.free_filename = 0;
    file_handle.type = ZEND_HANDLE_FILENAME;
    file_handle.opened_path = NULL;
    file_handle.handle.fp = NULL;

    op_array = zend_compile_file(&file_handle, ZEND_INCLUDE);

    if (op_array && file_handle.handle.stream.handle) {
        if (!file_handle.opened_path) {
            file_handle.opened_path = zend_string_init(path, strlen(path), 0);
        }

        zend_hash_add_empty_element(&EG(included_files), file_handle.opened_path);
    }
    zend_destroy_file_handle(&file_handle);
    if (op_array) {
        ZVAL_UNDEF(result);
        zend_execute(op_array, result);
        destroy_op_array(op_array);
        efree(op_array);
        if (!EG(exception)) {
            return 1;
        }
        return 0;
    }
    return 0;
}

int play_interface_utils_loader_import(char *path) {
    zend_file_handle file_handle;
    zend_op_array *op_array;
    char realpath[MAXPATHLEN];

    if (!VCWD_REALPATH(path, realpath)) {
        return 0;
    }

    file_handle.filename = path;
    file_handle.free_filename = 0;
    file_handle.type = ZEND_HANDLE_FILENAME;
    file_handle.opened_path = NULL;
    file_handle.handle.fp = NULL;

    op_array = zend_compile_file(&file_handle, ZEND_INCLUDE);

    if (op_array && file_handle.handle.stream.handle) {
        if (!file_handle.opened_path) {
            file_handle.opened_path = zend_string_init(path, strlen(path), 0);
        }

        zend_hash_add_empty_element(&EG(included_files), file_handle.opened_path);
    }
    zend_destroy_file_handle(&file_handle);

    if (op_array) {
        zval result;
        ZVAL_UNDEF(&result);
        zend_execute(op_array, &result);
        destroy_op_array(op_array);
        efree(op_array);
        if (!EG(exception)) {
            zval_ptr_dtor(&result);
        }
        return 1;
    }
    return 0;
}
