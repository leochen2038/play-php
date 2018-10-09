//
// Created by Leo on 18/5/11.
//

#include "play_interface.h"

zend_class_entry *play_interface_processor_ce;
PHP_METHOD(Processor,__construct){}
PHP_METHOD(Processor, run){}

const zend_function_entry play_interface_processor_functions[] = {
        PHP_ME(Processor, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
        PHP_ME(Processor, run, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
        {NULL, NULL, NULL}
};

void play_interface_processor_register(int _module_number)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY_EX(class, "Processor", 9, play_interface_processor_functions);
    play_interface_processor_ce = zend_register_internal_class_ex(&class, NULL);
}
