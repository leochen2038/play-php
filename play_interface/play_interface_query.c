//
// Created by Leo on 18/5/15.
//

#include <zend_interfaces.h>
#include "play_interface.h"
#include "../play_core/play_core.h"

zend_class_entry *play_interface_query_ce;

PHP_METHOD(Query, __call);
PHP_METHOD(Query, __construct);
static zval* play_interface_query_run_method(zval *obj, play_string *action, play_string *field, play_string *condition, zval *arg, zval *retval);
static int play_interface_query_check_field_type(zval *obj, play_string *field, zval* args);
int php_interface_query_parse_method(const char *method, int len, play_string *action, play_string *field, play_string *condition);

ZEND_BEGIN_ARG_INFO_EX(Query_call, 0, 0, 1)
    ZEND_ARG_INFO(0, method)
    ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()


const zend_function_entry play_interface_query_functions[] = {
    PHP_ME(Query, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(Query, __call, Query_call, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    {NULL, NULL, NULL}
};

void play_interface_query_register(int _module_number)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY(class, "Query", play_interface_query_functions);
    play_interface_query_ce = zend_register_internal_class_ex(&class, NULL);

    zend_declare_property_null(play_interface_query_ce, "module", 6, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR);
    zend_declare_property_null(play_interface_query_ce, "name", 4, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR);
    zend_declare_property_null(play_interface_query_ce, "database", 8, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR);
    zend_declare_property_null(play_interface_query_ce, "table", 5, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR);
    zend_declare_property_null(play_interface_query_ce, "where", 5, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR);
    zend_declare_property_null(play_interface_query_ce, "or", 2, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR);
    zend_declare_property_null(play_interface_query_ce, "field", 5, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR);
    zend_declare_property_null(play_interface_query_ce, "select", 6, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR);
    zend_declare_property_null(play_interface_query_ce, "order", 5, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR);
    zend_declare_property_null(play_interface_query_ce, "limit", 5, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR);
    zend_declare_property_null(play_interface_query_ce, "fields", 6, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR);
    zend_declare_property_null(play_interface_query_ce, "_router", 7, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR);
    zend_declare_property_stringl(play_interface_query_ce, "metaName", 8, "", 0, ZEND_ACC_PRIVATE|ZEND_ACC_CTOR);
}

PHP_METHOD(Query, __construct)
{
    zval _where, _or, _field, _fields;

    array_init(&_where);
    array_init(&_or);
    array_init(&_field);
    array_init(&_fields);

    zend_update_property(play_interface_query_ce, getThis(), ZEND_STRL("where"), &_where);
    zend_update_property(play_interface_query_ce, getThis(), ZEND_STRL("or"), &_or);
    zend_update_property(play_interface_query_ce, getThis(), ZEND_STRL("field"), &_field);
    zend_update_property(play_interface_query_ce, getThis(), ZEND_STRL("fields"), &_fields);

    zval_ptr_dtor(&_where);
    zval_ptr_dtor(&_or);
    zval_ptr_dtor(&_field);
    zval_ptr_dtor(&_fields);
}

PHP_METHOD(Query, __call)
{
    zval *arg;
    zval *method;
    play_string *action = play_string_new_with_size(63);
    play_string *field = play_string_new_with_size(63);
    play_string *condition = play_string_new_with_size(63);

#ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &method, &arg) == FAILURE) {
        return;;
    }
#else
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(method)
        Z_PARAM_ZVAL(arg)
    ZEND_PARSE_PARAMETERS_END();
#endif

    int i;
    if ( (i = php_interface_query_parse_method(Z_STRVAL_P(method), Z_STRLEN_P(method), action, field, condition)) == 0) {
        play_interface_utils_trigger_exception(0x00, "Call to undefined method Query::%s()", Z_STRVAL_P(method));
        RETURN_NULL();
    }
    if (i > 1 && zend_hash_num_elements(Z_ARRVAL_P(arg)) < 1) {
        play_interface_utils_trigger_exception(0x00, "Query::%s() expects exactly 1 parameters, 0 given", Z_STRVAL_P(method));
        RETURN_NULL();
    }
    zval *result = play_interface_query_run_method(getThis(), action, field, condition, arg, return_value);
    if (result != return_value) {
        RETURN_ZVAL(result, 1, 0);
    }
}

void play_interface_query_clean_property(zval *obj)
{

}

