//
// Created by Leo on 18/5/9.
//

#include "../play_core/play_core.h"
#include "play_interface.h"
#include "zend_interfaces.h"

static int module_number;

PHP_METHOD(Play, init);
PHP_METHOD(Play, reconst);
PHP_METHOD(Play, crontab);
PHP_METHOD(Play, service);

zend_class_entry *play_interface_play_ce;
const zend_function_entry play_interface_play_register_functions[] = {
    PHP_ME(Play, init, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(Play, reconst, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(Play, crontab, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};

void play_interface_play_register(int _module_number)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY_EX(class, "Play", 4, play_interface_play_register_functions);
    play_interface_play_ce = zend_register_internal_class_ex(&class, NULL);

    zend_declare_property_bool(play_interface_play_ce, "auto", 4, 0, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC);
    zend_declare_property_null(play_interface_play_ce, "environment", 11, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC);
    zend_declare_property_stringl(play_interface_play_ce, "render", 6, "html", 4, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC);
}

int play_interface_play_checknew()
{
    zval *pzauto = NULL;
    pzauto = zend_read_static_property(play_interface_play_ce, "auto", 4, 1);
    return Z_TYPE_P(pzauto) == IS_TRUE ? 1 : 0;
}

PHP_METHOD(Play, init)
{
    zval *proj_name = NULL;

#ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &proj_name) == FAILURE) {
        return;
    }
#else
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(proj_name)
    ZEND_PARSE_PARAMETERS_END();
#endif

    if (proj_name != NULL) {
        play_interface_play_init(Z_STRVAL_P(proj_name));
    } else {
        play_interface_play_init(".");
    }
}

PHP_METHOD(Play, reconst)
{
    zval *proj_name = NULL;

#ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &proj_name) == FAILURE) {
        return;
    }
#else
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(proj_name)
    ZEND_PARSE_PARAMETERS_END();
#endif

    if (proj_name != NULL) {
        play_string *root = play_find_project_root_by_path(Z_STRVAL_P(proj_name), 0);
        if (root == NULL) {
            php_printf("can not find any play project in %s\n", proj_name);
            return;
        }
        play_global_config_set_app_root(root->val, root->len);
        play_string_free(root);
    }
    play_interface_play_reconst();
}

PHP_METHOD(Play, crontab)
{
    zval *root_path;
    zval *environment;
    zval crontab_list;
    char file_path[512];

#ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|z", &root_path, &environment) == FAILURE) {
        return;
    }
#else
    ZEND_PARSE_PARAMETERS_START(1, 2)
            Z_PARAM_ZVAL(root_path)
            Z_PARAM_OPTIONAL
            Z_PARAM_ZVAL(environment)
    ZEND_PARSE_PARAMETERS_END();
#endif

    time_t timep;
    time(&timep);
    struct tm *p;
    play_global_config_set_app_root(Z_STRVAL_P(root_path), Z_STRLEN_P(root_path));
    REGISTER_STRINGL_CONSTANT("APP_ROOT", gconfig.app_root_ex, gconfig.app_root_length+1, CONST_CS);
    REGISTER_LONG_CONSTANT("TIME_NOW", timep, CONST_CS);

    if (environment != NULL && Z_STRLEN_P(environment) >0 ) {
        snprintf(file_path, 512, "%s/config/%s.crontab.php", gconfig.app_root, Z_STRVAL_P(environment));
        zend_update_static_property_stringl(play_interface_play_ce, "environment", 11, Z_STRVAL_P(environment), Z_STRLEN_P(environment));
    } else {
        snprintf(file_path, 512, "%s/config/release.crontab.php", gconfig.app_root);
        zend_update_static_property_stringl(play_interface_play_ce, "environment", 11, "release", 7);
    }

    if (play_interface_utils_load_config(file_path, &crontab_list) != 1) {
        php_printf("can not beexecuted:%s!\n", file_path);
        RETURN_NULL();
    }

    p = localtime(&timep);
    if ( Z_TYPE(crontab_list)== IS_ARRAY) {
        zval *z_item = NULL;
        zend_ulong idx;
        zend_string *z_key;
        int count = zend_hash_num_elements(Z_ARRVAL(crontab_list));
        zend_hash_internal_pointer_reset(Z_ARRVAL(crontab_list));

        int i;
        zval obj;
        zval *_mutex = NULL;
        char *lower_class_name;
        zend_class_entry *ce = NULL;
        for (i = 0; i < count; i ++) {
            z_item = zend_hash_get_current_data(Z_ARRVAL(crontab_list));
            zend_hash_get_current_key(Z_ARRVAL(crontab_list), &z_key, &idx);
            snprintf(file_path, 512, "%s/source/crontab/%s.php",gconfig.app_root, z_key->val);

            if (check_crontab_activation(Z_STRVAL_P(z_item), p) == 1) {
                lower_class_name = zend_str_tolower_dup(z_key->val, z_key->len);
                if ((ce = zend_hash_str_find_ptr(EG(class_table), lower_class_name, z_key->len)) == NULL) {
                    if (!play_interface_utils_loader_import(file_path)) {
                        efree(lower_class_name);
                        php_printf("cant not load crontab file %s\n", file_path);
                        continue;
                    }

                    if ((ce = zend_hash_str_find_ptr(EG(class_table), lower_class_name, z_key->len)) == NULL) {
                        efree(lower_class_name);
                        php_printf("can not find class %s in %s\n", lower_class_name, file_path);
                        continue;
                    }
                }
                object_init_ex(&obj, ce);
                zend_call_method_with_0_params(&obj, ce, NULL, "__construct", NULL);
                _mutex = zend_read_property(ce, &obj, "_mutex", 6, 0, NULL);
                pid_t parent_pid = getpid();
                play_interface_play_crontab_run_normal_level(lower_class_name, (Z_TYPE_P(_mutex) == IS_TRUE ? 1 : 0), &obj, ce);
                efree(lower_class_name);
                zval_ptr_dtor(&obj);
                if (parent_pid != getpid()) {
                    break; // 子进程提出循环
                }
            }
            zend_hash_move_forward(Z_ARRVAL(crontab_list));
        }
    }
    zval_ptr_dtor(&crontab_list);
    int status;
    while(wait(&status)>0);
}

