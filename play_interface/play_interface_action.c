//
// Created by Leo on 18/5/10.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../play_core/play_core.h"
#include "play_interface.h"
#include <stdio.h>
#include <zend_interfaces.h>

zend_class_entry *play_interface_action_ce;
static void play_interface_action_render();
static void play_interface_action_run(play_action *act);
static void play_interface_action_run_with_debug(play_action *act);

static void play_interface_action_get_uri_from_url(play_string **uri, play_string **render);
static void play_interface_action_update_property(const char *url, int urllen, const char *render, int render_len);
static play_processor *play_interface_action_run_processor(play_processor *p);

PHP_METHOD(Action, boot);
const zend_function_entry  play_interface_action_functions[] = {
    PHP_ME(Action, boot, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};

void play_interface_action_register(int _module_number)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY(class, "Action", play_interface_action_functions);
    play_interface_action_ce = zend_register_internal_class_ex(&class, NULL);
    zend_declare_property_null(play_interface_action_ce, "name", 4, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC);
    zend_declare_property_null(play_interface_action_ce, "render", 6, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC);
    zend_declare_property_null(play_interface_action_ce, "url", 3, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC);
    zend_declare_property_null(play_interface_action_ce, "template", 8, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC);
    zend_declare_property_null(play_interface_action_ce, "track", 5, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC);
}

PHP_METHOD(Action, boot)
{
    int arg_len = 0;
    char *arg_val = NULL;
    play_string *uri = NULL;
    play_string *render = NULL;
    play_action *action = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|s", &arg_val, &arg_len) == FAILURE) {
        return;
    }

    if (arg_len > 0) {
        uri = play_string_new_with_chars(arg_val, arg_len);
    } else {
        play_interface_action_get_uri_from_url(&uri, &render);
    }

    if (render == NULL) {
        zval *pr = zend_read_static_property(play_interface_play_ce, "render", 6, 1);
        play_interface_action_update_property(uri->val, uri->len, Z_STRVAL_P(pr),  Z_STRLEN_P(pr));
    } else {
        play_interface_action_update_property(uri->val, uri->len, render->val, render->len);
    }

    int checknew = play_interface_play_checknew();
    if ((action = play_manager_action_get_by_chars(uri->val, checknew)) == NULL) {
        play_interface_utils_trigger_exception(PLAY_ERR_ACTION_NOT_FIND, "can not find action %s\n", uri->val);
    } else {
        play_interface_action_run_with_debug(action);
    }

    play_string_free(uri);
    play_string_free(render);
    play_interface_action_render();
}

// 执行渲染输出
static void play_interface_action_render()
{
    int p_e = 0;
    zval exception;
    zval *render = NULL;
    int render_classname_len;

    if (EG(exception)) {
        p_e = 1;
        ZVAL_OBJ(&exception, EG(exception));
        EG(exception) = NULL;
    }
    render = zend_read_static_property(play_interface_action_ce, "render", 6, 1);
    render_classname_len = 8 + Z_STRLEN_P(render);
    char bigCase = toupper(Z_STRVAL_P(render)[0]);
    char className[render_classname_len];
    snprintf(className, render_classname_len, "Render_%c%s", bigCase, Z_STRVAL_P(render)+1);

    char *class_lowercase;
    zend_class_entry *ce = NULL;
    zval obj;

    char classPath[1024];
    snprintf(classPath, 1024, "%s/render/%s.php", gconfig.app_root, className);

    class_lowercase = zend_str_tolower_dup(className, render_classname_len-1);
    if ((ce = zend_hash_str_find_ptr(EG(class_table), class_lowercase, render_classname_len-1)) == NULL) {
        if (!play_interface_utils_loader_import(classPath)) {
            efree(class_lowercase);
            play_interface_utils_trigger_exception(PLAY_ERR_ACTION_NOT_FIND, "can not load render %s.php\n", className);
            return;
        }

        if ((ce = zend_hash_str_find_ptr(EG(class_table), class_lowercase, render_classname_len-1)) == NULL) {
            efree(class_lowercase);
            play_interface_utils_trigger_exception(PLAY_ERR_ACTION_NOT_FIND, "can not find class %s.php\n", className);
            return;
        }
    }

    object_init_ex(&obj, ce);
    zend_call_method_with_0_params(&obj, ce, NULL, "setheader", NULL);

    if (p_e) {
        zend_call_method_with_1_params(&obj, ce, NULL, "exception", NULL, &exception);
        zval_ptr_dtor(&exception);
    } else {
        zend_call_method_with_0_params(&obj, ce, NULL, "run", NULL);
    }

    efree(class_lowercase);
    zval_ptr_dtor(&obj);
}

