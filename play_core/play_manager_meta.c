//
// Created by Leo on 18/5/7.
//
#include "play_core.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <dirent.h>
#include <sys/stat.h>

static play_file_mtime_hashtable *__play_meta_files_mtime_path = NULL;
static play_meta_hashtable *__play_manager_meta_project_list = NULL;
static void play_manager_meta_refash_dir(play_meta_hashtable **ht, char *path);
static void play_manager_meta_free_meta(play_meta *meta);
static void play_manager_meta_free_meta_fields(play_meta_field *field);

play_meta_hashtable *play_manager_meta_get_list_by_path(char *path)
{
    play_meta_hashtable *ht = NULL;
    HASH_FIND_STR(__play_manager_meta_project_list, path, ht);
    if (ht == NULL) {
        ht = (play_meta_hashtable *)malloc(sizeof *ht);
        strcpy(ht->name, path);
        ht->project = NULL;
        HASH_ADD_STR(__play_manager_meta_project_list, name, ht);
        play_manager_meta_refash_dir(&ht->project, path);
    }
    return ht->project;
}

// 创建新meta结构
play_meta* play_manager_meta_create_meta(const char *filename, xmlNode *proot)
{
    int i;
    char c;
    xmlChar *module, *name;
    play_meta *meta = calloc(1, sizeof(play_meta));
    module = xmlGetProp(proot, "module");
    name = xmlGetProp(proot, "name");
    int moduleSize = strlen(module);
    int nameSize = strlen(name);

    if (moduleSize < 1) {
        // play_Error_trigger_Exception(0x00, "get meta module error in %s", filename);
        xmlFree(module);
        xmlFree(name);
        return NULL;
    }
    if (nameSize < 1) {
        // play_Error_trigger_Exception(0x00, "get meta name error in %s", filename);
        xmlFree(module);
        xmlFree(name);
        return NULL;
    }

    meta->module = play_string_new_with_chars(module, strlen(module));
    meta->name = play_string_new_with_chars(name, strlen(name));
    meta->funcName = play_string_new_with_size(meta->module->len + meta->name->len+5);
    xmlFree(module);
    xmlFree(name);

    c = toupper(meta->module->val[0]);
    play_string_append(meta->funcName, &c, 1);
    int change = 0;
    for (i = 1; i < meta->module->len; i++) {
        if (meta->module->val[i] == '_') {
            change = 1;
            continue;
        }
        if (change) {
            c = toupper(meta->module->val[i]);
            play_string_append(meta->funcName, &c, 1);
            change = 0;
            continue;
        }
        play_string_append(meta->funcName, &meta->module->val[i], 1);
    }
    play_string_append(meta->funcName, "_" , 1);
    c = toupper(meta->name->val[0]);
    play_string_append(meta->funcName, &c, 1);
    for (i = 1; i < meta->name->len; i++) {
        if (meta->name->val[i] == '_') {
            change = 1;
            continue;
        }
        if (change) {
            c = toupper(meta->name->val[i]);
            play_string_append(meta->funcName, &c, 1);
            change = 0;
            continue;
        }
        c = meta->name->val[i];
        play_string_append(meta->funcName, &c, 1);
    }

    return meta;
}

// 从xml节点中 解析key字段
void play_manager_meta_create_key(play_meta *meta, const char *filename, xmlNode *pcur)
{
    xmlChar *keyName = xmlGetProp(pcur, "name");
    xmlChar *keyLength = xmlGetProp(pcur, "length");
    xmlChar *keyType = xmlGetProp(pcur, "type");
    xmlChar *keyNote = xmlGetProp(pcur, "note");

    if (keyName == NULL) {
        // play_Error_trigger_Exception(0x00, "can not get key name in %s", filename);
        xmlFree(keyName);
        xmlFree(keyLength);
        xmlFree(keyType);
        xmlFree(keyNote);
        return;
    }
    if (keyType == NULL) {
        // play_Error_trigger_Exception(0x00, "can not get key type in %s", filename);
        xmlFree(keyName);
        xmlFree(keyLength);
        xmlFree(keyType);
        xmlFree(keyNote);
        return;
    }

    meta->key = calloc(1, sizeof(play_meta_field));
    meta->key->name = play_string_new_with_chars(keyName, strlen(keyName));
    meta->key->type = play_string_new_with_chars(keyType, strlen(keyType));
    if (keyLength != NULL) {
        meta->key->length = play_string_new_with_chars(keyLength, strlen(keyLength));
    }
    if (keyNote != NULL) {
        meta->key->note = play_string_new_with_chars(keyNote, strlen(keyNote));
    }

    int fieldNameSize = strlen(keyName);
    play_string *arKey = play_string_new_with_size(fieldNameSize);
    play_string *upper = play_string_new_with_size(fieldNameSize);
    char c = toupper(keyName[0]);
    int change = 1;
    if (c != '_') {
        play_string_append(arKey, &c, 1);
        play_string_append(upper, &c, 1);
        change = 0;
    }
    int i;
    for (i = 1; i < fieldNameSize; i++) {
        if (keyName[i] == '_') {
            //play_String_append(arKey, keyName+i, 1);
            change = 1;
            continue;
        }
        if (change) {
            c = toupper(keyName[i]);
            play_string_append(arKey, &c, 1);
            play_string_append(upper, &c, 1);
            change = 0;
            continue;
        }
        c = toupper(keyName[i]);
        play_string_append(arKey, keyName+i, 1);
        play_string_append(upper, &c, 1);
    }
    meta->key->funcName = arKey;
    meta->key->upperName = upper;

    xmlFree(keyName);
    xmlFree(keyLength);
    xmlFree(keyType);
    xmlFree(keyNote);
}

