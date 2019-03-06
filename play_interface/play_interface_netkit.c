//
// Created by Leo on 2018/10/16.
//


#include "../play_core/play_core.h"
#include "play_interface.h"
#include <stdio.h>
#include <zend_interfaces.h>

PHP_METHOD(NetKit, socket_protocol);
PHP_METHOD(NetKit, socket_fastcgi);

zend_class_entry *play_interface_netkit_ce;

const zend_function_entry  play_interface_net_functions[] = {
    PHP_ME(NetKit, socket_protocol, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(NetKit, socket_fastcgi, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};


void play_interface_netkit_register(int _module_number)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY(class, "NetKit", play_interface_net_functions);
    play_interface_netkit_ce = zend_register_internal_class_ex(&class, NULL);
}

PHP_METHOD(NetKit, socket_fastcgi)
{
    long port;
    zval *host = NULL;
    zval *params = NULL;
    zval *body = NULL;
    long timeout = 0;
    unsigned char respond = 1;
#ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "zlzz|bl", &host, &port, &params, &body, &respond, &timeout) == FAILURE) {
        return;
    }
#else
    ZEND_PARSE_PARAMETERS_START(4, 6)
        Z_PARAM_ZVAL(host)
        Z_PARAM_LONG(port)
        Z_PARAM_ZVAL(params)
        Z_PARAM_ZVAL(body)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(respond)
        Z_PARAM_LONG(timeout)
    ZEND_PARSE_PARAMETERS_END();
#endif

    if (port <= 0) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "port must greater than zero");
        RETURN_NULL();
    }
    if (Z_TYPE_P(host) != IS_STRING || Z_STRLEN_P(host) <= 0) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "host must be string, not empty");
        RETURN_NULL();
    }
    if (Z_TYPE_P(params) != IS_ARRAY) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "NetKit::socket_fastcgi()  parameter 3 must to be array");
        RETURN_NULL();
    }
    if (Z_TYPE_P(body) != IS_STRING) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "NetKit::socket_fastcgi()  parameter 4 must to be string");
        RETURN_NULL();
    }

    int i;
    zend_ulong idx;
    zval *z_item = NULL;
    zend_string *z_key;

    int size = 0;
    char *buf;
    int buf_size = 512 + Z_STRLEN_P(body);
    buf = calloc(1, buf_size);


    play_socket_ctx *client;
    timeout = timeout > 0 ? timeout : 1;
    if ((client = play_socket_connect(Z_STRVAL_P(host), port, timeout)) == NULL) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not connect %s:%d", Z_STRVAL_P(host), port);
        RETURN_NULL();
    }

    size = play_fastcgi_start_request(buf);
    int count = zend_hash_num_elements(Z_ARRVAL_P(params));
    zend_hash_internal_pointer_reset(Z_ARRVAL_P(params));

    for (i = 0; i < count; i ++) {
        z_item = zend_hash_get_current_data(Z_ARRVAL_P(params));
        zend_hash_get_current_key(Z_ARRVAL_P(params), &z_key, &idx);
        size += play_fastcgi_set_param(buf+size, z_key->val, z_key->len, Z_STRVAL_P(z_item), Z_STRLEN_P(z_item));
        zend_hash_move_forward(Z_ARRVAL_P(params));
    }
    size += play_fastcgi_end_request(buf+size);

    if (Z_STRLEN_P(body) > 0) {
        size += play_fastcgi_set_boby(buf + size, Z_STRVAL_P(body), Z_STRLEN_P(body));
        size += play_fastcgi_end_body(buf + size);
    }

    if (respond) {
        int response_size = 0;
        char *response = NULL;
        response = play_fastcgi_send_request(client->socket_fd, buf, size, &response_size);
        free(buf);
        if (response == NULL && response_size > 0) {
            play_interface_utils_trigger_exception(PLAY_ERR_BASE, "NetKit::socket_fastcgi() send or recv error");
            RETURN_NULL();
        }
        i = play_fastcgi_parse_head(response, response_size);
        ZVAL_STRINGL(return_value, response + i, response_size - i);
        free(response);
    } else {
        int nwrite = 0;
        nwrite = write(client->socket_fd, buf, size);
        free(buf);
        if (nwrite != size) {
            play_interface_utils_trigger_exception(PLAY_ERR_BASE, "NetKit::socket_fastcgi() send error");
            RETURN_NULL();
        }
    }
}