//PHP_METHOD(Play, crontabv1)
//{
//    zval *root_path;
//
//#ifndef FAST_ZPP
//    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &root_path) == FAILURE) {
//        return;
//    }
//#else
//    ZEND_PARSE_PARAMETERS_START(1, 1)
//        Z_PARAM_ZVAL(root_path)
//    ZEND_PARSE_PARAMETERS_END();
//#endif
//
//    time_t timep;
//    time(&timep);
//    play_global_config_set_app_root(Z_STRVAL_P(root_path), Z_STRLEN_P(root_path));
//    REGISTER_STRINGL_CONSTANT("APP_ROOT", gconfig.app_root_ex, gconfig.app_root_length+1, CONST_CS);
//    REGISTER_LONG_CONSTANT("TIME_NOW", timep, CONST_CS);
//
//    char file_path[512];
//    char *file_list[512];
//    int file_list_count = 0;
//    snprintf(file_path, 512, "%s/source/crontab", gconfig.app_root);
//    if (play_interface_utils_find_target_file(file_path, file_list, &file_list_count, "Cron_") != 0) {
//        php_printf("can not beexecuted:%s!\n", file_path);
//    }
//
//    int i = 0;
//    zval *_hit = NULL;
//    zval *_mutex = NULL;
//    zval *_es = NULL;
//    zend_class_entry *ce = NULL;
//    unsigned int class_name_length = 0;
//    char *lower_class_name;
//
//    zval obj;
//    for (i = 0; i < file_list_count; i++) {
//        class_name_length = (unsigned int)(rindex(file_list[i], '.') - rindex(file_list[i], '/')) - 1;
//        lower_class_name = zend_str_tolower_dup(rindex(file_list[i], '/')+1, class_name_length);
//        if ((ce = zend_hash_str_find_ptr(EG(class_table), lower_class_name,strlen(lower_class_name))) == NULL) {
//            if (!play_interface_utils_loader_import(file_list[i])) {
//                efree(lower_class_name);
//                php_printf("cant not load crontab file %s\n", file_list[i]);
//                continue;
//            }
//
//            if ((ce = zend_hash_str_find_ptr(EG(class_table), lower_class_name, strlen(lower_class_name))) == NULL) {
//                efree(lower_class_name);
//                php_printf("can not find class %s in %s\n", lower_class_name, file_list[i]);
//                continue;
//            }
//        }
//        object_init_ex(&obj, ce);
//        zend_call_method_with_0_params(&obj, ce, NULL, "__construct", NULL);
//        _hit = zend_read_property(ce, &obj, "_hit", 4, 0, NULL);
//        _mutex = zend_read_property(ce, &obj, "_mutex", 6, 0, NULL);
//        _es = zend_read_property(ce, &obj, "_es", 3, 0, NULL);
//
//        pid_t parent_pid = getpid();
//        if (Z_TYPE_P(_hit) == IS_TRUE) {
//            // step 4. 判断定时任务时间是否符合
//            if (Z_LVAL_P(_es) > 0 ) {
//                play_interface_play_crontab_run_second_level(lower_class_name, Z_LVAL_P(_es), &obj, ce);
//            } else {
//                play_interface_play_crontab_run_normal_level(lower_class_name, (Z_TYPE_P(_mutex) == IS_TRUE ? 1 : 0), &obj, ce);
//            }
//        }
//        efree(lower_class_name);
//        zval_ptr_dtor(&obj);
//        if (parent_pid != getpid()) {
//            break; // 子进程提出循环
//        }
//    }
//    int status;
//    while(wait(&status)>0);
//}