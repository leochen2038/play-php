//
// Created by Leo on 2018/10/16.
//

#include "play_core.h"
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <zconf.h>
#include "../play_lib/uthash/uthash.h"

#define PLAYSOCKET_NOTUSED(v) ((void)v)


//void debugLog(char *str) {
//    FILE *fp = NULL;
//    fp = fopen("/tmp/play-core.log", "a+");
//    int fd = fileno(fp);
//    fchmod(fd, 0777);
//    fputs(str, fp);
//    fclose(fp);
//}

play_socket_ctx *socket_hashtable = NULL;
size_t play_socket_send_with_protocol_v1(play_socket_ctx *sctx, char *request_id, const char *cmd, int cmd_len, const char *data, int data_len, char respond)
{
    /* 协议：4个字节(==>协议头),4个字节（数据长度）, 1个字节(协议版本), 1个字节(是否需要响应) 1个字节(命令字长度)，4个字节(内容长度), 32字节(请求唯一标识) */
    int ret = 0;
    char version = 0x01;
    int send_size = cmd_len + data_len + 47;
    int dataa_size = send_size - 8;
    char send_data[send_size];

    memcpy(send_data, "==>>", 4);
    memcpy(send_data+4, &dataa_size, 4);
    memcpy(send_data+8, &version, 1);
    memcpy(send_data+9, &respond, 1);
    memcpy(send_data+10, &cmd_len, 1);
    memcpy(send_data+11, &data_len, 4);
    memcpy(send_data+15, request_id, 32);
    memcpy(send_data+47, cmd, cmd_len);

    if (data_len > 0) {
        memcpy(send_data+47+cmd_len, data, data_len);
    }

    ret = php_stream_write(sctx->stream, send_data, send_size);

    if (ret != send_size) {
        return -ret-1000;
    }
    return ret;
}

size_t play_socket_send_with_protocol_v2(play_socket_ctx *sctx, unsigned short callerId, char *request_id, const char *cmd, int cmd_len, const char *data, int data_len, char respond)
{
    /* 协议：4个字节(==>协议头),4个字节（数据长度）, 1个字节(协议版本), 1个字节(是否需要响应), 2个字节(调用方ID),  1个字节(action长度), 4个字节(内容长度), 32字节(请求唯一标识), action, 内容 */
    int ret = 0;
    char version = 0x02;
    int send_size = cmd_len + data_len + 49;
    int dataa_size = send_size - 8;
    char send_data[send_size];

    memcpy(send_data, "==>>", 4);
    memcpy(send_data+4, &dataa_size, 4);
    memcpy(send_data+8, &version, 1);
    memcpy(send_data+9, &respond, 1);
    memcpy(send_data+10, &callerId, 2);
    memcpy(send_data+12, &cmd_len, 1);
    memcpy(send_data+13, &data_len, 4);
    memcpy(send_data+17, request_id, 32);
    memcpy(send_data+49, cmd, cmd_len);

    if (data_len > 0) {
        memcpy(send_data+49+cmd_len, data, data_len);
    }

    ret = php_stream_write(sctx->stream, send_data, send_size);

    if (ret != send_size) {
        return -ret-1000;
    }
    return ret;
}

