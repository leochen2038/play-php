//
// Created by Leo on 18/5/3.
//

#include "play_core.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>

static play_file_mtime_hashtable *__play_action_files_mtime_path = NULL;        // 保存已经解析过的file最后修改时间
static play_action_hashtable *__play_manager_action_project_list = NULL;
static void play_manager_action_read_dir(play_action_hashtable **ht, const char *path);
static void play_manager_action_parse_file(play_action_hashtable **ht, const char *file, struct stat *s);
static void play_manager_action_free_action(play_action *action);


/**
 * 通过项目路径，取得该项目下所有的action定义数组
 * 服务器上可能会运行多个项目，以项目的路径名为key，保存该项目下的所有action
 * @param path
 * @return
 */
play_action_hashtable *play_manager_action_get_list_by_path(char *path)
{
    play_action_hashtable *ht = NULL;
    HASH_FIND_STR(__play_manager_action_project_list, path, ht);
    if (ht == NULL) {
        ht = (play_action_hashtable *)malloc(sizeof *ht);
        strcpy(ht->name, path);
        ht->project = NULL;
        HASH_ADD_STR(__play_manager_action_project_list, name, ht);
        play_manager_action_read_dir(&ht->project, path);
    }
    return ht->project;
}

/**
 * 递归项目action目录下的文件
 * @param ht
 * @param fht
 * @param path
 */
static void play_manager_action_read_dir(play_action_hashtable **ht, const char *path)
{
    struct dirent *dt;
    struct stat s;

    DIR *dir = opendir(path);
    if (dir == NULL) return;

    while((dt = readdir(dir)) != NULL) {
        if (dt->d_name[0] == '.') continue;
        char fullpath[1024];
        snprintf(fullpath, 1024, "%s/%s", path, dt->d_name);

        stat(fullpath, &s);
        if (S_ISDIR(s.st_mode)) {
            play_manager_action_read_dir(ht, fullpath);
        } else {
            play_file_mtime_hashtable *mfile = NULL;
            HASH_FIND_STR(__play_action_files_mtime_path, fullpath, mfile);
            if (mfile == NULL || s.st_mtime != mfile->mtime) {
                play_manager_action_parse_file(ht, fullpath, &s);
                if (mfile == NULL) {
                    mfile = (play_file_mtime_hashtable *)malloc(sizeof *mfile);
                    strcpy(mfile->name, fullpath);
                    HASH_ADD_STR(__play_action_files_mtime_path, name, mfile);
                }
                mfile->mtime = s.st_mtime;
            }
        }
    }
    closedir(dir);
}

/**
 * 从文件里分析出语法tokens
 * @param fp
 * @return
 */
static play_string* play_manager_action_get_token(FILE *fp)
{
    char c;
    play_string *str = play_string_new_with_size(255);
    while ((c = fgetc(fp)) != EOF) {
        switch (c) {
            case ',':
                if (str->len > 0) {
                    str->val[str->len++] = c;
                }
                break;
            case '#':
                while (((c = fgetc(fp)) != '\n'));
                break;
            case '=':
                fgetc(fp);
                if (str->len != 0) {
                    return str;
                }
                return NULL;
            case '{':
            case '}':
            case '(':
            case ')':
                if (str->len == 0) {
                    str->val[str->len++] = c;
                    return str;
                } else {
                    fseek(fp, -1, SEEK_CUR);
                    return str;
                }
            default:
                if (c != '\n' && c != '\t' && c != ' ' && c != '\r') {
                    str->val[str->len++] = c;
                }
                break;
        }
    }
    if (str->len != 0) {
        return str;
    } else {
        play_string_free(str);
    }
    return NULL;
}

/**
 * 生成一个新的action结构体
 * @param str
 * @return
 */
static play_action *play_manager_action_new_action(play_string *str)
{
    play_action *act = calloc(1, sizeof(play_action));
    act->name = play_string_new_with_chars(str->val, str->len);

    int i;
    for (i = 0; i < act->name->len; i++) {
        if (act->name->val[i] == '.')
            act->name->val[i] = '/';
    }

    return act;
}

