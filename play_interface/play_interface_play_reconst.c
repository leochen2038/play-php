//
// Created by Leo on 18/5/9.
//

#include <stdio.h>
#include <dirent.h>
#include <zconf.h>
#include <memory.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "../play_core/play_core.h"

static void play_interface_play_reconst_create_processor(play_processor *p, int isPrint);
static int play_interface_play_reconst_create_processor_mkdir(play_processor *p);
static play_processor * play_interface_play_reconst_check_processor_loop(play_processor *p);
static void play_interface_play_reconst_create_db_api(play_meta_hashtable *ht);
static void play_interface_play_reconst_create_metas_api(play_meta_hashtable *ht);
static void play_interface_play_recons_create_condit(const char *condit, const char *field, const char *type, const char *note, play_string *src);
static void play_interface_play_reconst_check_processor(play_action_hashtable *ht);

void play_interface_play_reconst()
{
    play_interface_play_reconst_create_db_api(play_manager_meta_get_list_by_path(gconfig.meta_root));
    play_interface_play_reconst_check_processor(play_manager_action_get_list_by_path(gconfig.action_root));
    play_interface_play_reconst_create_metas_api(play_manager_meta_get_list_by_path(gconfig.meta_root));
}

static int play_interface_play_reconst_create_processor_mkdir(play_processor *p)
{
    if (p->path->len == 0) {
        return 1;
    }
    int i,j;
    char full[128];
    char name[128] = {0};
    for(i = 0,j = 0; i < p->path->len; i++) {
        if ( memcmp((p->path->val+i), "/", 1) == 0) {
            sprintf(full, "%s/source/processors/%s", gconfig.app_root, name);
            if (mkdir(full, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
                //zend_error(1, "can not create dir:%s", full);
                //return 0;
            }
        }
        name[j++] = p->path->val[i];
    }
    name[j] = 0;
    sprintf(full, "%s/source/processors/%s", gconfig.app_root, name);
    if (mkdir(full, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
        //zend_error(1, "can not create dir:%s", full);
        //return 0;
    }
    return 1;
}

static void play_interface_play_reconst_create_processor(play_processor *p, int isPrint)
{
    // step 1. 先判断文件是否存在，如果存在，则不创建
    if (access(p->file->val, 0) == 0) {
        return;
    }
    int i;
    char code[1024];

    struct timeval tval;
    time_t timep;
    struct tm *ptm;

    time(&timep);
    gettimeofday(&tval, NULL);
    ptm = localtime(&timep);

    play_string *src = play_string_new_with_size(10240);
    sprintf(code, "<?php\n\
/**\n\
 * %s.\n\
 *\n\
 * @version 1.0.0\n\
 * @author anonymous <anonymous@example.com>\n\
 * @copyright 2016-%04d The Play Framework\n\
 * @history:\n\
 *			1.0.0 | anonymous | %d-%02d-%02d %02d:%02d:%02d | initialization\n\
 */\n\
\n\
class %s extends Processor\n\
{\n", p->name->val, (1900 + ptm->tm_year), (1900 + ptm->tm_year),(ptm->tm_mon + 1), ptm->tm_mday,ptm->tm_hour, ptm->tm_min, ptm->tm_sec, p->name->val);
    play_string_append(src, code, strlen(code));
    if (p->nextCount == 0) {
        sprintf(code, "\tconst RC_NORMAL = 0x00;\n");
        play_string_append(src, code, strlen(code));
    } else {
        for (i = 0; i < p->nextCount; i++) {
            sprintf(code, "\tconst %s = 0x%02X;\n",p->next[i]->rcstring->val,i);
            play_string_append(src, code, strlen(code));
        }
    }
    sprintf(code, "\n\
    public function run()\n\
    {\n\
        // TODO\n\
    }\n\
}\n");
    play_string_append(src, code, strlen(code));
    //php_printf(">>%s,%d<<\n", p->path->val, p->path->len);
    if (play_interface_play_reconst_create_processor_mkdir(p)) {
        FILE *filefd = fopen(p->file->val, "w+");
        fwrite(src->val, src->len, 1, filefd);
        fclose(filefd);
        play_string_free(src);
        if (isPrint == 1) {
            printf("create process: %s...success\n", p->file->val);
        }

    }
}

static play_processor * play_interface_play_reconst_check_processor_loop(play_processor *p)
{
    int i;
    for (i = 0; i < p->nextCount; i++) {
        play_interface_play_reconst_create_processor(p->next[i], 1);
        play_interface_play_reconst_check_processor_loop(p->next[i]);
    }
    return NULL;
}

static void play_interface_play_reconst_check_processor(play_action_hashtable *ht)
{
    play_action_hashtable *item, *tmp;
    HASH_ITER(hh, ht, item, tmp) {
        if (item->action->proc) {
            play_interface_play_reconst_create_processor(item->action->proc, 1);
            play_interface_play_reconst_check_processor_loop(item->action->proc);
        }
    }
}


void play_interface_play_reconst_create_db_api(play_meta_hashtable *ht)
{
    char file[1024];
    char code[65535];
    play_string *src = play_string_new_with_size(1048576);
    play_string_append(src, "<?php\nclass DB\n{", strlen("<?php\nclass DB\n{"));

    play_meta_hashtable *item, *tmp;
    HASH_ITER(hh, ht, item, tmp) {
        sprintf(code, "\n    public static function get_%s(){\n        return new Query_%s();\n    }\n", item->meta->funcName->val, item->meta->funcName->val);
        play_string_append(src, code, strlen(code));
    }
    play_string_append(src, "}", 1);

    snprintf(file, 1024, "%s/library/play/db.api.php", gconfig.app_root);
    FILE *filefd = fopen(file, "w+");
    fwrite(src->val, src->len, 1, filefd);
    fclose(filefd);
    play_string_free(src);
    printf("create core: db.api.php...success\n");
}

void play_interface_play_reconst_create_metas_api(play_meta_hashtable *ht)
{
    char code[1024];
    play_string *src = play_string_new_with_size(1048576);

    char fileName[1024];
    play_meta_hashtable *item, *tmp;
    HASH_ITER(hh, ht, item, tmp) {
        sprintf(fileName, "%s/library/play/Meta.%s.api.php", gconfig.app_root, item->meta->funcName->val);
        sprintf(code, "<?php\nclass Query_%s extends Query\n{\n", item->meta->funcName->val);
        play_string_append(src, code, strlen(code));

        sprintf(code, "    /** %s */\n", item->meta->key->note->val);
        play_string_append(src, code, strlen(code));

        sprintf(code, "    public $%s;\n", item->meta->key->funcName->val);
        play_string_append(src, code, strlen(code));

        if (memcmp(item->meta->key->type->val, "auto", 4) != 0) {
            sprintf(code, "    /** %s */\n", item->meta->key->note->val);
            play_string_append(src, code, strlen(code));
            sprintf(code, "    public function set_%s($val){return $this;}\n", item->meta->key->funcName->val);
            play_string_append(src, code, strlen(code));
        }

        play_interface_play_recons_create_condit("where", item->meta->key->funcName->val, item->meta->key->type->val, item->meta->key->note->val, src);
        play_interface_play_recons_create_condit("or", item->meta->key->funcName->val, item->meta->key->type->val, item->meta->key->note->val, src);

        play_meta_field *field;
        for (field = item->meta->fields; field != NULL; field = field->next) {
            sprintf(code, "\n    /** %s */", field->note->val);
            play_string_append(src, code, strlen(code));

            sprintf(code, "\n    public $%s;\n", field->funcName->val);
            play_string_append(src, code, strlen(code));

            sprintf(code, "    /** %s */\n", field->note->val);
            play_string_append(src, code, strlen(code));

            sprintf(code, "    public function set_%s($val){return $this;}\n", field->funcName->val);
            play_string_append(src, code, strlen(code));

            play_interface_play_recons_create_condit("where", field->funcName->val, field->type->val, field->note->val, src);
            play_interface_play_recons_create_condit("or", field->funcName->val, field->type->val, field->note->val, src);
        }

        sprintf(code, "    public function orderBy($val,$desc){return $this;}\n", item->meta->key->funcName->val);
        play_string_append(src, code, strlen(code));
        sprintf(code, "    public function limit($start,$count){return $this;}\n", item->meta->key->funcName->val);
        play_string_append(src, code, strlen(code));

        play_string_append(src, "}", 1);
        FILE *filefd = fopen(fileName, "w+");

        fwrite(src->val, src->len, 1, filefd);
        fclose(filefd);
        play_string_reset(src);
        printf("create meat: %s...success\n", fileName);
    }
    play_string_free(src);
}

static void play_interface_play_recons_create_condit(const char *condit, const char *field, const char *type, const char *note, play_string *src)
{
    char code[258];

    if (note != NULL) {
        sprintf(code, "    /** %s */\n", note);
        play_string_append(src, code, strlen(code));
    }
    sprintf(code, "    public function %s_%s_Equal($val){return $this;}\n",condit, field);
    play_string_append(src, code, strlen(code));

    if (note != NULL) {
        sprintf(code, "    /** %s */\n", note);
        play_string_append(src, code, strlen(code));
    }
    sprintf(code, "    public function %s_%s_NotEqual($val){return $this;}\n", condit, field);
    play_string_append(src, code, strlen(code));

    if (note != NULL) {
        sprintf(code, "    /** %s */\n", note);
        play_string_append(src, code, strlen(code));
    }
    sprintf(code, "    public function %s_%s_In($val){return $this;}\n", condit, field);
    play_string_append(src, code, strlen(code));

    if (note != NULL) {
        sprintf(code, "    /** %s */\n", note);
        play_string_append(src, code, strlen(code));
    }    sprintf(code, "    public function %s_%s_NotIn($val){return $this;}\n", condit, field);
    play_string_append(src, code, strlen(code));

    if (note != NULL) {
        sprintf(code, "    /** %s */\n", note);
        play_string_append(src, code, strlen(code));
    }
    sprintf(code, "    public function %s_%s_Like($val){return $this;}\n", condit, field);
    play_string_append(src, code, strlen(code));

    if (note != NULL) {
        sprintf(code, "    /** %s */\n", note);
        play_string_append(src, code, strlen(code));
    }
    sprintf(code, "    public function %s_%s_Between($start, $end){return $this;}\n", condit, field);
    play_string_append(src, code, strlen(code));

    if (note != NULL) {
        sprintf(code, "    /** %s */\n", note);
        play_string_append(src, code, strlen(code));
    }
    sprintf(code, "    public function %s_%s_Less($val){return $this;}\n", condit, field);
    play_string_append(src, code, strlen(code));

    if (note != NULL) {
        sprintf(code, "    /** %s */\n", note);
        play_string_append(src, code, strlen(code));
    }
    sprintf(code, "    public function %s_%s_Gareater($val){return $this;}\n", condit, field);
    play_string_append(src, code, strlen(code));
}