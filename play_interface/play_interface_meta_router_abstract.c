//
// Created by Leo on 18/5/16.
//

#include "play_interface.h"

PHP_METHOD(Meta_Router_Abstract, getList);
PHP_METHOD(Meta_Router_Abstract, getOne);
PHP_METHOD(Meta_Router_Abstract, insert);
PHP_METHOD(Meta_Router_Abstract, count);
PHP_METHOD(Meta_Router_Abstract, update);
PHP_METHOD(Meta_Router_Abstract, delete);
PHP_METHOD(Meta_Router_Abstract, exec);
PHP_METHOD(Meta_Router_Abstract, bb);

zend_class_entry *play_interface_meta_router_abstract_ce;
ZEND_BEGIN_ARG_INFO_EX(Meta_Router_Abstract_getList, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, query, Query, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Meta_Router_Abstract_getOne, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, query, Query, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Meta_Router_Abstract_insert, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, query, Query, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Meta_Router_Abstract_count, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, query, Query, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Meta_Router_Abstract_update, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, query, Query, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Meta_Router_Abstract_delete, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, query, Query, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Meta_Router_Abstract_exec, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, query, Query, 0)
ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()


const zend_function_entry play_interface_meta_router_abstract_functions[] = {
    PHP_ME(Meta_Router_Abstract, getList, Meta_Router_Abstract_getList, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
    PHP_ME(Meta_Router_Abstract, getOne, Meta_Router_Abstract_getOne, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
    PHP_ME(Meta_Router_Abstract, insert, Meta_Router_Abstract_insert, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
    PHP_ME(Meta_Router_Abstract, count, Meta_Router_Abstract_count, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
    PHP_ME(Meta_Router_Abstract, update, Meta_Router_Abstract_update, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
    PHP_ME(Meta_Router_Abstract, delete, Meta_Router_Abstract_delete, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
    PHP_ME(Meta_Router_Abstract, exec, Meta_Router_Abstract_exec, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
    {NULL, NULL, NULL}
};

void play_interface_meta_router_abstract_register(int _module_number)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY(class, "Meta_Router_Abstract", play_interface_meta_router_abstract_functions);
    play_interface_meta_router_abstract_ce = zend_register_internal_class_ex(&class, NULL);
}

PHP_METHOD(Meta_Router_Abstract, getList){}
PHP_METHOD(Meta_Router_Abstract, getOne){}
PHP_METHOD(Meta_Router_Abstract, insert){}
PHP_METHOD(Meta_Router_Abstract, count){}
PHP_METHOD(Meta_Router_Abstract, update){}
PHP_METHOD(Meta_Router_Abstract, delete){}
PHP_METHOD(Meta_Router_Abstract, exec){}