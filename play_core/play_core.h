//
// Created by Leo on 2018/8/13.
//

#ifndef PROJECT_PLAY_CORE_H
#define PROJECT_PLAY_CORE_H

#include <zend_interfaces.h>
#include "../play_lib/uthash/uthash.h"

/* play_string.c */
typedef struct {
    int len;
    int size;
    char val[1];
}play_string;

play_string * play_string_new_with_size(int size);
play_string* play_string_new_with_chars(const char *chars, const int length);
int play_string_append(play_string *str, const char *chars, const int length);
int play_string_reset(play_string *string);
int play_string_free(play_string *string);


/* play_global_config.c */
typedef struct {
    char app_root[512];
    char app_root_ex[512];
    int app_root_length;
    char action_root[512];
    char meta_root[512];
    int meta_root_length;
    char server_root[512];
    int server_root_length;
    int action_root_length;
    char class_service_path[512];
    int class_service_path_length;
}global_config;

extern global_config gconfig;

void play_gloabl_config_init();
int play_global_config_set_app_root(const char *app_root, int app_root_leng);
char *play_global_config_error_message(int errcode);


/* play_manager_action.c */
typedef struct _processor {
    play_string *rcstring;
    play_string *name;
    play_string *path;
    play_string *file;
    play_string *lower_class_name;
    struct _processor *parent;
    struct _processor *next[32];
    int nextCount;
} play_processor;

typedef struct {
    long mtime;
    play_string *file;
    play_string *name;
    play_processor *proc;
} play_action;

typedef struct _play_action_hashtable{
    char name[128];                                 // action or project path name max
    struct _play_action_hashtable *project;
    play_action *action;
    UT_hash_handle hh;
} play_action_hashtable;

typedef struct _play_file_mtime_hashtable{
    char name[128];                                 // action file path max size
    struct _play_file_mtime_hashtable *project;
    long mtime;
    UT_hash_handle hh;
} play_file_mtime_hashtable;

typedef struct {
    play_string *list[2048];
    int size;
    int length;
} tokenList;

play_action *play_manager_action_get_by_chars(char *name, int checknew);
play_action_hashtable *play_manager_action_get_list_by_path(char *path);

/* play_manager_meta.c */
typedef struct _play_meta_field{
    play_string *name;
    play_string *funcName;
    play_string *upperName;
    play_string *type;
    play_string *length;
    play_string *defv;
    play_string *note;
    struct _play_meta_field *next;
}play_meta_field;

typedef struct {
    play_string *type;
    play_string *router;
    play_string *engine;
    play_string *database;
    play_string *table;
}play_meta_storage;

typedef struct{
    long mtime;
    play_string *file;
    play_string *funcName;
    play_string *module;
    play_string *name;
    play_meta_field *key;
    play_meta_field *fields;
    play_meta_storage *storage;
    zend_class_entry *ce;
}play_meta;

typedef struct _play_meta_hashtable{
    char name[128];                                 // meta name or project path  max
    struct _play_meta_hashtable *project;
    play_meta *meta;
    UT_hash_handle hh;
} play_meta_hashtable;

play_meta *play_manager_meta_get_by_chars(char *name, int checknew);
play_meta_hashtable* play_manager_meta_get_list_by_path(char *path);



/* tool.c method */
int play_explode(char ***dest, const char *src, const char delim);
int play_is_numeric(char *str);
void play_get_micro_uqid(char *muqid, char *hexip, int pid);
void play_str_tolower_copy(char *dest, const char *source, int length);
play_string *play_find_project_root_by_path(const char *path, int cache);


/* play_socket */
typedef struct {
    char ipv4[21];
    int socket_fd;
    char local_ip_hex[9];
    char *read_buf;
    int state;                      /* 状态,0,正常可用，1：关闭*/
    unsigned int read_buf_ncount;   /* 需要读取的字节数 */
    unsigned int read_buf_rcount;   /* 实际读取的字节数 */
    UT_hash_handle hh;
} play_socket_ctx;

size_t play_socket_recv_with_protocol_v1(play_socket_ctx *sctx);
size_t play_socket_send_with_protocol_v1(play_socket_ctx *sctx, char *request_id, const char *cmd, int cmd_len, const char *data, int data_len, char respond);
size_t play_socket_send_with_protocol_v2(play_socket_ctx *sctx, unsigned short callerId, char *request_id, const char *cmd, int cmd_len, const char *data, int data_len, char respond);

play_socket_ctx *play_socket_connect(const char *host, int port, int wait_time, int persisent);
size_t play_socket_send_recv(int socket_fd, const char *send, int sendlen, char *recv);
void play_socket_cleanup_with_protocol(play_socket_ctx *sctx);
void play_socket_cleanup_and_close(play_socket_ctx *sctx, int persisent);
size_t socket_read(int socketfd, char *buffer, int length);


/* play_fastcgi */
int play_fastcgi_start_request(play_socket_ctx *sctx);
int play_fastcgi_end_request(play_socket_ctx *sctx);
int play_fastcgi_set_param(play_socket_ctx *sctx, char *key, int klen, char *val, int vlen);
int play_fastcgi_set_boby(play_socket_ctx *sctx, char *body, int len);
unsigned char * play_fastcgi_get_response(play_socket_ctx *sctx, int *response_size);
int play_fastcgi_parse_head(char *data, int data_size);

#endif //PROJECT_PLAY_CORE_H