void play_manager_meta_add_filed(play_meta *meta, play_meta_field *filed)
{
    if (meta->fields == NULL) {
        meta->fields = filed;
    } else {
        play_meta_field *p = meta->fields;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = filed;
    }
}

// 从xml节点中 解析fields字段
void play_manager_meta_create_fileds(play_meta *meta, const char *filename, xmlNode *pcur)
{
    char c;
    int i;
    int change = 0;
    xmlNode *p = pcur->xmlChildrenNode;

    while (p != NULL) {
        if (!xmlStrcmp(p->name, BAD_CAST("field"))) {

            xmlChar *fieldName = xmlGetProp(p, "name");
            xmlChar *fieldType = xmlGetProp(p, "type");
            xmlChar *fieldLength = xmlGetProp(p, "length");
            xmlChar *fieldDef = xmlGetProp(p, "default");
            xmlChar *fieldNote = xmlGetProp(p, "note");

            if (fieldName == NULL) {
                // play_Error_trigger_Exception(0x00, "can not get field name in %s", filename);
                xmlFree(fieldName);
                xmlFree(fieldType);
                xmlFree(fieldLength);
                xmlFree(fieldDef);
                xmlFree(fieldNote);
                return;
            }
            if (fieldType == NULL) {
                // play_Error_trigger_Exception(0x00, "can not get field type in %s", filename);
                xmlFree(fieldName);
                xmlFree(fieldType);
                xmlFree(fieldLength);
                xmlFree(fieldDef);
                xmlFree(fieldNote);
                return;
            }

            int fieldNameSize = strlen(fieldName);
            play_string *arKey = play_string_new_with_size(fieldNameSize);
            play_string *uppeName = play_string_new_with_size(fieldNameSize);

            c = toupper(fieldName[0]);
            play_string_append(arKey, &c, 1);
            play_string_append(uppeName, &c, 1);

            change = 0;
            for (i = 1; i < fieldNameSize; i++) {
                if (fieldName[i] == '_') {
                    //play_String_append(uppeName, fieldName+i, 1);
                    change = 1;
                    continue;
                }
                if (change) {
                    c = toupper(fieldName[i]);
                    play_string_append(arKey, &c, 1);
                    play_string_append(uppeName, &c, 1);
                    change = 0;
                    continue;
                }
                c = toupper(fieldName[i]);
                play_string_append(arKey, fieldName+i, 1);
                play_string_append(uppeName, &c, 1);

            }

            play_meta_field *field = calloc(1, sizeof(play_meta_field));
            field->funcName = arKey;
            field->upperName = uppeName;

            field->name = play_string_new_with_chars(fieldName, strlen(fieldName));
            field->type = play_string_new_with_chars((char*)fieldType, strlen(fieldType));
            if (fieldLength != NULL) {
                field->length = play_string_new_with_chars(fieldLength, strlen(fieldLength));
            } else {
                field->length = play_string_new_with_size(0);
            }

            if (fieldDef != NULL) {
                field->defv = play_string_new_with_chars(fieldDef, strlen(fieldDef));
            } else {
                field->defv = play_string_new_with_size(0);
            }

            if (fieldNote != NULL) {
                field->note = play_string_new_with_chars(fieldNote, strlen(fieldNote));
            } else {
                field->note = play_string_new_with_size(0);
            }
            play_manager_meta_add_filed(meta, field);
            // ds_puts(meta->fields, arKey->val, field);

            xmlFree(fieldName);
            xmlFree(fieldType);
            xmlFree(fieldLength);
            xmlFree(fieldDef);
            xmlFree(fieldNote);
        }
        p = p->next;
    }
}

void play_manager_meta_create_strategy(const char *filename, play_meta *meta, xmlNode *pcur)
{
    xmlNode *p = pcur->xmlChildrenNode;
    if (xmlStrcmp(p->name, BAD_CAST("storage")) == 0) {
        xmlChar *type = xmlGetProp(p, "type");
        xmlChar *router = xmlGetProp(p, "router");
        xmlChar *engine = xmlGetProp(p, "engine");

        meta->storage = calloc(1, sizeof(play_meta_storage));
        meta->storage->type = play_string_new_with_chars(type, strlen(type));
        meta->storage->router = play_string_new_with_chars(router, strlen(router));
        meta->storage->engine = play_string_new_with_chars((char*)engine, strlen(engine));

        xmlFree(type);
        xmlFree(router);
        xmlFree(engine);
    }
}

