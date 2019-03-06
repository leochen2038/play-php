//
// Created by Leo on 2019/1/21.
//

#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <zconf.h>
#include <stdio.h>
#include "play_core.h"

typedef struct {
    unsigned char version;
    unsigned char type;
    unsigned char request_id_b1;
    unsigned char request_id_b0;
    unsigned char content_length_b1;
    unsigned char content_length_b0;
    unsigned char padding_length;
    unsigned char reserved;
} fcgi_header;

#define FCGI_HEADER_LENGTH  8
#define FCGI_VERSION_1      1

/* FastCGI header types */
#define FCGI_TYPE_BEGIN_REQUEST      1
#define FCGI_TYPE_ABORT_REQUEST      2
#define FCGI_TYPE_END_REQUEST        3
#define FCGI_TYPE_PARAMS             4
#define FCGI_TYPE_STDIN              5
#define FCGI_TYPE_STDOUT             6
#define FCGI_TYPE_STDERR             7
#define FCGI_TYPE_DATA               8
#define FCGI_TYPE_GET_VALUES         9
#define FCGI_TYPE_GET_VALUES_RESULT  10
#define FCGI_TYPE_MAX                11

typedef struct {
    unsigned char role_b1;
    unsigned char role_b0;
    unsigned char flags;
    unsigned char reserved[5];
} fcgi_begin_request_body;

typedef struct {
    fcgi_header header;
    fcgi_begin_request_body body;
} fcgi_begin_request_record;

/* application role types */
#define FCGI_ROLE_KEEP_CONNECTION  1
#define FCGI_ROLE_RESPONDER        1
#define FCGI_ROLE_AUTHORIZER       2
#define FCGI_ROLE_FILTER           3

typedef struct {
    unsigned char app_status_b3;
    unsigned char app_status_b2;
    unsigned char app_status_b1;
    unsigned char app_status_b0;
    unsigned char protocol_status;
    unsigned char reserved[3];
} fcgi_end_request_body;

static void play_fastcgi_init_header(fcgi_header *header, int type, int request_id, int content_length, int padding_length);
static void play_fastcgi_init_begin_request_body(fcgi_begin_request_body *body, int role, int keepalive);
static void play_fastcgi_init_kv_body(unsigned char *buf, int *len, unsigned char *key, int klen, unsigned char *val, int vlen);
static int fastcgi_read_header(int fd, fcgi_header *header);
static int fastcgi_read_body(int fd, char *buffer, int length, int padding);
static int fastcgi_read_end_request(int fd);

int play_fastcgi_start_request(play_socket_ctx *sctx)
{
    fcgi_begin_request_record record;
    play_fastcgi_init_header(&record.header, FCGI_TYPE_BEGIN_REQUEST, 0, sizeof(record.body), 0);
    play_fastcgi_init_begin_request_body(&record.body, FCGI_ROLE_RESPONDER, 1);

    if (send(sctx->socket_fd, (char *)&record, sizeof(record), 0) != sizeof(record)) {
        return -1;
    }
    return 0;
}

int play_fastcgi_end_request(play_socket_ctx *sctx)
{
    fcgi_header header;
    play_fastcgi_init_header(&header, FCGI_TYPE_PARAMS, 0, 0, 0);
    if (send(sctx->socket_fd, &header, FCGI_HEADER_LENGTH, 0) != FCGI_HEADER_LENGTH) {
        return -1;
    }
    return 0;
}

int play_fastcgi_set_param(play_socket_ctx *sctx, char *key, int klen, char *val, int vlen)
{
    int bodylen = 0;
    char kvbody[256];
    bzero(kvbody, 256);
    fcgi_header header;

    play_fastcgi_init_kv_body(kvbody, &bodylen, key, klen, val, vlen);
    play_fastcgi_init_header(&header, FCGI_TYPE_PARAMS, 0, bodylen, 0);

    if (send(sctx->socket_fd, &header, FCGI_HEADER_LENGTH, 0) != FCGI_HEADER_LENGTH) {
        return -1;
    }
    if (send(sctx->socket_fd, kvbody, bodylen, 0) != bodylen) {
        return -2;
    }
    return 0;
}

int play_fastcgi_set_boby(play_socket_ctx *sctx, char *body, int len)
{
    int pack_size;
    fcgi_header header;
    do {
        pack_size = len > 0xffff ? 0xffff : len;
        play_fastcgi_init_header(&header, FCGI_TYPE_STDIN, 0, pack_size, 0);
        if (send(sctx->socket_fd, &header, FCGI_HEADER_LENGTH, 0) != FCGI_HEADER_LENGTH) {
            return -1;
        }
        if (write(sctx->socket_fd, body, pack_size) != pack_size) {
            return -2;
        }
        body = body + pack_size;
        len -= pack_size;
    } while (len > 0);

    play_fastcgi_init_header(&header, FCGI_TYPE_STDIN, 0, 0, 0);
    if (write(sctx->socket_fd, &header, FCGI_HEADER_LENGTH) != FCGI_HEADER_LENGTH) {
        return -3;
    }
    return 0;
}


