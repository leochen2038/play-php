//
//  play_global_config.c
//  test
//
//  Created by Leo on 18/1/4.
//  Copyright © 2018年 Leo. All rights reserved.
//

#include "play_core.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

global_config gconfig;

void play_gloabl_config_init()
{
    gconfig.app_root_length = 0;
    gconfig.app_root[0] = 0;

    gconfig.action_root[0] = 0;
    gconfig.action_root_length = 0;

    gconfig.meta_root[0] = 0;
    gconfig.meta_root_length = 0;

    gconfig.server_root[0] = 0;
    gconfig.server_root_length = 0;

    gconfig.class_service_path[0] = 0;
    gconfig.class_service_path_length = 0;
}

int play_global_config_set_app_root(const char *app_root, int app_root_leng)
{
    if (app_root_leng > 512) {
        return 1;
    }
    gconfig.app_root_length = app_root_leng;
    gconfig.app_root[app_root_leng] = 0;
    memcpy(gconfig.app_root, app_root, app_root_leng);
    snprintf(gconfig.app_root_ex, 511, "%s/", app_root);
    snprintf(gconfig.action_root, 511, "%s/assets/action", app_root);
    snprintf(gconfig.meta_root, 511, "%s/assets/meta", app_root);
    snprintf(gconfig.server_root, 511, "%s/assets/service", app_root);
    snprintf(gconfig.class_service_path, 511, "%s/source/services", app_root);

    gconfig.server_root_length = strlen(gconfig.server_root);
    gconfig.meta_root_length = strlen(gconfig.meta_root);
    gconfig.action_root_length = strlen(gconfig.action_root);
    gconfig.class_service_path_length = strlen(gconfig.class_service_path);

    return 0;
}

char *play_global_config_error_message(int errcode)
{
    switch (errcode) {
        case 1: return "set app_root length must not bigger then 255";
        case 2: return "set local_ip length must not bigger then 16";
        case 3: return "error ip format";
    }
    return NULL;
}