static void play_manager_meta_add(play_meta_hashtable **ht, play_meta *meta)
{
    play_meta_hashtable *i;
    HASH_FIND_STR(*ht, meta->funcName->val, i);
    if (i == NULL) {
        i = (play_meta_hashtable *)malloc(sizeof *i);
        i->project = NULL;
        strcpy(i->name, meta->funcName->val);
        HASH_ADD_STR(*ht, name, i);
    } else {
        play_manager_meta_free_meta(i->meta);
    }
    i->meta = meta;
}

static void play_manager_meta_parse(play_meta_hashtable **ht, const char *filename, struct stat *s)
{
    xmlDocPtr pdoc = NULL;
    xmlNodePtr proot = NULL, pcur = NULL;

    xmlKeepBlanksDefault(0);
    pdoc = xmlReadFile (filename, "UTF-8", XML_PARSE_RECOVER);
    if (pdoc == NULL) {
        // play_Error_trigger_Exception(0x00, "can not find %s", filename);
        return;
    }

    proot = xmlDocGetRootElement (pdoc);
    if (proot == NULL) {
        // play_Error_trigger_Exception(0x00, "%s is empty", filename);
        return;
    }
    play_meta *meta = play_manager_meta_create_meta(filename, proot);
    meta->file = play_string_new_with_chars(filename, strlen(filename));
    meta->mtime = s->st_mtime;
    play_manager_meta_create_key(meta, filename, proot->xmlChildrenNode);
    play_manager_meta_create_fileds(meta, filename, proot->xmlChildrenNode->next);
    play_manager_meta_create_strategy(filename, meta, proot->xmlChildrenNode->next->next);
    play_manager_meta_add(ht, meta);

    xmlFreeDoc (pdoc);
    xmlCleanupParser();
    xmlMemoryDump();
}

static void play_manager_meta_refash_dir(play_meta_hashtable **ht,  char *path)
{
    struct dirent *dt;
    DIR *dir = opendir(path);
    struct stat s;
    if (dir == NULL) return;

    while((dt = readdir(dir)) != NULL) {
        if (dt->d_name[0] == '.') continue;
        char fullpath[1024];
        snprintf(fullpath, 1024, "%s/%s", path, dt->d_name);
        stat(fullpath, &s);
        if (S_ISDIR(s.st_mode)) {
            play_manager_meta_refash_dir(ht, fullpath);
        } else {
            play_file_mtime_hashtable *mfile = NULL;
            HASH_FIND_STR(__play_meta_files_mtime_path, fullpath, mfile);
            if (mfile == NULL || s.st_mtime != mfile->mtime) {
                play_manager_meta_parse(ht, fullpath, &s);
                if (mfile == NULL) {
                    mfile = (play_file_mtime_hashtable *)malloc(sizeof *mfile);
                    strcpy(mfile->name, fullpath);
                    HASH_ADD_STR(__play_meta_files_mtime_path, name, mfile);
                }
                mfile->mtime = s.st_mtime;
            }
        }
    }
    closedir(dir);
}

static void play_manager_meta_free_meta_fields(play_meta_field *field)
{
    if (field->next != NULL) {
        play_manager_meta_free_meta_fields(field->next);
    }
    play_string_free(field->name);
    play_string_free(field->funcName);
    play_string_free(field->upperName);
    play_string_free(field->type);
    play_string_free(field->length);
    play_string_free(field->defv);
    play_string_free(field->note);
    free(field);
}

/**
 * 释放meta结构体内存
 * @param meta
 */
static void play_manager_meta_free_meta(play_meta *meta)
{
    play_string_free(meta->storage->type);
    play_string_free(meta->storage->router);
    play_string_free(meta->storage->engine);
    free(meta->storage);

    if (meta->fields != NULL) {
        play_manager_meta_free_meta_fields(meta->fields);
    }

    play_string_free(meta->file);
    play_string_free(meta->funcName);
    play_string_free(meta->module);
    play_string_free(meta->name);

    play_string_free(meta->key->name);
    play_string_free(meta->key->funcName);
    play_string_free(meta->key->upperName);
    play_string_free(meta->key->type);
    play_string_free(meta->key->length);
    play_string_free(meta->key->defv);
    play_string_free(meta->key->note);
    free(meta->key);

    free(meta);
}

play_meta *play_manager_meta_get_by_chars(char *name, int checknew)
{
    play_meta *meta = NULL;
    play_meta_hashtable *item = NULL;
    play_meta_hashtable *ht = play_manager_meta_get_list_by_path(gconfig.meta_root);
    HASH_FIND_STR(ht, name, item);
    if (checknew == 1) {
        if (item != NULL && (meta = item->meta)) {
            struct stat s;
            stat(meta->file->val, &s);
            if (s.st_mtime != meta->mtime) {
                play_string *file = play_string_new_with_chars(meta->file->val, meta->file->len);
                play_manager_meta_parse(&ht, file->val, &s);
                play_string_free(file);
            }
        } else {
            play_manager_meta_refash_dir(&ht, gconfig.meta_root);
            HASH_FIND_STR(ht, name, item);
        }
    }
    return item == NULL ? NULL : item->meta;
}