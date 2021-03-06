//
// Created by Leo on 2018/10/16.
//
#include "../play_core/play_core.h"
#include "play_interface.h"
#include <stdio.h>
#include <zend_interfaces.h>

PHP_METHOD(NetKit, socket_protocol);
PHP_METHOD(NetKit, socket_protocol_v2);
PHP_METHOD(NetKit, socket_protocol_v3);
PHP_METHOD(NetKit, socket_fastcgi);

zend_class_entry *play_interface_netkit_ce;

const zend_function_entry  play_interface_net_functions[] = {
    PHP_ME(NetKit, socket_protocol, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(NetKit, socket_protocol_v2, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(NetKit, socket_protocol_v3, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
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
    long timeout = 0;
    unsigned char respond = 1;
    zval *host, *params, *body;

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
    zval *z_item;
    zend_ulong idx;
    zend_string *z_key;

    play_socket_ctx *client;
    timeout = timeout > 0 ? timeout : 1;
    if ((client = play_socket_connect(Z_STRVAL_P(host), port, timeout, 0)) == NULL) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not connect %s:%d", Z_STRVAL_P(host), port);
        RETURN_NULL();
    }

    if (play_fastcgi_start_request(client) != 0) {
        play_socket_cleanup_and_close(client);
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "NetKit::socket_fastcgi() can not send start request to %s:%d", Z_STRVAL_P(host), port);
        RETURN_NULL();
    }
    int count = zend_hash_num_elements(Z_ARRVAL_P(params));
    zend_hash_internal_pointer_reset(Z_ARRVAL_P(params));
    for (i = 0; i < count; i ++) {
        z_item = zend_hash_get_current_data(Z_ARRVAL_P(params));
        zend_hash_get_current_key(Z_ARRVAL_P(params), &z_key, &idx);
        if (play_fastcgi_set_param(client, z_key->val, z_key->len, Z_STRVAL_P(z_item), Z_STRLEN_P(z_item)) != 0 ) {
            play_socket_cleanup_and_close(client);
            play_interface_utils_trigger_exception(PLAY_ERR_BASE, "NetKit::socket_fastcgi() can not send param to %s:%d", Z_STRVAL_P(host), port);
            RETURN_NULL();
        }
        zend_hash_move_forward(Z_ARRVAL_P(params));
    }

    if (play_fastcgi_end_request(client) != 0) {
        play_socket_cleanup_and_close(client);
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "NetKit::socket_fastcgi() can not send end request to %s:%d", Z_STRVAL_P(host), port);
        RETURN_NULL();
    }
    if (Z_STRLEN_P(body) > 0 && play_fastcgi_set_boby(client, Z_STRVAL_P(body), Z_STRLEN_P(body)) != 0) {
        play_socket_cleanup_and_close(client);
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "NetKit::socket_fastcgi() can not send body to %s:%d", Z_STRVAL_P(host), port);
        RETURN_NULL();
    }

    if (respond) {
        int response_size = 0;
        char *response = NULL;
        response = play_fastcgi_get_response(client, &response_size);
        play_socket_cleanup_and_close(client);
        if (response == NULL && response_size == 0) {
            play_interface_utils_trigger_exception(PLAY_ERR_BASE, "NetKit::socket_fastcgi() can not get response from %s:%d", Z_STRVAL_P(host), port);
            RETURN_NULL();
        }
        i = play_fastcgi_parse_head(response, response_size);
        ZVAL_STRINGL(return_value, response + i, response_size - i);
        free(response);
    } else {
        play_socket_cleanup_and_close(client);
    }
}