int run_static_router_method(char *class_lowercase, int class_lowercase_len, const char *method, zval *retval, int param_count, zval *obj, zval *args)
{
    zend_class_entry *ce = NULL;
    if ((ce = zend_hash_str_find_ptr(EG(class_table), class_lowercase, class_lowercase_len)) == NULL) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not find class %s\n", class_lowercase);
        return -1;
    }
    if (param_count == 1) {
        zend_call_method(NULL, ce, NULL, method, strlen(method), retval, 1, obj, NULL);
    } else if (param_count == 2) {
        zend_call_method(NULL, ce, NULL, method, strlen(method), retval, 2, obj, args);
    } else {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "database method param count must less or equal 2 %s:%s\n", class_lowercase, method);
        return -1;
    }
    return 0;
}

static zval *play_interface_query_run_method(zval *obj, play_string *action, play_string *field, play_string *condition, zval *arg, zval *retval)
{
    if (memcmp(action->val, "set", 3) == 0) {
        if (play_interface_query_check_field_type(obj, field, arg)) {
            zval data;
            ZVAL_ZVAL(&data, arg, 1, 0);
            zval *_property = zend_read_property(play_interface_query_ce, obj, "field", 5, 1, NULL);

            zval *zfn = NULL;
            if ((zfn = zend_read_property(play_interface_query_ce, obj, field->val, field->len, 1, NULL)) == NULL) {
                zval *metaName = zend_read_property(Z_OBJCE_P(obj), obj, "metaName", 8, 1, NULL);
                play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not find field %s in meta %s", field->val, Z_STRVAL_P(metaName));
                return 0;
            }
            add_assoc_zval_ex(_property,  Z_STRVAL_P(zfn), Z_STRLEN_P(zfn), &data);
        }
        return obj;
    } else if (memcmp(action->val, "where", 5) == 0) {
        zval *keyptr;
        zval *zfn = NULL;
        if ((zfn = zend_read_property(play_interface_query_ce, obj, field->val, field->len, 1, NULL)) == NULL) {
            zval *metaName = zend_read_property(Z_OBJCE_P(obj), obj, "metaName", 8, 1, NULL);
            play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not find field %s in meta %s", field->val, Z_STRVAL_P(metaName));
            return 0;
        }
        zval *_property = zend_read_property(play_interface_query_ce, obj, "where", 5, 1, NULL);
        if (condition->len > 0 && zend_read_property(play_interface_query_ce, obj, Z_STRVAL_P(zfn), Z_STRLEN_P(zfn), 1, NULL) != NULL ) {
            zval data;
            ZVAL_ZVAL(&data, arg, 1, 0);
            if ((keyptr = zend_hash_str_find(Z_ARRVAL_P(_property), Z_STRVAL_P(zfn), Z_STRLEN_P(zfn))) == NULL) {
                zval key;
                array_init(&key);
                add_assoc_zval_ex(_property, Z_STRVAL_P(zfn), Z_STRLEN_P(zfn), &key);
                add_assoc_zval_ex(&key, condition->val, condition->len, &data);
            } else {
                add_assoc_zval_ex(keyptr, condition->val, condition->len, &data);
            }
        }
        return obj;
    } else if (action->len == 2 && memcmp(action->val, "or", 2) == 0) {
        zval *keyptr = NULL;
        zval *zfn = NULL;
        if ((zfn = zend_read_property(play_interface_query_ce, obj, field->val, field->len, 1, NULL)) == NULL) {
            zval *metaName = zend_read_property(Z_OBJCE_P(obj), obj, "metaName", 8, 1, NULL);
            play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not find field %s in meta %s", field->val, Z_STRVAL_P(metaName));
            return 0;
        }
        zval *_property = zend_read_property(play_interface_query_ce, obj, "or", 2, 1, NULL);
        if (condition->len > 0 && zend_read_property(play_interface_query_ce, obj, Z_STRVAL_P(zfn), Z_STRLEN_P(zfn), 1, NULL) != NULL ) {
            zval data;
            ZVAL_ZVAL(&data, arg, 1, 0);
            if ((keyptr = zend_hash_str_find(Z_ARRVAL_P(_property), Z_STRVAL_P(zfn), Z_STRLEN_P(zfn))) == NULL) {
                zval key;
                array_init(&key);
                add_assoc_zval_ex(_property, Z_STRVAL_P(zfn), Z_STRLEN_P(zfn), &key);
                add_assoc_zval_ex(&key, condition->val, condition->len, &data);
            } else {
                add_assoc_zval_ex(keyptr, condition->val, condition->len, &data);
            }
        }
        return obj;
    } else if (memcmp(action->val, "getList", 7) == 0) {
        zend_update_property(play_interface_query_ce, obj, "select", 6, arg);
        zval *router = zend_read_property(play_interface_query_ce, obj, "_router", 7, 1, NULL);
        if (Z_TYPE_P(router) == IS_STRING) {
            run_static_router_method(Z_STRVAL_P(router), Z_STRLEN_P(router), "getlist", retval, 1, obj, NULL);
        } else {
            zend_call_method_with_1_params(router, Z_OBJCE_P(router), NULL, "getlist", retval, obj);
        }
        play_interface_query_clean_property(obj);
        return retval;
    } else if (memcmp(action->val, "update", 6) == 0) {
        zval *router = zend_read_property(play_interface_query_ce, obj, "_router", 7, 1, NULL);
        if (Z_TYPE_P(router) == IS_STRING) {
            run_static_router_method(Z_STRVAL_P(router), Z_STRLEN_P(router), "update", retval, 1, obj, NULL);
        } else {
            zend_call_method_with_1_params(router, Z_OBJCE_P(router), NULL, "update", retval, obj);
        }
        play_interface_query_clean_property(obj);
        return retval;
    } else if (memcmp(action->val, "delete", 6) == 0) {
        zval *router = zend_read_property(play_interface_query_ce, obj, "_router", 7, 1, NULL);
        if (Z_TYPE_P(router) == IS_STRING) {
            run_static_router_method(Z_STRVAL_P(router), Z_STRLEN_P(router), "delete", retval, 1, obj, NULL);
        } else {
            zend_call_method_with_1_params(router, Z_OBJCE_P(router), NULL, "delete", retval, obj);
        }
        play_interface_query_clean_property(obj);
        return retval;
    } else if (memcmp(action->val, "count", 5) == 0) {
        zval *router = zend_read_property(play_interface_query_ce, obj, "_router", 7, 1, NULL);
        if (Z_TYPE_P(router) == IS_STRING) {
            run_static_router_method(Z_STRVAL_P(router), Z_STRLEN_P(router), "count", retval, 1, obj, NULL);
        } else {
            zend_call_method_with_1_params(router, Z_OBJCE_P(router), NULL, "count", retval, obj);
        }
        play_interface_query_clean_property(obj);
        return retval;
    } else if (memcmp(action->val, "insert", 6) == 0) {
        zval *router = zend_read_property(play_interface_query_ce, obj, "_router", 7, 1, NULL);
        if (Z_TYPE_P(router) == IS_STRING) {
            run_static_router_method(Z_STRVAL_P(router), Z_STRLEN_P(router), "insert", retval, 1, obj, NULL);
        } else {
            zend_call_method_with_1_params(router, Z_OBJCE_P(router), NULL, "insert", retval, obj);
        }
        play_interface_query_clean_property(obj);
        return retval;
    } else if (memcmp(action->val, "getOne", 6) == 0) {
        int count = 0;
        zval *_where = zend_read_property(play_interface_query_ce, obj, "where", 5, 1, NULL);
        zval *_metaName = zend_read_property(play_interface_query_ce, obj, "metaName", 8, 1, NULL);
        count = zend_hash_num_elements(Z_ARRVAL_P(_where));
        if (count == 0) {
            play_meta *meta = play_manager_meta_get_by_chars(Z_STRVAL_P(_metaName), 0);
            object_init_ex(retval, meta->ce);
        } else {
            zval *router = zend_read_property(play_interface_query_ce, obj, "_router", 7, 1, NULL);
            if (Z_TYPE_P(router) == IS_STRING) {
                run_static_router_method(Z_STRVAL_P(router), Z_STRLEN_P(router), "getone", retval, 1, obj, NULL);
            } else {
                zend_call_method_with_1_params(router, Z_OBJCE_P(router), NULL, "getone", retval, obj);
            }
            play_interface_query_clean_property(obj);
        }
        return retval;
    } else if (memcmp(action->val, "orderBy", 7) == 0) {
        zend_update_property(play_interface_query_ce, obj, "order", 5, arg);
        return obj;
    } else if (memcmp(action->val, "limit", 5) == 0) {
        zend_update_property(play_interface_query_ce, obj, "limit", 5, arg);
        return obj;
    } else {
        char funName[action->len];
        play_str_tolower_copy(funName, action->val, action->len);
        zval *router = zend_read_property(play_interface_query_ce, obj, "_router", 7, 1, NULL);
        if (Z_TYPE_P(router) == IS_STRING) {
            run_static_router_method(Z_STRVAL_P(router), Z_STRLEN_P(router), funName, retval, 2, obj, arg);
        } else {
            zend_call_method(router, Z_OBJCE_P(router), NULL, funName, action->len, retval, 2, obj, arg);
        }
        play_interface_query_clean_property(obj);
        return retval;
    }
    return obj;
}

