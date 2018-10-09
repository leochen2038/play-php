//
// Created by Leo on 18/5/14.
//

#include <zend_interfaces.h>
#include "zend_constants.h"
#include "play_interface.h"
#include "../play_core/play_core.h"

zend_class_entry *play_interface_crontab_ce;

PHP_METHOD(Crontab, setCrontab);
PHP_METHOD(Crontab, getCrontab);
PHP_METHOD(Crontab, checkHit);
PHP_METHOD(Crontab, fork);
PHP_METHOD(Crontab, run){}
PHP_METHOD(Crontab, setMutex);
PHP_METHOD(Crontab, debug);
PHP_METHOD(Crontab, everySeconds);

static int module_number;
static int check_item(char *item, int target);
static int hit_comma(char *item, int target);
static int hit_each(char *item, int target);
static int hit_between(char *item, int target);
static int hit_between_each(char *item, int target);
static void get_crontab_file_by_class_name(char *filepath, char *class_name, size_t class_name_length, char *file);

const zend_function_entry play_interface_crontab_functions[] = {
        PHP_ME(Crontab, setCrontab, NULL, ZEND_ACC_PROTECTED|ZEND_ACC_CTOR)
        PHP_ME(Crontab, setMutex, NULL, ZEND_ACC_PROTECTED|ZEND_ACC_CTOR)
        PHP_ME(Crontab, getCrontab, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
        PHP_ME(Crontab, checkHit, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
        PHP_ME(Crontab, fork, NULL, ZEND_ACC_PROTECTED|ZEND_ACC_CTOR)
        PHP_ME(Crontab, run, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
        PHP_ME(Crontab, debug, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
        PHP_ME(Crontab, everySeconds, NULL, ZEND_ACC_PROTECTED|ZEND_ACC_CTOR)
        {NULL, NULL, NULL}
};

void play_interface_crontab_register(int _module_number)
{
    module_number = _module_number;
    zend_class_entry class;
    INIT_CLASS_ENTRY_EX(class, "Crontab", 7, play_interface_crontab_functions);
    play_interface_crontab_ce = zend_register_internal_class_ex(&class, NULL);
    zend_declare_property_stringl(play_interface_crontab_ce, "_crontab", 8, NULL, 0, ZEND_ACC_PUBLIC);
    zend_declare_property_bool(play_interface_crontab_ce, "_hit", 4, 0, ZEND_ACC_PUBLIC);
    zend_declare_property_bool(play_interface_crontab_ce, "_mutex", 6, 0, ZEND_ACC_PUBLIC);
}

PHP_METHOD(Crontab, setMutex)
{
    zval *mutex = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &mutex) == FAILURE) {
        return;
    }
    if (mutex != NULL) {
        zend_update_property_bool(Z_OBJCE_P(getThis()), getThis(), "_mutex", 6, Z_LVAL_P(mutex));
    }
    RETURN_ZVAL(getThis(), 1, 0);
}

PHP_METHOD(Crontab, getCrontab)
{
    zval *retval = zend_read_property(Z_OBJCE_P(getThis()), getThis(), "_crontab", 8, 0, NULL);
    RETURN_ZVAL(retval, 1, 0);
}

PHP_METHOD(Crontab, checkHit)
{
    zval *retval = zend_read_property(Z_OBJCE_P(getThis()), getThis(), "_hit", 4, 0, NULL);
    RETURN_ZVAL(retval, 1, 0);
}


PHP_METHOD(Crontab, fork)
{
    zval *method, *args;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|zz", &method, &args) == FAILURE) {
        return;
    }

    pid_t fpid;
    fpid = fork();
    if (fpid < 0) {
        php_printf("error in fork!");
    } else if (fpid == 0) {
        char *funName = zend_str_tolower_dup(Z_STRVAL_P(method), Z_STRLEN_P(method));
        zend_call_method(getThis(), Z_OBJCE_P(getThis()), NULL, funName, Z_STRLEN_P(method), NULL, 1, args, NULL);
        php_printf("in crontab fork: %s\n", funName);
        efree(funName);
        exit(0);
    }
}

PHP_METHOD(Crontab, everySeconds)
{
    zval *es;
    zval *mutex = NULL;
    zval *crontab = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|z", &es, &mutex) == FAILURE) {
        return;
    }

    crontab = zend_read_property(Z_OBJCE_P(getThis()), getThis(), "_crontab", 8, 0, NULL);
    if (crontab == NULL || Z_STRLEN_P(crontab) == 0) {
        play_interface_utils_trigger_exception(-1, "please set crontab first\n");
        return;
    }

    if (mutex != NULL && Z_TYPE_P(mutex) == IS_FALSE) {
        zend_update_property_bool(Z_OBJCE_P(getThis()), getThis(), "_mutex", 6, 0);
    } else {
        zend_update_property_bool(Z_OBJCE_P(getThis()), getThis(), "_mutex", 6, 1);
    }

    zend_update_property_long(Z_OBJCE_P(getThis()), getThis(), "_es", 3, Z_LVAL_P(es));
    RETURN_ZVAL(getThis(), 1, 0);
}

