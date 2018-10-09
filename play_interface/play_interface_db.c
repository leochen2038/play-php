//
// Created by Leo on 18/5/15.
//

#include <zend_interfaces.h>
#include "play_interface.h"
#include "../play_core/play_core.h"

zend_class_entry *play_interface_db_ce;

PHP_METHOD(DB, __callStatic);
ZEND_BEGIN_ARG_INFO_EX(DB_callStatic, 0, 0, 1)
    ZEND_ARG_INFO(0, method)
    ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

const zend_function_entry play_interface_db_functions[] = {
        PHP_ME(DB, __callStatic, DB_callStatic, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
        {NULL, NULL, NULL}
};

void play_interface_db_register(int _module_number)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY(class, "DB", play_interface_db_functions);
    play_interface_db_ce = zend_register_internal_class_ex(&class, NULL);
}


PHP_METHOD(DB, __callStatic)
{
    zval *method;
    zval *arg;
    zend_class_entry *ce = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &method, &arg) == FAILURE) {
        return;
    }

    int checknew = play_interface_play_checknew();
    play_meta *meta = play_manager_meta_get_by_chars(Z_STRVAL_P(method)+4, checknew);
    if (meta == NULL) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not find meta %s\n", Z_STRVAL_P(method)+4);
        RETURN_NULL();
    }

    char classPath[1024];
    char *class_lowercase;
    snprintf(classPath, 1024, "%s/database/%s.php", gconfig.app_root, meta->storage->router->val);

    class_lowercase = zend_str_tolower_dup(meta->storage->router->val, meta->storage->router->len);
    if ((ce = zend_hash_str_find_ptr(EG(class_table), class_lowercase, meta->storage->router->len)) == NULL) {
        if (!play_interface_utils_loader_import(classPath)) {
            efree(class_lowercase);
            play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not load database %s.php\n", meta->storage->router->val);
            RETURN_NULL();
        }

        if ((ce = zend_hash_str_find_ptr(EG(class_table), class_lowercase, meta->storage->router->len)) == NULL) {
            efree(class_lowercase);
            play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not find class %s in %s.php\n", meta->storage->router->val, meta->storage->router->val);
            RETURN_NULL();
        }
    }


    object_init_ex(return_value, play_interface_query_ce);
    zend_call_method_with_0_params(return_value, play_interface_query_ce, NULL, "__construct", NULL);
    zend_update_property_stringl(play_interface_query_ce, return_value, "module", 6, meta->module->val, meta->module->len);
    zend_update_property_stringl(play_interface_query_ce, return_value, "name", 4, meta->name->val,  meta->name->len);
    zend_update_property_stringl(play_interface_query_ce, return_value, "metaName", 8, Z_STRVAL_P(method)+4, Z_STRLEN_P(method) - 4);

    if(memcmp(meta->storage->type->val, "static", 6) == 0) {
        zend_update_property_stringl(play_interface_query_ce, return_value, "_router", 7, class_lowercase, meta->storage->router->len);
    } else {
        zval router;
        object_init_ex(&router, ce);
        zend_update_property(play_interface_query_ce, return_value, "_router", 7, &router);
        zend_call_method_with_1_params(&router, ce, NULL, "__construct", NULL, return_value);
        zval_ptr_dtor(&router);
    }

    // 添加 fields 属性
    zval *_property = zend_read_property(play_interface_query_ce, return_value, "fields", 6, 0, NULL);

    zval _key;
    array_init(&_key);
    add_assoc_stringl_ex(&_key, "type", 4, meta->key->type->val, meta->key->type->len);
    add_assoc_null_ex(&_key, "default", 7);
    add_assoc_stringl_ex(&_key, "funcName", 8, meta->key->funcName->val, meta->key->funcName->len);
    add_assoc_zval_ex(_property, meta->key->name->val, meta->key->name->len, &_key);
    zend_update_property_stringl(play_interface_query_ce, return_value, meta->key->name->val, meta->key->name->len, meta->key->name->val, meta->key->name->len);

    play_meta_field *field = meta->fields;
    while (field) {
        zval _field;
        array_init(&_field);
        add_assoc_stringl_ex(&_field, "type", 4, field->type->val, field->type->len);
        add_assoc_stringl_ex(&_field, "default", 7, field->defv->val, field->defv->len);
        add_assoc_stringl_ex(&_field, "funcName", 8, field->funcName->val, field->funcName->len);
        add_assoc_zval_ex(_property, field->name->val, field->name->len, &_field);
        zend_update_property_stringl(play_interface_query_ce, return_value, field->name->val, field->name->len, field->name->val, field->name->len);
        field = field->next;
    }

    efree(class_lowercase);
}