// 返回0，表示错误，返回1表示只有有action,
int php_interface_query_parse_method(const char *method, int len, play_string *action, play_string *field, play_string *condition)
{
    // 获取 action;
    int actionIndex;
    for (actionIndex = 0; actionIndex < len; actionIndex++) {
        if (memcmp(method+actionIndex, "_", 1) != 0) {
            action->len++;
        } else {
            break;
        }
    }
    memcpy(action->val, method, actionIndex);
    if (action->len == 0) {
        return 0;
    }

    if (actionIndex == len) {
        return 1;
    }

    int conditionIndex;
    for (conditionIndex = len-1; conditionIndex > actionIndex; conditionIndex--) {
        //if(!isupper(method[conditionIndex])) {
        if (memcmp(method+conditionIndex, "_", 1) != 0) {
            condition->len++;
        } else {
            break;
        }
    }

    if (conditionIndex == len-1) {
        return 0;
    }

    field->len = conditionIndex - actionIndex - 1;

    if (field->len < 1) {
        field->len = condition->len;
        memcpy(field->val, method+conditionIndex+1, field->len);
        condition->len = 0;
        return 2;
    }

    memcpy(condition->val, method+conditionIndex+1, condition->len);
    memcpy(field->val, method+actionIndex+1, field->len);
    return 3;
}

