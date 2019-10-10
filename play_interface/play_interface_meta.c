//
// Created by Leo on 2019/10/9.
//
#include "play_interface.h"
#include "../play_core/play_core.h"

typedef struct {
    char name[128]; // meta name or project path max
    struct zend_class_entry *ce;
    UT_hash_handle hh;
}play_meta_class;

play_meta_class *meta_class = NULL;
zend_class_entry *play_interface_meta_ce;

const zend_function_entry play_interface_meta_functions[] = {
        {NULL, NULL, NULL}
};

void play_interface_meta_class_register(int _module_number)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY(class, "Meta", play_interface_meta_functions);
    play_interface_meta_ce = zend_register_internal_class_ex(&class, NULL);
}