// 执行processor, 并返回下一个processor
static play_processor *play_interface_action_run_processor(play_processor *p)
{
    zval obj, ret;
    play_processor *next = NULL;
    zend_class_entry *ce = NULL;

    // 加载php文件
    if ((ce = zend_hash_str_find_ptr(EG(class_table), p->lower_class_name->val, p->lower_class_name->len)) == NULL) {
        if (!play_interface_utils_loader_import(p->file->val)) {
            play_interface_utils_trigger_exception(PLAY_ERR_ACTION_NOT_FIND, "can not load processor file %s", p->file->val);
            return NULL;
        }
        if ((ce = zend_hash_str_find_ptr(EG(class_table), p->lower_class_name->val, p->lower_class_name->len)) == NULL) {
            play_interface_utils_trigger_exception(PLAY_ERR_ACTION_NOT_FIND, "can not find class %s in %s", p->name->val, p->file->val);
            return NULL;
        }
    }

    // 生成process对象并执行run方法
    object_init_ex(&obj, ce);
    zend_call_method_with_0_params(&obj, ce, NULL, "run", &ret);
    zval_ptr_dtor(&obj);

    // 如果有抛出异常则返回NULL, 不执行后面的process
    if (EG(exception)) {
        zval_ptr_dtor(&ret);
        return NULL;
    }

    // 检查process返回，并返回对应的process;
    int rc_code = 0;
    if (Z_TYPE(ret) == IS_LONG) {
        rc_code = (int)Z_LVAL(ret);
    } else if (Z_TYPE(ret) != IS_NULL) {
        zval_ptr_dtor(&ret);
        return NULL;
    }

    if (p->nextCount != 0) {
        int i;
        zval *pData = NULL;
        for (i = 0; i < p->nextCount; i++) {
            if ((pData = zend_hash_str_find_ptr(&(ce->constants_table),  p->next[i]->rcstring->val, p->next[i]->rcstring->len)) == NULL) {
                zval_ptr_dtor(&ret);
                play_interface_utils_trigger_exception(PLAY_ERR_ACTION_NOT_FIND, "Undefined class constant '%s::%s'", p->name->val, p->next[i]->rcstring->val);
                return NULL;
            }
            if (rc_code == Z_LVAL_P(pData)) {
                next = p->next[i];
            }
        }
    }

    zval_ptr_dtor(&ret);
    return next;
}

static void play_interface_action_run_with_debug(play_action *act)
{
    play_processor *p = act->proc;
    if (p != NULL) {
        zval *track = zend_read_static_property(play_interface_action_ce, "track", 5, 1);
        if (Z_TYPE_P(track) == IS_NULL) {
            array_init(track);
        }
        char barf[128] = {NULL};
        do {
            char strtrack[256];
            if (p->parent != NULL) {
                snprintf(strtrack, 256, "%s|-- %s::%s", barf,  p->rcstring->val, p->name->val);
                snprintf(barf, 128, "%s    ", barf);
            } else {
                snprintf(strtrack, 256, "%s", p->name->val);
            }
            add_next_index_stringl(track, strtrack, strlen(strtrack));
        } while((p = play_interface_action_run_processor(p)) != NULL);
    }
}

static void play_interface_action_run(play_action *act)
{
    play_processor *p = act->proc;
    if (p != NULL) {
        while((p = play_interface_action_run_processor(p)) != NULL);
    }
}

// 更新php的Action类，静态成员属性
static void play_interface_action_update_property(const char *url, int urllen, const char *render, int render_len)
{
    int i;
    char name[128] = {0};
    for (i = 0; i < urllen; i++) {
        if (url[i] == '/') {
            name[i] = '.';
        } else {
            name[i] = url[i];
        }
    }

    zend_update_static_property_stringl(play_interface_action_ce, "url", 3, url, urllen);
    zend_update_static_property_stringl(play_interface_action_ce, "name", 4, name, urllen);
    zend_update_static_property_stringl(play_interface_action_ce, "render", 6, render , render_len);
    zend_update_static_property_stringl(play_interface_action_ce, "template", 8, url , urllen);
}

static void play_interface_action_get_uri_from_url(play_string **uri, play_string **render)
{
    int i;
    int render_len = 0;
    int uri_len = 0;
    int find_dot = 0;

    if (PG(auto_globals_jit)) {
        zend_is_auto_global_str("_SERVER", 7);
    }

    zval *pzval = zend_hash_str_find(Z_ARRVAL_P(&PG(http_globals)[TRACK_VARS_SERVER]), "REQUEST_URI", 11);
    if (pzval != NULL && Z_STRLEN_P(pzval) > 0) {
        char *foo = Z_STRVAL_P(pzval);
        for (i = 1; i < strlen(foo); ++i) {
            if (foo[i] == '?') {
                break;
            }
            if (foo[i] == '.') {
                find_dot = 1;
                continue;
            }
            if (find_dot) {
                render_len++;
            } else {
                uri_len++;
            }
        }
    }
    *uri = (uri_len > 0) ? play_string_new_with_chars(Z_STRVAL_P(pzval)+1, uri_len) : play_string_new_with_chars("index",  5);
    *render = (render_len > 0) ? play_string_new_with_chars(Z_STRVAL_P(pzval)+2+uri_len, render_len) : NULL;
}