/**
 * 检查传入的值是否合法
 * @param z_type
 * @param field
 * @param arg_1
 * @param arg_2
 * @return
 */
static int play_interface_query_check_field_zvtype(int z_type, play_string *field, zval *arg_1, zval *arg_2)
{
    if (Z_TYPE_P(arg_1) == z_type && arg_2 == NULL) {
        return 1;
    }

    if (Z_TYPE_P(arg_1) == z_type) {
        if (Z_STRLEN_P(arg_2) == 2 && memcmp(Z_STRVAL_P(arg_2), "@+", 2) == 0) {
            return 1;
        } else if (Z_STRLEN_P(arg_2) == 2 && memcmp(Z_STRVAL_P(arg_2), "@-", 2) == 0) {
            return 1;
        } else {
            play_interface_utils_trigger_exception(PLAY_ERR_BASE, "Query::set_%s() expects parameter 2 must to be @+ or @-, %s given in", field->val, Z_STRVAL_P(arg_2));
        }
    }

    play_interface_utils_trigger_exception(PLAY_ERR_BASE, "Query::set_%s() expects parameter 1 must be %s, %s given in", field->val, play_interface_utils_get_zval_type_name(z_type), play_interface_utils_get_zval_type_name(Z_TYPE_P(arg_1)));
    return 0;
}

/**
 * 检查数组中的类型是否符合xml里的定义
 * @param z_type
 * @param field
 * @param arg_1
 * @param arg_2
 * @return
 */
static int play_interface_query_check_field_array_zvtype(int z_type, play_string *field, zval *arg_1, zval *arg_2)
{
    if (arg_2 != NULL) {
        if (z_type != IS_UNDEF && Z_TYPE_P(arg_1) != z_type && Z_TYPE_P(arg_1) != IS_ARRAY) {
            if (z_type == IS_ARRAY) {
                play_interface_utils_trigger_exception(PLAY_ERR_BASE, "Query::set_%s() expects parameter 1 must to be array", field->val);
            } else {
                play_interface_utils_trigger_exception(PLAY_ERR_BASE, "Query::set_%s() expects parameter 1 must to be array or %s", field->val, play_interface_utils_get_zval_type_name(z_type));
            }
            return 0;
        }
        if (Z_STRLEN_P(arg_2) == 1 && memcmp(Z_STRVAL_P(arg_2), "&", 1) == 0) {
            return 1;
        } else if (Z_STRLEN_P(arg_2) == 2 && memcmp(Z_STRVAL_P(arg_2), "@+", 2) == 0) {
            return 1;
        } else if (Z_STRLEN_P(arg_2) == 2 && memcmp(Z_STRVAL_P(arg_2), "@-", 2) == 0) {
            return 1;
        }
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "Query::set_%s() expects parameter 2 must to be &, @+ or @-, %s given in", field->val, Z_STRVAL_P(arg_2));
        return 0;
    }

    if (Z_TYPE_P(arg_1) != IS_ARRAY) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "Query::set_%s() expects parameter 1 must to be array", field->val);
        return 0;
    }
    return 1;
}