PHP_METHOD(Crontab, setCrontab)
{
    int hit = 0;
    zval *crontab_str = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &crontab_str) == FAILURE) {
        return;
    }

    //step 1. 获取当前时间戳
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);
    // char *wday[] = {"日", "－", "二", "三", "四", "五", "六"};
    // printf ("%d-%d-%d ", (1900+p->tm_year), (p->tm_mon+1), p->tm_mday);
    // printf("%s%d:%d:%d\n", wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);

    char **crontabList;
    if (play_explode(&crontabList, Z_STRVAL_P(crontab_str), ' ') != 5) {
        php_printf("crontab error\n");
        return;
    }

//    printf("crontab:%s\n", crontab_str->value.str.val);
    int i;
    int targetType[] = {p->tm_min, p->tm_hour, p->tm_mday,(p->tm_mon+1),p->tm_wday};
    for (i= 4; i >= 0; i--) {
        hit = check_item(crontabList[i], targetType[i]);
        if (hit == 0) {
            break;
        }
    }
    // php_printf("hit:%d, crontab:%s\n", hit, Z_STRVAL_P(crontab_str));
    zend_update_property_bool(Z_OBJCE_P(getThis()), getThis(), "_hit", 4, hit);
    zend_update_property_stringl(Z_OBJCE_P(getThis()), getThis(), "_crontab", 8, Z_STRVAL_P(crontab_str), Z_STRLEN_P(crontab_str));
    zend_update_property_long(Z_OBJCE_P(getThis()), getThis(), "_es", 3, 0);
    RETURN_ZVAL(getThis(), 1, 0);
}

PHP_METHOD(Crontab, debug)
{
    char *lower_class_name = NULL;
    int class_name_length;
    zend_class_entry *ce;
    zval *app_root, *class_name, obj;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|zz", &app_root, &class_name) == FAILURE) {
        return;
    }
    time_t timep;
    time(&timep);
    play_global_config_set_app_root(Z_STRVAL_P(app_root), Z_STRLEN_P(app_root));
    REGISTER_STRINGL_CONSTANT("APP_ROOT", gconfig.app_root_ex, gconfig.app_root_length+1, CONST_CS);
    REGISTER_LONG_CONSTANT("TIME_NOW", timep, CONST_CS);

    char file[1024] = {0};
    get_crontab_file_by_class_name(gconfig.app_root, Z_STRVAL_P(class_name),  Z_STRLEN_P(class_name), file);

    if (strlen(file) == 0) {
        php_printf("can not find class:%s\n", Z_STRVAL_P(class_name));
        return;
    }

    class_name_length = Z_STRLEN_P(class_name);
    lower_class_name = zend_str_tolower_dup(Z_STRVAL_P(class_name), class_name_length);

    if ((ce = zend_hash_str_find_ptr(EG(class_table), lower_class_name, class_name_length)) == NULL) {
        if (!play_interface_utils_loader_import(file)) {
            php_printf("cant not load crontab file %s\n", file);
            return;
        }

        if ((ce = zend_hash_str_find_ptr(EG(class_table), lower_class_name, class_name_length)) == NULL) {
            php_printf("can not find class %s in %s\n", Z_STRVAL_P(class_name), file);
            return;
        }
    }

    efree(lower_class_name);

    object_init_ex(&obj, ce);
    zend_call_method_with_0_params(&obj, ce, NULL, "__construct", NULL);
    zend_call_method_with_0_params(&obj, ce, NULL, "run", NULL);
    zval_ptr_dtor(&obj);
    int status;
    wait(&status);
}