PHP_METHOD(NetKit, socket_protocol)
{
    int result;
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
    if ((sctx = play_socket_connect(Z_STRVAL_P(host), port, timeout, 1)) == NULL) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not connect %s:%d", Z_STRVAL_P(host), port);
        RETURN_NULL();
    }

    char request_id[33];
    play_get_micro_uqid(request_id, sctx->local_ip_hex, getpid());
    play_socket_send_with_protocol_v1(sctx, request_id, Z_STRVAL_P(cmd), Z_STRLEN_P(cmd), Z_STRVAL_P(message), Z_STRLEN_P(message), respond);
    if (respond) {
        result = play_socket_recv_with_protocol_v1(sctx);
        if (result < 0 || sctx->read_buf == NULL ) {
            play_socket_cleanup_and_close(sctx);
            play_interface_utils_trigger_exception(PLAY_ERR_BASE, "response error:%d", result);
            RETURN_NULL();
        } else {
            int data_len = 0;
            char reponse_id[33];
            reponse_id[32] = 0;

            memcpy(&data_len, sctx->read_buf+1, 4);
            memcpy(reponse_id, sctx->read_buf+5, 32);

            if (memcmp(reponse_id, request_id, 32) != 0) {
                play_socket_cleanup_and_close(sctx);
                play_interface_utils_trigger_exception(PLAY_ERR_BASE, "reponse_id != request_id %s:%s", request_id, reponse_id);
                RETURN_NULL();
            }

            if (data_len > sctx->read_buf_ncount) {
                play_socket_cleanup_and_close(sctx);
                play_interface_utils_trigger_exception(PLAY_ERR_BASE, "data len error:%d, %d", data_len, sctx->read_buf_ncount);
                RETURN_NULL();
            }

            ZVAL_STRINGL(return_value, sctx->read_buf+37, data_len);
            play_socket_cleanup_with_protocol(sctx);
        }
    }
}

PHP_METHOD(NetKit, socket_protocol_v2)
{
    long callerId = 0;
    int result;
    long port = 0;
    long timeout = 0;
    unsigned char respond = 1;
    zval *host, *cmd, *message;

    #ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "lzlzz|bl", &callerId, &host, &port, $cmd, &message, &respond, &timeout) == FAILURE) {
        return;
    }
    #else
    ZEND_PARSE_PARAMETERS_START(5, 7)
            Z_PARAM_LONG(callerId)
            Z_PARAM_ZVAL(host)
            Z_PARAM_LONG(port)
            Z_PARAM_ZVAL(cmd)
            Z_PARAM_ZVAL(message)
            Z_PARAM_OPTIONAL
            Z_PARAM_BOOL(respond)
            Z_PARAM_LONG(timeout)
        ZEND_PARSE_PARAMETERS_END();
    #endif

    if (callerId <= 0 || callerId > 65535) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "callerId length must in 1, 65535");
        RETURN_NULL();
    }
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
    if ((sctx = play_socket_connect(Z_STRVAL_P(host), port, timeout, 1)) == NULL) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not connect %s:%d", Z_STRVAL_P(host), port);
        RETURN_NULL();
    }

    char request_id[33];
    play_get_micro_uqid(request_id, sctx->local_ip_hex, getpid());
    play_socket_send_with_protocol_v2(sctx, (unsigned short)callerId, request_id, Z_STRVAL_P(cmd), Z_STRLEN_P(cmd), Z_STRVAL_P(message), Z_STRLEN_P(message), respond);

    if (respond) {
        result = play_socket_recv_with_protocol_v1(sctx);
        if (result < 0 || sctx->read_buf == NULL ) {
            play_socket_cleanup_and_close(sctx);
            play_interface_utils_trigger_exception(PLAY_ERR_BASE, "response error:%d", result);
            RETURN_NULL();
        } else {
            int data_len = 0;
            char reponse_id[33];
            reponse_id[32] = 0;

            memcpy(&data_len, sctx->read_buf+1, 4);
            memcpy(reponse_id, sctx->read_buf+5, 32);
            if (memcmp(reponse_id, request_id, 32) != 0) {
                play_socket_cleanup_and_close(sctx);
                play_interface_utils_trigger_exception(PLAY_ERR_BASE, "reponse_id != request_id %s:%s", request_id, reponse_id);
                RETURN_NULL();
            }

            if (data_len > sctx->read_buf_ncount) {
                play_socket_cleanup_and_close(sctx);
                play_interface_utils_trigger_exception(PLAY_ERR_BASE, "data len error:%d, %d", data_len, sctx->read_buf_ncount);
                RETURN_NULL();
            }

            ZVAL_STRINGL(return_value, sctx->read_buf+37, data_len);
            play_socket_cleanup_with_protocol(sctx);
        }
    }
}


