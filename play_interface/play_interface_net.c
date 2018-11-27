//
// Created by Leo on 2018/10/16.
//


#include "../play_core/play_core.h"
#include "play_interface.h"
#include <stdio.h>
#include <zend_interfaces.h>

PHP_METHOD(Net, protocolsocket);
zend_class_entry *play_interface_net_ce;

const zend_function_entry  play_interface_net_functions[] = {
    PHP_ME(Net, protocolsocket, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};


void play_interface_net_register(int _module_number)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY(class, "Net", play_interface_net_functions);
    play_interface_net_ce = zend_register_internal_class_ex(&class, NULL);
}

PHP_METHOD(Net, protocolsocket)
{
    char *host = NULL;
    char *message = NULL;
    char *cmd = NULL;
    long port;
    long timeout = 0;
    char respond = 1;
    int hostLen, messageLen, cmd_len;

#ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "slssb|l", &host, &hostLen, &port, $cmd, &cmd_len, &message, &messageLen, &respond, &timeout) == FAILURE) {
        return;
    }
#else
    ZEND_PARSE_PARAMETERS_START(5, 6)
        Z_PARAM_STRING(host, hostLen)
        Z_PARAM_LONG(port)
        Z_PARAM_STRING(cmd, cmd_len)
        Z_PARAM_STRING(message, messageLen)
        Z_PARAM_BOOL(respond)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(timeout)
    ZEND_PARSE_PARAMETERS_END();
#endif

    play_socket_ctx *sctx = NULL;
    timeout = timeout > 0 ? timeout : 1;

    if (cmd_len <= 0 || cmd_len >= 256) {
        play_interface_utils_trigger_exception(-1, "cmd length must in 1, 255");
        RETURN_NULL();
    }
    if ((sctx = play_socket_connect(host, port, timeout)) == NULL) {
        play_interface_utils_trigger_exception(PLAY_ERR_BASE, "can not connect %s:%d", host, port);
        RETURN_NULL();
    }

    char request_id[33];
    play_get_micro_uqid(request_id, sctx->local_ip_hex, getpid());
    play_socket_send_with_protocol_v1(sctx, request_id, cmd, cmd_len, message, messageLen, respond);
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


//PHP_METHOD(Net, nativesocket)
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