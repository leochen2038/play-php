//
// Created by Leo on 18/5/14.
//

#include "play_interface.h"

PHP_METHOD(Input, getInPost);
PHP_METHOD(Input, getInHeader);
PHP_METHOD(Input, getInGet);
PHP_METHOD(Input, getInCookie);
PHP_METHOD(Input, getInRequest);


zend_class_entry *play_interface_input_ce;
static zval *play_interface_input_get_param(zval *track, zval *key, zval *defaultVal, zval *regex, char *track_name, char *method_name);

const zend_function_entry play_interface_input_functions[] = {
        PHP_ME(Input, getInPost, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
        PHP_ME(Input, getInHeader, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
        PHP_ME(Input, getInGet, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
        PHP_ME(Input, getInCookie, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
        PHP_ME(Input, getInRequest, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
        {NULL, NULL, NULL}
};

void play_interface_input_register(int _module_number)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY_EX(class, "Input", 5, play_interface_input_functions);
    play_interface_input_ce = zend_register_internal_class_ex(&class, NULL);
}

PHP_METHOD(Input, getInRequest)
{
    zval *key = NULL;
    zval *regex = NULL;
    zval *defaultVal = NULL;
    zval *val = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|zzz", &key, &regex, &defaultVal) == FAILURE) {
        return;
    }

    if (key == NULL) {
        RETURN_ZVAL(&PG(http_globals)[TRACK_VARS_REQUEST], 1, 0);
    }

    if (Z_TYPE_P(key) != IS_STRING) {
        play_interface_utils_trigger_exception(0x01, "Input::getInRequest() expects parameter 1 must to be string");
        RETURN_NULL();
    }

    val = play_interface_input_get_param(&PG(http_globals)[TRACK_VARS_REQUEST], key, defaultVal, regex, "raw", "Input::getInRequest()");
    if (val == NULL) {
        RETURN_NULL();
    }
    RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(Input, getInGet)
{
    zval *key = NULL;
    zval *regex = NULL;
    zval *defaultVal = NULL;
    zval *val = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|zzz", &key, &regex, &defaultVal) == FAILURE) {
        return;
    }
    if (key == NULL) {
        RETURN_ZVAL(&PG(http_globals)[TRACK_VARS_GET], 1, 0);
    }

    if (Z_TYPE_P(key) != IS_STRING) {
        play_interface_utils_trigger_exception(0x01, "Input::getInGet() expects parameter 1 must to be string");
        RETURN_NULL();
    }

    val = play_interface_input_get_param(&PG(http_globals)[TRACK_VARS_GET], key, defaultVal, regex, "get", "Input::getInGet()");
    if (val == NULL) {
        RETURN_NULL();
    }
    RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(Input, getInPost)
{
    zval *key = NULL;
    zval *regex = NULL;
    zval *defaultVal = NULL;
    zval *val = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|zzz", &key, &regex, &defaultVal) == FAILURE) {
        return;
    }
    if (key == NULL) {
        RETURN_ZVAL(&PG(http_globals)[TRACK_VARS_POST], 1, 0);
    }

    if (Z_TYPE_P(key) != IS_STRING) {
        play_interface_utils_trigger_exception(0x01, "Input::getInPost() expects parameter 1 must to be string");
        RETURN_NULL();
    }

    val = play_interface_input_get_param(&PG(http_globals)[TRACK_VARS_POST], key, defaultVal, regex, "post", "Input::getInPost()");
    if (val == NULL) {
        RETURN_NULL();
    }
    RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(Input, getInCookie)
{
    zval *key = NULL;
    zval *regex = NULL;
    zval *defaultVal = NULL;
    zval *val = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|zzz", &key, &regex, &defaultVal) == FAILURE) {
        return;
    }
    if (key == NULL) {
        RETURN_ZVAL(&PG(http_globals)[TRACK_VARS_COOKIE], 1, 0);
    }

    if (Z_TYPE_P(key) != IS_STRING) {
        play_interface_utils_trigger_exception(0x01, "Input::getInCookie() expects parameter 1 must to be string");
        RETURN_NULL();
    }

    val = play_interface_input_get_param(&PG(http_globals)[TRACK_VARS_COOKIE], key, defaultVal, regex, "cookie", "Input::getInCookie()");
    if (val == NULL) {
        RETURN_NULL();
    }
    RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(Input, getInHeader)
{
    int i;
    char upper[64];
    zval *key = NULL;
    zval *regex = NULL;
    zval *defaultVal = NULL;
    zval *val = NULL;
    zval upper_key;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|zzz", &key, &regex, &defaultVal) == FAILURE) {
        return;
    }
    if (key == NULL) {
        RETURN_ZVAL(&PG(http_globals)[TRACK_VARS_SERVER], 1, 0);
    }

    if (Z_TYPE_P(key) != IS_STRING) {
        play_interface_utils_trigger_exception(0x01, "Input::getInHeader() expects parameter 1 must to be string");
        RETURN_NULL();
    }

    memcpy(upper, "HTTP_", 5);
    for (i = 0 ; i < Z_STRLEN_P(key) ; i++) {
        upper[i+5] = toupper(Z_STRVAL_P(key)[i]);
    }
    upper[i+5] = 0;

    ZVAL_STRING(&upper_key, upper);
    val = play_interface_input_get_param(&PG(http_globals)[TRACK_VARS_SERVER], &upper_key, defaultVal, regex, "header", "Input::getInHeader()");
    if (val == NULL) {
        RETURN_NULL();
    }
    RETURN_ZVAL(val, 1, 0);
}

static zval *play_interface_input_get_param(zval *track, zval *key, zval *defaultVal, zval *regex, char *track_name, char *method_name)
{
    zval *val = NULL;
    zval *context = zend_read_static_property(play_interface_context_ce, "_data", 5, 0);
    if (Z_TYPE_P(context) != IS_ARRAY || (val = zend_hash_str_find(Z_ARR_P(context), Z_STRVAL_P(key), Z_STRLEN_P(key))) == NULL) {
        if ((val = zend_hash_str_find(Z_ARR_P(track), Z_STRVAL_P(key), Z_STRLEN_P(key))) == NULL) {
            if (defaultVal != NULL) {
                return defaultVal;
            }
            if (regex != NULL) {
                play_interface_utils_trigger_exception(0x01, "%s %s not given", track_name, Z_STRVAL_P(key));
            }
            return NULL;
        }
    }

    // 值非空
    if (regex == NULL || Z_TYPE_P(regex) == IS_NULL) {
        return val;
    }

    if (Z_TYPE_P(regex) != IS_STRING) {
        play_interface_utils_trigger_exception(0x01, "%s expects parameter 2 must to be string", method_name);
        return NULL;
    }

    if (Z_STRLEN_P(regex) != 0) {
        if (!play_interface_utils_valid(val, Z_STRVAL_P(regex))) {
            if (defaultVal != NULL) {
                return defaultVal;
            }
            play_interface_utils_trigger_exception(0x01, "input %s %s regex %s error", track_name, Z_STRVAL_P(key), Z_STRVAL_P(regex));
        }
    }
    return val;
}

