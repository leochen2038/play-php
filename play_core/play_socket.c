//
// Created by Leo on 2018/10/16.
//

#include "play_core.h"
#include <php.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../play_lib/uthash/uthash.h"

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
        php_printf("send error\n");
        return ret;
    }
    return ret;
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
    int size, rcount;
    char header[4];
    rcount = read(sctx->socket_fd, header, 4);

    if (memcmp(header, "==>>", 4) != 0) {
        return -1;
    }

    rcount = read(sctx->socket_fd, &size, 4);
    sctx->read_buf = malloc(size+1);
    sctx->read_buf[size] = 0;
    sctx->read_buf_ncount = size;
    sctx->read_buf_rcount = 0;

    while ((rcount = read(sctx->socket_fd, sctx->read_buf+sctx->read_buf_rcount, size))) {
        if (rcount > 0) {
            sctx->read_buf_rcount += rcount;
            if (sctx->read_buf_rcount >= sctx->read_buf_ncount) {
                break;
            } else {
                size -= rcount;
            }
        } else if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
            php_printf("\nwait\n");
            usleep(10);
        } else {
            // 读取错误
            php_printf("read error\n");
        }
    }
}



size_t play_socket_send(int socket_fd, const char *data, size_t len)
{
    size_t ret;
    ret = send(socket_fd, data, len, 0);
    if (ret != len) {
        php_printf("send error\n");
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
        php_printf("连接断开");
    } else {
        php_printf("服务超时");
    }
    return ret;
}

size_t play_socket_send_recv(int socket_fd, const char *send, int sendlen, char *recv)
{
    size_t ret = 0;
    ret = play_socket_send(socket_fd, send, sendlen);
    php_printf("send ret:%ld\n", ret);
    if (ret == sendlen) {
        ret = play_socket_recv(socket_fd, recv);
        php_printf("recv ret:%ld\n", ret);
        if (ret > 0) {
            return ret;
        }
    }
    return 0;
}

play_socket_ctx *play_socket_connect(const char *host, int port, int wait_time)
{
    int needConnect = 1;
    char cipv4[21];
    int socket_fd = 0;
    play_socket_ctx *sctx = NULL;

    snprintf(cipv4, 21, "%s:%d", host, port);
    HASH_FIND_STR(socket_hashtable, cipv4, sctx);
    if (sctx != NULL) {
        char recvdata[1024];
        socket_fd = sctx->socket_fd;
        play_socket_cleanup_with_protocol(sctx);
        while (recv(socket_fd, recvdata, 1024, MSG_DONTWAIT) > 0) {
            // php_printf("cache data:%s\n", recvdata);
        }
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
            // php_printf("use coonect\n");
            needConnect = 0;
        }
    }

    if (needConnect == 1) {
        // php_printf("new coonect\n");
        struct sockaddr_in servaddr;
        if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
            // php_printf("创建网络连接失败, socket error!\n");
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
        HASH_ADD_STR(socket_hashtable, ipv4, sctx);
    }

    // 设置超时时间
    if (wait_time > 0) {
        struct timeval timeout = {wait_time, 0};
        setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
        setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));
    }
    return sctx;
}