static int check_item(char *item, int target)
{
    if (strcmp(item, "*") == 0) {
        return 1;
    }

    if (index(item, ',') != 0) {
        return hit_comma(item, target);
    } else if (index(item, '-') != 0 && index(item, '/') != 0 ) {
        return hit_between_each(item, target);
    } else if (index(item, '-') != 0) {
        return hit_between(item, target);
    } else if (index(item, '/') != 0) {
        return hit_each(item, target);
    } else if (play_is_numeric(item)) {
        return target == atoi(item) ? 1 : 0;
    }
    return 0;
}

static int hit_each(char *item, int target)
{
    char **each;
    int each_size;

    each_size = play_explode(&each, item, '/');
    if (each_size == 2 && strcmp(each[0], "*") == 0 && play_is_numeric(each[1]) && atoi(each[1]) > 0 && target%atoi(each[1]) == 0) {
        return 1;
    }
    return 0;
}

static int hit_between(char *item, int target)
{
    char **between;
    int between_size;

    between_size = play_explode(&between, item, '-');
    if (between_size == 2 && atoi(between[0]) <= target && target <= atoi(between[1])) {
        return 1;
    }

    return 0;
}

static int hit_between_each(char *item, int target)
{
    char **each, **between;
    int each_size, between_size;

    each_size = play_explode(&each, item, '/');
    if (each_size == 2) {
        between_size = play_explode(&between, each[0], '-');
        if (between_size == 2 && atoi(between[0]) <= target && target <= atoi(between[1]) && play_is_numeric(each[1]) && atoi(each[1]) > 0 && target/atoi(each[1]) == 0) {
            return 1;
        }
    }
    return 0;
}

static int hit_comma(char *item, int target)
{
    int size, i;
    char **tmp;

    size = play_explode(&tmp, item, ',');
    for (i = 0; i < size; i++) {
        if (atoi(tmp[i]) == target) {
            return 1;
        }
    }
    return 0;
}

static void get_crontab_file_by_class_name(char *filepath, char *class_name, size_t class_name_length, char *file)
{
    DIR *pDir;
    size_t namelen = 0;
    struct dirent *ent = NULL;
    pDir = opendir(filepath);

    while (NULL != (ent = readdir(pDir))) {
        if (ent->d_type == 8) {
            namelen = strlen(ent->d_name);
            if (namelen > 4 && memcmp(ent->d_name+(namelen - 4), ".php", 4) == 0 && memcmp(ent->d_name, "Cron", 4) == 0) {
                if (memcmp(ent->d_name, class_name, class_name_length) == 0) {
                    sprintf(file, "%s/%s", filepath, ent->d_name);
                    return ;
                }
            }
        } else if (ent->d_type == 4) {
            if (memcmp(ent->d_name, ".", 1) != 0) {
                char sub_path[1024];
                snprintf(sub_path, 1024, "%s/%s", filepath, ent->d_name);
                get_crontab_file_by_class_name(sub_path, class_name, class_name_length, file);
            }
        }
    }
    closedir(pDir);
}
