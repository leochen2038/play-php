//
// Created by Leo on 2018/8/28.
//

#ifndef PROJECT_PLAY_INTERFACE_H
#define PROJECT_PLAY_INTERFACE_H

#include <php.h>

#define PLAY_ERR_BASE                       128
#define PLAY_ERR_ACTION_NOT_FIND            129


// play_interface_utils.c
int play_interface_utils_valid(zval *val, char *pattern);
void play_interface_utils_trigger_exception(int type, char *format, ...);
int play_interface_utils_loader_import(char *path);
const char *play_interface_utils_get_zval_type_name(int idx);
int play_interface_utils_find_target_file(char *path, char *file_list[], int *file_list_count, char *target);
void play_interface_utils_append_crontab_mutex_file(char *lock_file);
void play_interface_utils_clean_crontab_mutex_file();


// play_interface_action.c
extern zend_class_entry *play_interface_action_ce;
void play_interface_action_register(int _module_number);


// play_interface_play.c
extern zend_class_entry *play_interface_play_ce;
void play_interface_play_register(int _module_number);
int play_interface_play_checknew();

// play_interface_play_crontab.c
void play_interface_play_crontab_run_second_level(char *lower_class_name, long es, zval *obj, zend_class_entry *ce);
void play_interface_play_crontab_run_normal_level(char *lower_class_name, long mutex, zval *obj, zend_class_entry *ce);

// play_interface_play_init.c
void play_interface_play_init(const char *proj);

// play_interface_play_reconst.c
void play_interface_play_reconst();


// play_interface_context.c
extern zend_class_entry *play_interface_context_ce;
void play_interface_context_register(int _module_number);

// play_interface_crontab.c
extern zend_class_entry *play_interface_crontab_ce;
void play_interface_crontab_register(int _module_number);

// play_interface_db.c
extern zend_class_entry *play_interface_db_ce;
void play_interface_db_register(int _module_number);


// play_interface_input.c
extern zend_class_entry *play_interface_input_ce;
void play_interface_input_register(int _module_number);

// play_interface_output.c
extern zend_class_entry *play_interface_output_ce;
void play_interface_output_register(int _module_number);

// play_interface_meta_router_abstract.c
extern zend_class_entry *play_interface_meta_router_abstract_ce;
void play_interface_meta_router_abstract_register(int _module_number);

// play_interface_processor.c
extern zend_class_entry *play_interface_processor_ce;
void play_interface_processor_register(int _module_number);

// play_interface_query.c
extern zend_class_entry *play_interface_query_ce;
void play_interface_query_register(int _module_number);

// play_interface_render_abstract.c
extern zend_class_entry *play_interface_render_abstract_ce;
void play_interface_render_abstract_register(int _module_number);

#endif //PROJECT_PLAY_INTERFACE_H