//
// Created by Leo on 18/5/11.
//

#include "play_interface.h"

PHP_METHOD(Output, get);
PHP_METHOD(Output, set);
PHP_METHOD(Output, getComment);

zend_class_entry *play_interface_output_ce;
const zend_function_entry play_interface_output_functions[] = {
    PHP_ME(Output, get, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(Output, set, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(Output, getComment, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};

void play_interface_output_register(int _module_number)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY_EX(class, "Output", 6, play_interface_output_functions);
    play_interface_output_ce = zend_register_internal_class_ex(&class, NULL);
    zend_declare_property_null(play_interface_output_ce, "_data", 5, ZEND_ACC_PRIVATE|ZEND_ACC_STATIC);
    zend_declare_property_null(play_interface_output_ce, "_comment", 8, ZEND_ACC_PRIVATE|ZEND_ACC_STATIC);
}

PHP_METHOD(Output, getComment)
{
    zval *key = NULL;
    zval *val = NULL;

#ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &key) == FAILURE) {
        return;
    }
#else
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(key)
    ZEND_PARSE_PARAMETERS_END();
#endif

    zval *_data = zend_read_static_property(play_interface_output_ce, "_comment", 8, 1);
    if (Z_TYPE_P(_data) == IS_NULL) {
        array_init(_data);
    }
    if (key == NULL) {
        RETURN_ZVAL(_data, 1, 0);
    }

    if (Z_TYPE_P(key) != IS_STRING) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "Output::get() expects parameter 1 must to be string");
        RETURN_NULL();
    }
    if ((val = zend_hash_str_find(Z_ARR_P(_data), Z_STRVAL_P(key), Z_STRLEN_P(key))) == NULL) {
        RETURN_NULL();
    }
    RETURN_ZVAL(val, 1, 0);
}


PHP_METHOD(Output, get)
{
    zval *key = NULL;
    zval *val = NULL;

#ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &key) == FAILURE) {
        return;
    }
#else
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(key)
    ZEND_PARSE_PARAMETERS_END();
#endif

    zval *_data = zend_read_static_property(play_interface_output_ce, "_data", 5, 1);
    if (Z_TYPE_P(_data) == IS_NULL) {
        array_init(_data);
    }
    if (key == NULL) {
        RETURN_ZVAL(_data, 1, 0);
    }

    if (Z_TYPE_P(key) != IS_STRING) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "Output::get() expects parameter 1 must to be string");
        RETURN_NULL();
    }
    if ((val = zend_hash_str_find(Z_ARR_P(_data), Z_STRVAL_P(key), Z_STRLEN_P(key))) == NULL) {
        RETURN_NULL();
    }
    RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(Output, set)
{
    zval *key = NULL;
    zval *val = NULL;
    zval *comment = NULL;

#ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz|z", &key, &val, &comment) == FAILURE) {
        return;
    }
#else
    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_ZVAL(key)
        Z_PARAM_ZVAL(val)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(comment)
    ZEND_PARSE_PARAMETERS_END();
#endif

    zval *_data = zend_read_static_property(play_interface_output_ce, "_data", 5, 1);
    if (Z_TYPE_P(_data) == IS_NULL) {
        array_init(_data);
    }
    zval element;
    ZVAL_ZVAL(&element, val, 1, 0);
    add_assoc_zval_ex(_data, Z_STRVAL_P(key), Z_STRLEN_P(key), &element);

    // 增加字段注释
    zval *_comment = zend_read_static_property(play_interface_output_ce, "_comment", 8, 1);
    if (Z_TYPE_P(_comment) == IS_NULL) {
        array_init(_comment);
    }

    zval commentElement;
    if (comment != NULL) {
        ZVAL_ZVAL(&commentElement, comment, 1, 0);
    } else {
        ZVAL_STRINGL(&commentElement, "", 0);
    }
    add_assoc_zval_ex(_comment, Z_STRVAL_P(key), Z_STRLEN_P(key), &commentElement);
}