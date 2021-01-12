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

void debugLog(char *str) {
    FILE *fp = NULL;
    fp = fopen("/tmp/play-core.log", "a+");
    int fd = fileno(fp);
    fchmod(fd, 0777);
    fputs(str, fp);
    fclose(fp);
}

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

    ret = send(sctx->socket_fd, send_data, send_size, 0);

    if (ret != send_size) {
        // php_printf("send error\n");
        return ret;
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

    ret = send(sctx->socket_fd, send_data, send_size, 0);

    if (ret != send_size) {
        return ret;
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

    ret = send(sctx->socket_fd, send_data, send_size, 0);
    sprintf(debug, "send traceId:%s, ret:%d, errno:%d\n", trace_id, ret, errno);
    debugLog(debug);
    if (ret != send_size) {
        return -errno-1000;
    }
    return ret;
}

size_t play_socket_recv_with_protocol_v3(play_socket_ctx *sctx, char *trace_id, int timeout)
{
    char debug[1024] = {0};
    int size, rcount, result;
    char header[8];
    rcount = socket_read_timeout(sctx->socket_fd, header, 8, timeout);
    sprintf(debug, "recv traceId:%s, ret:%d, errno:%d\n", trace_id, rcount, errno);
    debugLog(debug);
    if (rcount < 1) {
        return -errno - 1000;
    }

    if (memcmp(header, "<<==", 4) != 0) {
        return -1;
    }

    memcpy(&size, header+4, 4);
    sctx->read_buf = malloc(size+1);
    sctx->read_buf[size] = 0;
    sctx->read_buf_ncount = size;
    sctx->read_buf_rcount = 0;
    result = socket_read_timeout(sctx->socket_fd, sctx->read_buf, size, timeout);
    if (result != size) {
        return -2;
    }
    return 1;
}

void play_socket_cleanup_with_protocol(play_socket_ctx *sctx)
{
    if (sctx->read_buf != NULL) {
        free(sctx->read_buf);
        sctx->read_buf = NULL;
        sctx->read_buf_ncount = 0;
        sctx->read_buf_rcount = 0;
    }
}

size_t play_socket_recv_with_protocol_v1(play_socket_ctx *sctx)
{
    int size, rcount, result;
    char header[8];

    rcount = socket_read(sctx->socket_fd, header, 8);
    if (rcount < 1) {
        return -errno - 1000;
    }

    if (memcmp(header, "==>>", 4) != 0) {
        return -1;
    }

    memcpy(&size, header+4, 4);
    sctx->read_buf = malloc(size+1);
    sctx->read_buf[size] = 0;
    sctx->read_buf_ncount = size;
    sctx->read_buf_rcount = 0;

    result = socket_read(sctx->socket_fd, sctx->read_buf, size);
    if (result != size) {
        return -2;
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

play_socket_ctx *play_socket_connect(const char *host, int port, int wait_time, int persisent)
{
    int needConnect = 1;
    char cipv4[21];
    char checkSocket;
    int socket_fd = 0;
    play_socket_ctx *sctx = NULL;

    snprintf(cipv4, 21, "%s:%d", host, port);
    if (persisent) {
        HASH_FIND_STR(socket_hashtable, cipv4, sctx);
        if (sctx != NULL) {
            char recvdata[1024];
            socket_fd = sctx->socket_fd;
            play_socket_cleanup_with_protocol(sctx);
            while (recv(socket_fd, recvdata, 1024, MSG_DONTWAIT) > 0) {}
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
                needConnect = 0;
            }
        }
    }

    if (needConnect == 1) {
        struct sockaddr_in servaddr;
        if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
            // printf("创建网络连接失败, socket error!\n");
            return NULL;
        };

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
        if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0 ) {
            // php_printf("创建网络连接失败, inet_pton error!\n");
            return NULL;
        }

        if (connect(socket_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            // php_printf("连接到服务器失败, connect error!\n");
            return NULL;
        }

        if (sctx == NULL) {
            char **ip_piece;
            char *local_ip = inet_ntoa(servaddr.sin_addr);
            play_explode(&ip_piece, local_ip, '.');

            sctx = (play_socket_ctx *) calloc(1, sizeof *sctx);
            strcpy(sctx->ipv4, cipv4);
            snprintf(sctx->local_ip_hex, 9, "%02x%02x%02x%02x", atoi(ip_piece[0]), atoi(ip_piece[1]), atoi(ip_piece[2]), atoi(ip_piece[3]));
        }

        sctx->socket_fd = socket_fd;
        if (persisent) {
            HASH_ADD_STR(socket_hashtable, ipv4, sctx);
        }
    }

    // 设置超时时间
    if (wait_time > 0) {
        struct timeval timeout = {wait_time, 0};
        setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
        setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));
    }
    return sctx;
}

void play_socket_cleanup_and_close(play_socket_ctx *sctx, int persisent)
{
    if (persisent) {
        HASH_DEL(socket_hashtable, sctx);
    }
    close(sctx->socket_fd);
    if (sctx->read_buf != NULL) {
        free(sctx->read_buf);
    }
    free(sctx);
}
