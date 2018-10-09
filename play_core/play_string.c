//
// Created by Leo on 18/5/3.
//

#include "play_core.h"
#include <string.h>
#include <stdlib.h>

play_string * play_string_new_with_size(int size)
{
    play_string *string = calloc(1, sizeof(play_string) + size);
    string->len = 0;
    string->size = size+1;
    string->val[0] = 0;
    return string;
}

play_string* play_string_new_with_chars(const char *chars, const int length)
{
    play_string *string = malloc(sizeof(play_string) + length);
    string->len = length;
    string->size = length+1;
    memcpy(string->val, chars, length);
    string->val[length] = 0;
    return string;
}

int play_string_append(play_string *str, const char *chars, const int length)
{
    if ((str->len + length) < str->size) {
        memcpy(str->val+str->len, chars, length);
        str->len += length;
        return 0;
    }
    return -1;
}

int play_string_reset(play_string *string)
{
    string->len = 0;
    memset(string->val, 0, string->size);
    return 0;
}


int play_string_free(play_string *string)
{
    if (string != NULL) {
        free(string);
    }
    return 0;
}