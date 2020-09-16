//
//  tool.c
//  play
//
//  Created by Leo on 17/12/13.
//  Copyright © 2017年 Leo. All rights reserved.
//

#include "play_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <zconf.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>

//typedef struct _play_hitem_project_path {
//    char path[128];
//    char *play_string;
//    UT_hash_handle hh;
//}play_hitem_project_path;
//static play_hitem_project_path *__play_project_path_hashtable = NULL;


static const unsigned char tolower_map[256] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
        0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
        0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
        0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
        0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
        0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
        0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
        0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
        0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
        0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
        0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
        0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
        0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
};
#define tolower_ascii(c) (tolower_map[(unsigned char)(c)])

void play_str_tolower_copy(char *dest, const char *source, int length)
{
    unsigned char *str = (unsigned char*)source;
    unsigned char *result =(unsigned char*)dest;
    unsigned char *end = str + length;

    while (str < end) {
        *result++ = tolower_ascii(*str++);
    }
    *result = '\0';
}

/**
 * 从当前目录开始往上级目录查找，直到根目录
 * 如果发现.play文件，则返回当前目录路径
 * 如果找不到，则返回NULL
 * @param path
 * @return play_string*
 */
play_string *play_find_project_root_by_path(const char *path, int cache)
{
    int path_len = strlen(path);
    char play_file[1024];
    sprintf(play_file, "%s/.play", path);

    while (path_len > 0) {
        if (access(play_file, F_OK) == 0) {
            return play_string_new_with_chars(play_file, strlen(play_file) - 6);
        }
        while (path_len > 0 && play_file[--path_len] != '/') {}
        memcpy(play_file+path_len, "/.play", 7); // 要把后面的结束符\0也有一起copy
    }
    return NULL;
}

/**
 * 注意要释放内存
 * @param dest
 * @param src
 * @param delim
 * @return
 */
int play_explode(char ***dest, const char *src, const char delim) {
    char  **parts;
    int cnt = 0, len = 0;
    
    size_t maxlen;
    maxlen = strlen(src);
    parts = (char **) malloc(sizeof(char *) * 1);
    parts[cnt] = NULL;
    
    size_t itemlen = 0;
    char item[maxlen];
    memset(item, 0, maxlen);
    
    while (*src != '\0') {
        if (*src == delim) {
            if (len > 0) {
                itemlen = strlen(item);
                parts[cnt] = (char *) malloc(sizeof(char) * itemlen+1);
                memcpy(parts[cnt], item, itemlen+1);
                memset(item, 0, maxlen);
                len = 0;
            }
            
            parts = (char **) realloc(parts, sizeof(char *) * (++cnt + 1));
            parts[cnt] = NULL;
        } else {
            item[len++] = *src;
        }
        src++;
    }
    
    itemlen = strlen(item);
    parts[cnt] = (char *) malloc(sizeof(char) * itemlen+1);
    memcpy(parts[cnt], item, itemlen+1);
    
    memcpy(dest, &parts, sizeof(parts));
    return (cnt > 0) ? (cnt + 1) : 0;
}

int play_is_numeric(char *str)
{
    int i;
    size_t length = strlen(str);
    if (length == 0) return 0;
    for (i = 0; i < length; ++i) {
        if(!isdigit(str[i]) && str[i] != '.') {
            return 0;
        }
    }
    return 1;
}


void play_get_micro_uqid(char *muqid, char *hexip, int pid)
{
    struct timeval tval;
    time_t timep;
    struct tm *p;

    time(&timep);
    gettimeofday(&tval, NULL);

    p = localtime(&timep);
    snprintf(muqid, 33, "100%02d%02d%02d%02d%02d%02d%05x%s%04x", (p->tm_year - 100), (p->tm_mon + 1), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, tval.tv_usec, hexip, pid%0x10000);
}

char * play_get_intranet_ip()
{
    struct ifaddrs *ifaddr, *ifa;
    char *host = NULL;

    if (getifaddrs(&ifaddr) == -1) {
        return NULL;
    }

    for (ifa = ifaddr; ifa != NULL && ifa->ifa_addr != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *in = (struct sockaddr_in*)ifa->ifa_addr;
            host = inet_ntoa(in->sin_addr);

            if (memcmp(host, "192.168", 7) == 0) {
                freeifaddrs(ifaddr);
                return host;
            }
        }
    }

    freeifaddrs(ifaddr);
    return NULL;
}