/**
 * 检查用户输入的值是否合法
 * @param obj
 * @param field
 * @param args
 * @return
 */
static int play_interface_query_check_field_type(zval *obj, play_string *field, zval* args)
{
    zval *zfn = NULL;
    zval *f_item = NULL;
    zval *t_item = NULL;
    zval *arg_1 = NULL;
    zval *arg_2 = NULL;

    if ((zfn = zend_read_property(play_interface_query_ce, obj, field->val, field->len, 1, NULL)) == NULL) {
        zval *metaName = zend_read_property(Z_OBJCE_P(obj), obj, "metaName", 8, 1, NULL);
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not find field %s in meta %s", field->val, Z_STRVAL_P(metaName));
        return 0;
    }
    zval *_fields = zend_read_property(play_interface_query_ce, obj, "fields", 6, 0, NULL);
    if ((f_item = zend_hash_str_find(Z_ARRVAL_P(_fields), Z_STRVAL_P(zfn), Z_STRLEN_P(zfn))) == NULL) {
        zval *metaName = zend_read_property(Z_OBJCE_P(obj), obj, "metaName", 8, 1, NULL);
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not find field %s in meta %s", field->val, Z_STRVAL_P(metaName));
        return 0;
    }

    if ((t_item = zend_hash_str_find(Z_ARRVAL_P(f_item), "type", 4)) == NULL) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not find %s type in meta", field->val);
        return 0;
    }

    if (zend_hash_num_elements(Z_ARRVAL_P(args)) == 2) {
        zend_hash_internal_pointer_reset(Z_ARRVAL_P(args));
        arg_1 = zend_hash_get_current_data(Z_ARRVAL_P(args));
        zend_hash_move_forward(Z_ARRVAL_P(args));
        arg_2 = zend_hash_get_current_data(Z_ARRVAL_P(args));
    } else {
        zend_hash_internal_pointer_reset(Z_ARRVAL_P(args));
        arg_1 = zend_hash_get_current_data(Z_ARRVAL_P(args));
    }

    switch (Z_STRLEN_P(t_item)) {
        case 3: {
            if (memcmp(Z_STRVAL_P(t_item), "int", 3) == 0 ) {
                return play_interface_query_check_field_zvtype(IS_LONG, field, arg_1, arg_2);
            }
        } break;
        case 5: {
            if (memcmp(Z_STRVAL_P(t_item), "float", 5) == 0) {
                return play_interface_query_check_field_zvtype(IS_DOUBLE, field, arg_1, arg_2);
            } else if (memcmp(Z_STRVAL_P(t_item), "array", 5) == 0) {
                return play_interface_query_check_field_array_zvtype(IS_UNDEF, field, arg_1, arg_2);
            }
        } break;
        case 6: {
            if (memcmp(Z_STRVAL_P(t_item), "string", 6) == 0) {
                return arg_2 == NULL && Z_TYPE_P(arg_1) == IS_STRING ? 1 : 0;
            }
        } break;
        case 9: {
            if (memcmp(Z_STRVAL_P(t_item), "array:int", 9) == 0) {
                return play_interface_query_check_field_array_zvtype(IS_LONG, field, arg_1, arg_2);
            }
        } break;
        case 11: {
            if (memcmp(Z_STRVAL_P(t_item), "array:float", 11) == 0) {
                return play_interface_query_check_field_array_zvtype(IS_DOUBLE, field, arg_1, arg_2);
            } else if (memcmp(Z_STRVAL_P(t_item), "array:array", 11) == 0) {
                return play_interface_query_check_field_array_zvtype(IS_ARRAY, field, arg_1, arg_2);
            }
        } break;
        case 12: {
            if (memcmp(Z_STRVAL_P(t_item), "array:string", 12) == 0) {
                return play_interface_query_check_field_array_zvtype(IS_STRING, field, arg_1, arg_2);
            }
        } break;
    }
    play_interface_utils_trigger_exception(PLAY_ERR_BASE, "Query::%s unknow type %s", field->val, Z_STRVAL_P(t_item));
    return 0;
}