size_t play_socket_send_with_protocol_v3(play_socket_ctx *sctx, int callerId, int tagId, char *trace_id, int span_id, const char *cmd, int cmd_len, const char *data, int data_len, char respond)
{
    /* 协议：4个字节(==>协议头), 4个字节（数据长度）, 1个字节(协议版本), 4个字节(tagId), 32字节(traceId) 16字节(spanId) | 4个字节(调用方ID), 1个字节(action长度), 1个字节(是否需要响应) , action, 内容 */
    int ret = 0;
    char version = 0x03;
    int send_size = 67 + cmd_len + data_len;
    int data_size = send_size - 8;
    char send_data[send_size];
    char fillZero[16] = {0};
    char debug[1024] = {0};
    fillZero[0] = (char)span_id;

    memcpy(send_data, "==>>", 4);
    memcpy(send_data+4, &data_size, 4);
    memcpy(send_data+8, &version, 1);
    memcpy(send_data+9, &tagId, 4);
    memcpy(send_data+13, trace_id, 32);
    memcpy(send_data+45, fillZero, 16);

    memcpy(send_data+61, &callerId, 4);
    memcpy(send_data+65, &cmd_len, 1);
    memcpy(send_data+66, &respond, 1);

    memcpy(send_data+67, cmd, cmd_len);

    if (data_len > 0) {
        memcpy(send_data+67+cmd_len, data, data_len);
    }

    ret = php_stream_write(sctx->stream, send_data, send_size);

    if (ret != send_size) {
        return -ret-1000;
    }
    return ret;
}

size_t play_socket_recv_with_protocol_v3(play_socket_ctx *sctx, char *trace_id, int timeout)
{
    int size, rcount;
    char header[8];
    rcount = php_stream_read(sctx->stream, header, 8);
    if (rcount < 1) {
        return -1;
    }

    if (memcmp(header, "<<==", 4) != 0) {
        return -2;
    }

    memcpy(&size, header+4, 4);
    sctx->read_buf = emalloc(size+1);
    sctx->read_buf[size] = 0;
    sctx->read_buf_ncount = size;
    sctx->read_buf_rcount = 0;
    rcount = php_stream_read(sctx->stream, sctx->read_buf, size);
    if (rcount != size) {
        return -3;
    }
    return 1;
}

void play_socket_cleanup_with_protocol(play_socket_ctx *sctx)
{
    if (sctx->read_buf != NULL) {
        efree(sctx->read_buf);
        sctx->read_buf = NULL;
        sctx->read_buf_ncount = 0;
        sctx->read_buf_rcount = 0;
    }
}

size_t play_socket_recv_with_protocol_v1(play_socket_ctx *sctx)
{
    int size, rcount;
    char header[8];

    rcount = socket_read(sctx->socket_fd, header, 8);
    if (rcount < 1) {
        return -1;
    }

    if (memcmp(header, "==>>", 4) != 0) {
        return -2;
    }

    memcpy(&size, header+4, 4);
    sctx->read_buf = malloc(size+1);
    sctx->read_buf[size] = 0;
    sctx->read_buf_ncount = size;
    sctx->read_buf_rcount = 0;

    rcount = php_stream_read(sctx->stream, sctx->read_buf, size);
    if (rcount != size) {
        return -3;
    }
    return 1;
}

size_t socket_read_timeout(int socketfd, char *buffer, int length, int timeout) {
    int readCount = 0;
    int nread = 0;
    time_t start, current;
    time(&start);

    while (readCount < length) {
        nread = recv(socketfd, buffer + readCount, length - readCount, MSG_WAITALL);
        if (nread > 0) {
            readCount += nread;
            continue;
        }

        if (nread <= 0 && !(errno == EINTR || errno == EAGAIN)) {
            return -1;
        }
        if (timeout > 0) {
            time(&current);
            if (current - start > timeout) {
                return -2;
            }
        }
    }

    return readCount;
}

size_t socket_read(int socketfd, char *buffer, int length) {
    int readCount = 0;
    int nread = 0;

    while (readCount < length) {
        nread = recv(socketfd, buffer + readCount, length - readCount, MSG_WAITALL);
        if (nread > 0) {
            readCount += nread;
            continue;
        }

        if (nread <= 0 && !(errno == EINTR || errno == EAGAIN)) {
            return -1;
        }
    }

    return readCount;
}



size_t play_socket_send(int socket_fd, const char *data, size_t len)
{
    size_t ret;
    ret = send(socket_fd, data, len, 0);
    if (ret != len) {
        // php_printf("send error\n");
        return ret;
    }
    return ret;
}