/**
 * 解析process
 * @param cp
 * @param proc
 */
static void play_manager_action_parse_processor(play_string *cp, play_processor *proc)
{
    int aLen = cp->len;
    int i = aLen;
    int nameLen = aLen;
    char classPath[1024];

    for (i = aLen - 1; i != 0 ; i--) {
        if (cp->val[i] == '.') {
            nameLen = aLen - i;
            break;
        }
    }
    int index = i;
    if (i == 0) {
        proc->name = play_string_new_with_chars(cp->val + index, nameLen);
        proc->path = play_string_new_with_size(0);
    } else {
        for (i = 0; i < index; i++) {
            cp->val[i] = cp->val[i] == '.' ? '/' : cp->val[i];
        }
        proc->name = play_string_new_with_chars(cp->val + index+1, nameLen-1);
        proc->path = play_string_new_with_chars(cp->val, index);
    }

    if (proc->path->len == 0) {
        snprintf(classPath, 1024, "%s/source/processors/%s.php", gconfig.app_root, proc->name->val);
    } else {
        snprintf(classPath, 1024, "%s/source/processors/%s/%s.php", gconfig.app_root, proc->path->val, proc->name->val);
    }

    proc->file = play_string_new_with_chars(classPath, strlen(classPath));
    proc->lower_class_name = play_string_new_with_size(proc->name->len);
    play_str_tolower_copy(proc->lower_class_name->val, proc->name->val, proc->name->len);
    proc->lower_class_name->len = proc->name->len;
}

static void play_manager_action_add(play_action_hashtable **ht, play_action *act)
{
    play_action_hashtable *i;
    HASH_FIND_STR(*ht, act->name->val, i);
    if (i == NULL) {
        i = (play_action_hashtable *)malloc(sizeof *i);
        i->project = NULL;
        strcpy(i->name, act->name->val);
        HASH_ADD_STR(*ht, name, i);
    } else {
        play_manager_action_free_action(i->action);
    }
    i->action = act;
}

/**
 * 分析tokens语法
 * @param ht
 * @param tokens
 * @param file
 * @param s
 */
static void play_manager_action_parse_tokens(play_action_hashtable **ht, tokenList *tokens, const char *file, struct stat *s)
{

    play_string *tmp;
    int i = 0, j = 0;
    play_processor *curp = NULL;
    play_string *actname = play_string_new_with_size(256);

    for (i = 0; i < tokens->length; i++) {
        play_string *p = tokens->list[i];
        if (memcmp(p->val, "{", 1) == 0 && curp == NULL && i != 0) {
            tmp = tokens->list[i-1];
            p = tokens->list[++i];

            if (memcmp(p->val, "}", 1) == 0) {
                curp = NULL;
            } else {
                curp = calloc(1, sizeof(play_processor));
                curp->nextCount = 0;
                play_manager_action_parse_processor(p, curp);
            }

            // 分析出多个actname
            for (j = 0; j < tmp->len; j++) {
                if (tmp->val[j] == ' '){
                    continue;
                }
                if (tmp->val[j] == ',' && actname->len > 0)  {
                    play_action *act = play_manager_action_new_action(actname);
                    act->file = play_string_new_with_chars(file, strlen(file));
                    act->mtime = s->st_mtime;
                    act->proc = curp;
                    play_manager_action_add(ht, act);
                    play_string_reset(actname);
                } else if (tmp->val[j] != ',') {
                    play_string_append(actname, &tmp->val[j], 1);
                }
            }
            if (actname->len > 0) {
                play_action *act = play_manager_action_new_action(actname);
                act->file = play_string_new_with_chars(file, strlen(file));
                act->mtime = s->st_mtime;
                act->proc = curp;
                play_manager_action_add(ht, act);
                play_string_reset(actname);
            }
            continue;
        }
        if (memcmp(p->val, "(", 1) == 0 && curp != NULL) {
            play_string *rc = tokens->list[++i];
            if (memcmp(rc->val, ")", 1) == 0) {
                curp = curp->parent;
                continue;
            }

            play_string *p = tokens->list[++i];
            play_processor *proc = calloc(1, sizeof(play_processor));
            proc->parent = curp;
            proc->rcstring = play_string_new_with_chars(rc->val, rc->len);
            play_manager_action_parse_processor(p, proc);
            curp->next[curp->nextCount++] = proc;
            curp = proc;
            continue;
        }
        if (memcmp(p->val, ")", 1) == 0 && curp != NULL) {
            curp = curp->parent;
            continue;
        }
        if (curp != NULL) {
            play_string *rc = p;
            p = tokens->list[++i];
            play_processor *proc = calloc(1, sizeof(play_processor));
            proc->parent = curp;
            proc->rcstring = play_string_new_with_chars(rc->val, rc->len);
            play_manager_action_parse_processor(p, proc);
            curp->next[curp->nextCount++] = proc;
            curp = proc;
            continue;
        }
        if (memcmp(p->val, "}", 1) == 0) {
            curp = NULL;
            continue;
        }
    }

    play_string_free(actname);
}