int play_fastcgi_parse_head(char *data, int data_size)
{
    int i = 0;
    char *p = data;
    for (i = 0; i < data_size; i++, ++p) {
        if (*p == '\r' && *(p+1) == '\n' && *(p+2) == '\r' && *(p+3) == '\n') {
            return i+4;
        }
    }
    return 0;
}

unsigned char * play_fastcgi_get_response(play_socket_ctx *sctx, int *response_size)
{
    unsigned char *response = NULL;
    int total = 0;
    int exit_flag = 0;
    fcgi_header response_header;

    while (!exit_flag) {
        if (fastcgi_read_header(sctx->socket_fd, &response_header) == -1) {
            return NULL;
        }

        switch (response_header.type) {
            case FCGI_TYPE_STDOUT:
            case FCGI_TYPE_STDERR: {
                int length, padding;
                int ret;

                length = (response_header.content_length_b1 << 8) + response_header.content_length_b0;
                padding = response_header.padding_length;
                total += length; /* total bytes */

                if (!response) {
                    response = malloc(total);
                } else {
                    response = realloc(response, total);
                }

                ret = fastcgi_read_body(sctx->socket_fd, response + (total - length), length, padding);
                if (ret == -1) {
                    return NULL;
                }
                break;
            }

            case FCGI_TYPE_END_REQUEST: {
                if (fastcgi_read_end_request(sctx->socket_fd) == -1) {
                    return NULL;
                }
                exit_flag = 1;
                break;
            }
        }
    }
    *response_size = total;
    return response;
}



static void play_fastcgi_init_kv_body(unsigned char *buf, int *len, unsigned char *key, int klen, unsigned char *val, int vlen)
{
    unsigned char *last = buf;

    if (klen < 128) {
        *last++ = (unsigned char)klen;
    } else {
        *last++ = (unsigned char)((klen >> 24) | 0x80);
        *last++ = (unsigned char)(klen >> 16);
        *last++ = (unsigned char)(klen >> 8);
        *last++ = (unsigned char)klen;
    }

    if (vlen < 128) {
        *last++ = (unsigned char)vlen;
    } else {
        *last++ = (unsigned char)((vlen >> 24) | 0x80);
        *last++ = (unsigned char)(vlen >> 16);
        *last++ = (unsigned char)(vlen >> 8);
        *last++ = (unsigned char)vlen;
    }

    memcpy(last, key, klen); last += klen;
    memcpy(last, val, vlen); last += vlen;

    *len = last - buf;
}

static void play_fastcgi_init_header(fcgi_header *header, int type, int request_id, int content_length, int padding_length)
{
    header->version = FCGI_VERSION_1;
    header->type = (unsigned char)type;
    header->request_id_b1 = (unsigned char)((request_id >> 8) & 0xFF);
    header->request_id_b0 = (unsigned char)(request_id & 0xFF);
    header->content_length_b1 = (unsigned char)((content_length >> 8) & 0xFF);
    header->content_length_b0 = (unsigned char)(content_length & 0xFF);
    header->padding_length = padding_length;
    header->reserved = 0;
}

static void play_fastcgi_init_begin_request_body(fcgi_begin_request_body *body, int role, int keepalive)
{
    bzero(body, sizeof(*body));
    body->role_b1 = (unsigned char)((role >> 8) & 0xFF);
    body->role_b0 = (unsigned char)(role & 0xFF);
    body->flags = (unsigned char)(keepalive ? 1 : 0);
}

static int fastcgi_read_header(int fd, fcgi_header *header)
{
    if (read(fd, header, FCGI_HEADER_LENGTH) != FCGI_HEADER_LENGTH) {
        return -1;
    }

    return 0;
}

static int fastcgi_read_body(int fd, char *buffer, int length, int padding)
{
    int nread;
    char temp[8];

    nread = read(fd, buffer, length);
    if (nread != length) {
        return -1;
    }

    if (padding > 0 && padding <= 8) {
        nread = read(fd, temp, padding);
        if (nread != padding) {
            return -1;
        }
    }

    return 0;
}

static int fastcgi_read_end_request(int fd)
{
    fcgi_end_request_body body;

    if (read(fd, &body, sizeof(body)) != sizeof(body)) {
        return -1;
    }

    return 0;
}