size_t play_socket_recv(int socket_fd, char *recvdata)
{
    size_t ret = 0;
    while ((ret = recv(socket_fd, recvdata, 1024*640, MSG_DONTWAIT)) > 0) {
        recvdata += ret;
        // php_printf("recv:%s\n",recvdata);
    }
    if (ret == 0) {
        // php_printf("连接断开");
    } else {
        // php_printf("服务超时");
    }
    return ret;
}

size_t play_socket_send_recv(int socket_fd, const char *send, int sendlen, char *recv)
{
    size_t ret = 0;
    ret = play_socket_send(socket_fd, send, sendlen);
    // php_printf("send ret:%ld\n", ret);
    if (ret == sendlen) {
        ret = play_socket_recv(socket_fd, recv);
        // printf("recv ret:%ld\n", ret);
        if (ret > 0) {
            return ret;
        }
    }
    return 0;
}

php_stream *play_socket_stream(const char *host, int host_len, int timeout, int persistent, char *persistent_id)
{
    int err = 0;
    int tcp_keepalive = 0;
    int tcp_flag = 1;

    struct timeval tv, read_tv, *tv_ptr = NULL;
    php_stream  *stream;
    php_netstream_data_t *sock;

    tv.tv_sec  = (time_t)timeout;
    tv.tv_usec = (int)((timeout - tv.tv_sec) * 1000000);
    if(tv.tv_sec != 0 || tv.tv_usec != 0) {
        tv_ptr = &tv;
    }

    read_tv.tv_sec  = (time_t)timeout;
    read_tv.tv_usec = (int)((timeout-read_tv.tv_sec)*1000000);
    if (persistent == 1) {
        stream = php_stream_xport_create(host, host_len, 0, STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT, persistent_id,
                                         tv_ptr, NULL, NULL, NULL);
    } else {
        stream = php_stream_xport_create(host, host_len, 0, STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT, NULL,
                                         tv_ptr, NULL, NULL, NULL);
    }

    if (!stream) {
        return NULL;
    }

    sock = (php_netstream_data_t*)stream->abstract;
    err = setsockopt(sock->socket, IPPROTO_TCP, TCP_NODELAY, (char*) &tcp_flag, sizeof(tcp_flag));
    PLAYSOCKET_NOTUSED(err);
    err = setsockopt(sock->socket, SOL_SOCKET, SO_KEEPALIVE, (char*) &tcp_keepalive, sizeof(tcp_keepalive));
    PLAYSOCKET_NOTUSED(err);

    php_stream_auto_cleanup(stream);

    if (read_tv.tv_sec != 0 || read_tv.tv_usec != 0) {
        php_stream_set_option(stream,PHP_STREAM_OPTION_READ_TIMEOUT, 0, &read_tv);
    }
    php_stream_set_option(stream, PHP_STREAM_OPTION_WRITE_BUFFER, PHP_STREAM_BUFFER_NONE, NULL);

    return stream;
}

play_socket_ctx *play_socket_connect(const char *host, int port, int wait_time, int persistent)
{
    char ipv4[22] = {0};
    play_socket_ctx *sctx = (play_socket_ctx *) calloc(1, sizeof *sctx);
    sctx->persistent = 0;
    if (persistent == 1) {
        sctx->persistent = 1;
        snprintf(sctx->persistent_id, 32, "playsocket:%s:%d", host, port);
    }
    snprintf(ipv4, 21, "%s:%d", host, port);

    sctx->stream = play_socket_stream(ipv4, strlen(ipv4), wait_time, sctx->persistent, sctx->persistent_id);
    if (sctx->stream == NULL) {
        free(sctx);
        sctx = NULL;
    }
    return sctx;
}

void play_socket_cleanup_and_close(play_socket_ctx *sctx)
{
    if (sctx->persistent) {
        php_stream_pclose(sctx->stream);
    } else {
        php_stream_close(sctx->stream);
    }

    if (sctx->read_buf != NULL) {
        efree(sctx->read_buf);
    }
    free(sctx);
}