PHP_METHOD(NetKit, socket_protocol_v3)
{
    long callerId = 0;
    int result;
    long tagId = 0;
    long port = 0;
    long timeout = 0;
    unsigned char respond = 1;
    char *traceId = NULL;
    long spanId = 0;
    zval *host, *cmd, *message;

    #ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "zlzz|llbl", &host, &port, $cmd, &message, &callerId, &tagId, &respond, &timeout) == FAILURE) {
        return;
    }
    #else
    ZEND_PARSE_PARAMETERS_START(4, 8)
                Z_PARAM_ZVAL(host)
                Z_PARAM_LONG(port)
                Z_PARAM_ZVAL(cmd)
                Z_PARAM_ZVAL(message)
                Z_PARAM_OPTIONAL
                Z_PARAM_LONG(callerId)
                Z_PARAM_LONG(tagId)
                Z_PARAM_BOOL(respond)
                Z_PARAM_LONG(timeout)
            ZEND_PARSE_PARAMETERS_END();
    #endif

    if (callerId <= 0 || callerId > 65535) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "callerId length must in 1, 65535");
        RETURN_NULL();
    }
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
    if ((sctx = play_socket_connect(Z_STRVAL_P(host), port, timeout, 1)) == NULL) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not connect %s:%d", Z_STRVAL_P(host), port);
        RETURN_NULL();
    }

    // get trackId
    zval *zTraceId = zend_read_static_property(play_interface_action_ce, "trackId", 7, 1);
    if (zTraceId == NULL || Z_STRLEN_P(zTraceId) == 0) {
        char tmpTrackId[33] = {0};
        play_get_micro_uqid(tmpTrackId, local_ip_hex, getpid());
        zend_update_static_property_stringl(play_interface_action_ce, "trackId", 7, tmpTrackId , strlen(tmpTrackId));
        zTraceId = zend_read_static_property(play_interface_action_ce, "trackId", 7, 1);
    }
    traceId = Z_STRVAL_P(zTraceId);

    // get spanId
    zval *zSpanId = zend_read_static_property(play_interface_action_ce, "spanId", 6, 1);
    spanId = Z_LVAL_P(zSpanId);
    spanId = spanId > 255 ? 255 : spanId + 1;
    zend_update_static_property_long(play_interface_action_ce, "spanId", 6, spanId);

    result = play_socket_send_with_protocol_v3(sctx, callerId, tagId, traceId, spanId, Z_STRVAL_P(cmd), Z_STRLEN_P(cmd), Z_STRVAL_P(message), Z_STRLEN_P(message), respond);
    if (result < 0) {
        play_socket_cleanup_and_close(sctx);
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "send error:%d", result);
        RETURN_NULL();
    }
    if (respond) {
        result = play_socket_recv_with_protocol_v3(sctx, traceId, timeout);
        // try agent
        if (result < 0) {
            play_socket_cleanup_and_close(sctx);
            if ((sctx = play_socket_connect(Z_STRVAL_P(host), port, timeout, 1)) == NULL) {
                play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not connect %s:%d", Z_STRVAL_P(host), port);
                RETURN_NULL();
            }
            result = play_socket_send_with_protocol_v3(sctx, callerId, tagId, traceId, spanId, Z_STRVAL_P(cmd), Z_STRLEN_P(cmd), Z_STRVAL_P(message), Z_STRLEN_P(message), respond);
            if (result < 0) {
                play_socket_cleanup_and_close(sctx);
                play_interface_utils_trigger_exception(PLAY_ERR_BASE, "send agent error:%d", result);
                RETURN_NULL();
            }
            result = play_socket_recv_with_protocol_v3(sctx, traceId, timeout);
        }

        if (result < 0 || sctx->read_buf == NULL ) {
            play_socket_cleanup_and_close(sctx);
            play_interface_utils_trigger_exception(PLAY_ERR_BASE, "response error:%d", result);
            RETURN_NULL();
        } else {
            int message = sctx->read_buf_ncount - 41;
            char reponse_id[33];
            reponse_id[32] = 0;

            memcpy(reponse_id, sctx->read_buf+5, 32);
            if (memcmp(reponse_id, traceId, 32) != 0) {
                play_socket_cleanup_and_close(sctx);
                play_interface_utils_trigger_exception(PLAY_ERR_BASE, "reponse_id != request_id %s:%s", traceId, reponse_id);
                RETURN_NULL();
            }

            ZVAL_STRINGL(return_value, sctx->read_buf+41, message);
            play_socket_cleanup_with_protocol(sctx);
        }
    }
}