/**
 * 释放词法tokens列表
 * @param tokens
 */
static void play_manager_action_free_tokens(tokenList *tokens)
{
    int i = 0;
    for (i = 0; i < tokens->length; i++) {
        free(tokens->list[i]);
    }
    free(tokens);
}

/**
 * 解析单个文件里的action
 * @param ht
 * @param file
 * @param s
 */
static void play_manager_action_parse_file(play_action_hashtable **ht, const char *file, struct stat *s)
{
    tokenList *tokens = calloc(1, sizeof(tokenList));
    FILE *fp = fopen(file, "r");

    while (!feof(fp)) {
        play_string *token = play_manager_action_get_token(fp);
        if (token != NULL) {
            tokens->list[tokens->length++] = token;
        }
    }
    fclose(fp);
    play_manager_action_parse_tokens(ht, tokens, file, s);
    play_manager_action_free_tokens(tokens);
}


/**
 * 释放processor结构体内存
 * @param p
 */
void play_manager_action_free_process(play_processor *p)
{
    int i;
    for (i = 0; i < p->nextCount; i++) {
        play_manager_action_free_process(p->next[i]);
    }

    play_string_free(p->rcstring);
    play_string_free(p->name);
    play_string_free(p->path);
    play_string_free(p->file);
    play_string_free(p->lower_class_name);
    free(p);
}

/**
 * 释放action结构体内存
 * @param data
 */
static void play_manager_action_free_action(play_action *action)
{
    play_processor *p = action->proc;
    if (p != NULL) {
        play_manager_action_free_process(p);
    }

    play_string_free(action->file);
    play_string_free(action->name);
    free(action);
}


/**
 * 通过名称获取对应的action
 * @param name
 * @param checknew
 * @return
 */
play_action *play_manager_action_get_by_chars(char *name, int checknew)
{
    play_action *action = NULL;
    play_action_hashtable *item = NULL;
    play_action_hashtable *ht = play_manager_action_get_list_by_path(gconfig.action_root);
    HASH_FIND_STR(ht, name, item);
    if (checknew == 1) {
        if (item != NULL && (action = item->action)) {
            struct stat s;
            stat(action->file->val, &s);

            if (s.st_mtime != action->mtime) {
                play_string *file = play_string_new_with_chars(action->file->val, action->file->len);
                play_manager_action_parse_file(&ht, file->val, &s);
                play_string_free(file);
            }
        }
        if (item == NULL) {
            play_manager_action_read_dir(&ht, gconfig.action_root);
            HASH_FIND_STR(ht, name, item);
        }
    }
    return item == NULL ? NULL :item->action;
}

