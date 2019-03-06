#include <string.h>  /* strcpy */
#include <stdlib.h>  /* malloc */
#include <stdio.h>   /* printf */
#include <zconf.h>
#include "play_lib/uthash/uthash.h"
#include "play_core/play_core.h"
#include "play_lib/uthash/utarray.h"


void test_fastcgi()
{
    int size = 0;
    char buf[1024];
    bzero(buf, 1024);
    play_socket_ctx *client;
    client = play_socket_connect("10.211.55.16", 9000, 86400);
    size = play_fastcgi_start_request(buf);
    size += play_fastcgi_set_param(buf+size, "SCRIPT_FILENAME", strlen("SCRIPT_FILENAME"), "/media/psf/psvrfls/web/htdocs/index.php", strlen("/media/psf/psvrfls/web/htdocs/index.php"));
    size += play_fastcgi_set_param(buf+size, "REQUEST_METHOD", strlen("REQUEST_METHOD"), "POST", strlen("POST"));
    size += play_fastcgi_set_param(buf+size, "DOCUMENT_ROOT", strlen("DOCUMENT_ROOT"), "/media/psf/psvrfls/web/htdocs/", strlen("/media/psf/psvrfls/web/htdocs/"));
    size += play_fastcgi_set_param(buf+size, "REQUEST_URI", strlen("REQUEST_URI"), "/common/test.json", strlen("/common/test.json"));
    size += play_fastcgi_set_param(buf+size, "CONTENT_TYPE", strlen("CONTENT_TYPE"), "application/x-www-form-urlencoded", strlen("application/x-www-form-urlencoded"));
    size += play_fastcgi_set_param(buf+size, "CONTENT_LENGTH", strlen("CONTENT_LENGTH"), "17", strlen("17"));
    size += play_fastcgi_set_param(buf+size, "QUERY_STRING", strlen("QUERY_STRING"), "a=2", strlen("a=2"));
    size += play_fastcgi_end_request(buf+size);

    size += play_fastcgi_set_boby(buf+size, "a=20&b=10&c=5&d=6", 17);
    size += play_fastcgi_end_body(buf+size);

    int response_size = 0;
    char * response = NULL;
    response = play_fastcgi_send_request(client->socket_fd, buf, size, &response_size);
    printf("%s\n", response);
    if (response == NULL) {

    }
    response_size = strlen(response);
    int i = play_fastcgi_parse_head(response, response_size);
}

int main()
{
    test_fastcgi();
    // test_fastcgi();
}