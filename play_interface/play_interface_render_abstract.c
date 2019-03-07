//
// Created by Leo on 18/5/11.
//

#include "play_interface.h"

PHP_METHOD(Render_Abstract, __construct){}
PHP_METHOD(Render_Abstract, setHeader){}
PHP_METHOD(Render_Abstract, run){}
PHP_METHOD(Render_Abstract, exception){}

zend_class_entry *play_interface_render_abstract_ce;
const zend_function_entry play_interface_render_abstract_functions[] = {
    PHP_ME(Render_Abstract, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Render_Abstract, setHeader, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
    PHP_ME(Render_Abstract, run, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
    PHP_ME(Render_Abstract, exception, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
    {NULL, NULL, NULL}
};

void play_interface_render_abstract_register(int _module_number)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY_EX(class, "Render_Abstract", 15, play_interface_render_abstract_functions);
    play_interface_render_abstract_ce = zend_register_internal_class_ex(&class, NULL);
}