//PHP_METHOD(NetKit, nativesocket)
//{
//    char *persistent_id = NULL;
//    int tcp_keepalive = 0;
//    int tcp_flag = 1;
//    struct timeval tv, read_tv, *tv_ptr = NULL;
//    php_stream     *stream;
//    char *message = NULL;
//    long port;
//    long timeout = 0;
//    int messageLen;
//    zend_string *estr = NULL;
//    php_netstream_data_t *sock;
//    zval *host;
//    int err = 0;
//    int host_len;
//    if (zend_parse_parameters(ZEND_NUM_ARGS(), "zls|l", &host, &port, &message, &messageLen, &timeout) == FAILURE) {
//        return;
//    }
//
//    spprintf(&persistent_id, 0, "playsocket:%s", Z_STRVAL_P(host));
//
//    tv.tv_sec  = (time_t)timeout;
//    tv.tv_usec = (int)((timeout - tv.tv_sec) * 1000000);
//    if(tv.tv_sec != 0 || tv.tv_usec != 0) {
//        tv_ptr = &tv;
//    }
//
//    read_tv.tv_sec  = (time_t)timeout;
//    read_tv.tv_usec = (int)((timeout-read_tv.tv_sec)*1000000);
//
//    stream = php_stream_xport_create(Z_STRVAL_P(host), Z_STRLEN_P(host), 0, STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT, persistent_id, tv_ptr, NULL, &estr, &err);
//
//    if (persistent_id) {
//        efree(persistent_id);
//    }
//
//    if (!stream) {
//        if (estr) {
//            zend_string_release(estr);
//            printf("%s", ZSTR_VAL(estr));
//        }
//        return;
//    }
//
//    sock = (php_netstream_data_t*)stream->abstract;
//    err = setsockopt(sock->socket, IPPROTO_TCP, TCP_NODELAY, (char*) &tcp_flag, sizeof(tcp_flag));
//    PHPREDIS_NOTUSED(err);
//    err = setsockopt(sock->socket, SOL_SOCKET, SO_KEEPALIVE, (char*) &tcp_keepalive, sizeof(tcp_keepalive));
//    PHPREDIS_NOTUSED(err);
//
//    //php_stream_auto_cleanup(stream);
//
//    if (read_tv.tv_sec != 0 || read_tv.tv_usec != 0) {
//        php_stream_set_option(stream,PHP_STREAM_OPTION_READ_TIMEOUT, 0, &read_tv);
//    }
//    php_stream_set_option(stream, PHP_STREAM_OPTION_WRITE_BUFFER, PHP_STREAM_BUFFER_NONE, NULL);



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