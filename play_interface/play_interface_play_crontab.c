//
// Created by Leo on 18/5/21.
//

#include "play_interface.h"
#include "zend_interfaces.h"

void play_interface_play_crontab_run_second_level(char *lower_class_name, long es, zval *obj, zend_class_entry *ce)
{
    pid_t fpid;
    fpid = fork();
    if (fpid < 0) {
        php_printf("error in fork!\n");
    } else if (fpid == 0) {
        int wait_time;
        time_t timep, time_end;
        time(&timep);
        time_end = timep + 60;
        char *lock_mutex_file = malloc(512);
        snprintf(lock_mutex_file, 512, "/tmp/%s.cron.lock", lower_class_name);
        do {
            wait_time = (int)(es - (timep % 60) % es);
            if (wait_time > 0) sleep(wait_time);
            if (access(lock_mutex_file, F_OK) != 0) {
                int status;
                fopen(lock_mutex_file, "a+");
                fpid = fork();
                if (fpid < 0) {
                    php_printf("error in fork\n");
                } else if (fpid == 0) {
                    play_interface_utils_append_crontab_mutex_file(lock_mutex_file);
                    zend_call_method_with_0_params(obj, ce, NULL, "run", NULL);
                    wait(&status);
                    zval_ptr_dtor(&obj);
                    // remove(lock_mutex_file);
                    exit(0);
                }
                wait(&status);
            }
            time(&timep);
        } while (timep + es < time_end);
    }
}


void play_interface_play_crontab_run_normal_level(char *lower_class_name, long mutex, zval *obj, zend_class_entry *ce)
{
    char *lock_mutex_file = malloc(512);
    snprintf(lock_mutex_file, 512, "/tmp/%s.cron.lock", lower_class_name);

    if (mutex == 1) {
        if (access(lock_mutex_file, F_OK) == 0) {
            php_printf("find lock file in %s!\n", lower_class_name);
            return;
        }
        fopen(lock_mutex_file, "a+");
    }

    pid_t fpid;
    fpid = fork();
    if (fpid < 0) {
        php_printf("error in fork!\n");
    } else if (fpid == 0) {
        if (mutex == 1) {
            play_interface_utils_append_crontab_mutex_file(lock_mutex_file);
        }
        zend_call_method_with_0_params(obj, ce, NULL, "run", NULL);
    }
}