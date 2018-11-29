//
// Created by Leo on 18/5/11.
//

#include "play_interface.h"

PHP_METHOD(Output, get);
PHP_METHOD(Output, set);

zend_class_entry *play_interface_output_ce;
const zend_function_entry play_interface_output_functions[] = {
    PHP_ME(Output, get, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(Output, set, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};

void play_interface_output_register(int _module_number)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY_EX(class, "Output", 6, play_interface_output_functions);
    play_interface_output_ce = zend_register_internal_class_ex(&class, NULL);
    zend_declare_property_null(play_interface_output_ce, "_data", 5, ZEND_ACC_PRIVATE|ZEND_ACC_STATIC);
}

PHP_METHOD(Output, get)
{
    zval *key = NULL;
    zval *val = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &key) == FAILURE) {
        return;
    }

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

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &key, &val) == FAILURE) {
        return;
    }

    zval *_data = zend_read_static_property(play_interface_output_ce, "_data", 5, 1);
    if (Z_TYPE_P(_data) == IS_NULL) {
        array_init(_data);
    }
    zval element;
    ZVAL_ZVAL(&element, val, 1, 0);
    add_assoc_zval_ex(_data, Z_STRVAL_P(key), Z_STRLEN_P(key), &element);
}