PHP_METHOD(NetKit, socket_protocol)
{
    long port = 0;
    long timeout = 0;
    unsigned char respond = 1;
    zval *host, *cmd, *message;

#ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "zlzz|bl", &host, &port, $cmd, &message, &respond, &timeout) == FAILURE) {
        return;
    }
#else
    ZEND_PARSE_PARAMETERS_START(4, 6)
        Z_PARAM_ZVAL(host)
        Z_PARAM_LONG(port)
        Z_PARAM_ZVAL(cmd)
        Z_PARAM_ZVAL(message)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(respond)
        Z_PARAM_LONG(timeout)
    ZEND_PARSE_PARAMETERS_END();
#endif

    if (port <= 0) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "port must greater than zero");
        RETURN_NULL();
    }
    if (Z_TYPE_P(host) != IS_STRING || Z_STRLEN_P(host) <= 0) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "host must be string, not empty");
        RETURN_NULL();
    }
    if (Z_STRLEN_P(cmd) <= 0 || Z_STRLEN_P(cmd) >= 256) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "cmd length must in 1, 255");
        RETURN_NULL();
    }

    play_socket_ctx *sctx = NULL;
    timeout = timeout > 0 ? timeout : 1;
    if ((sctx = play_socket_connect(Z_STRVAL_P(host), port, timeout)) == NULL) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not connect %s:%d", Z_STRVAL_P(host), port);
        RETURN_NULL();
    }

    char request_id[33];
    play_get_micro_uqid(request_id, sctx->local_ip_hex, getpid());
    play_socket_send_with_protocol_v1(sctx, request_id, Z_STRVAL_P(cmd), Z_STRLEN_P(cmd), Z_STRVAL_P(message), Z_STRLEN_P(message), respond);
    if (respond) {
        play_socket_recv_with_protocol_v1(sctx);
        if (sctx->read_buf != NULL) {
            int data_len = 0;
            char reponse_id[33];
            reponse_id[32] = 0;

            memcpy(&data_len, sctx->read_buf+1, 4);
            memcpy(reponse_id, sctx->read_buf+5, 32);
            if (memcmp(reponse_id, request_id, 32) != 0) {
                play_socket_cleanup_with_protocol(sctx);
                play_interface_utils_trigger_exception(PLAY_ERR_BASE, "reponse_id != request_id %s:%s", request_id, reponse_id);
                RETURN_NULL();
            }
            if (data_len > sctx->read_buf_ncount) {
                play_socket_cleanup_with_protocol(sctx);
                play_interface_utils_trigger_exception(PLAY_ERR_BASE, "data len error:%d, %d", data_len, sctx->read_buf_ncount);
                RETURN_NULL();
            }

            ZVAL_STRINGL(return_value, sctx->read_buf+37, data_len);
            play_socket_cleanup_with_protocol(sctx);
        }
    }
}


//PHP_METHOD(NetKit, nativesocket)
//{
//    char *host = NULL;
//    char *message = NULL;
//    long port;
//    long timeout = 0;
//    int hostLen, messageLen;
//
//    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sls|l", &host, &hostLen, &port, &message, &messageLen, &timeout) == FAILURE) {
//        return;
//    }
//
//    int sockfd;
//    long ret;
//    long bufSize = 1024*640;
//    char buf[bufSize];
//    timeout = timeout ==  0 ? 3 : timeout;
//
//    if ((sockfd = play_socket_connect(host, port, timeout)) <= 0) {
//        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not connect %s:%d", host, port);
//        RETURN_NULL();
//    }
//    printf("get socket:%d\n", sockfd);
//
//    memset(buf, 0, bufSize);
//    if ((ret = play_socket_send_recv(sockfd, message, messageLen, buf)) <= 0) {
//        RETURN_NULL();
//    }
//
//    RETURN_STRINGL(buf, ret);
//}