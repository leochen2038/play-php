#include <string.h>  /* strcpy */
#include <stdlib.h>  /* malloc */
#include <stdio.h>   /* printf */
#include "play_lib/uthash/uthash.h"
#include "play_core/play_core.h"
#include "play_lib/uthash/utarray.h"


//struct my_struct {
//    const char *name;          /* key */
//    int id;
//    UT_hash_handle hh;         /* makes this structure hashable */
//};
//
//
//int main(int argc, char *argv[]) {
//    const char *names[] = { "joe", "bob", "betty", NULL };
//    struct my_struct *s, *tmp, *users = NULL;
//
//    for (int i = 0; names[i]; ++i) {
//        s = (struct my_struct *)malloc(sizeof *s);
//        s->name = names[i];
//        s->id = i;
//        HASH_ADD_KEYPTR( hh, users, s->name, strlen(s->name), s );
//    }
//
//    HASH_FIND_STR( users, "betty", s);
//    if (s) printf("betty's id is %d\n", s->id);
//
//    /* free the hash table contents */
//    HASH_ITER(hh, users, s, tmp) {
//        HASH_DEL(users, s);
//        free(s);
//    }
//    return 0;
//}

/* hash of hashes */
//typedef struct item {
//    char name[10];
//    struct item *sub;
//    int val;
//    UT_hash_handle hh;
//} item_t;
//
//item_t *items=NULL;
//
//int main(int argc, char *argvp[]) {
//    item_t *item1, *item2, *tmp1, *tmp2;
//
//    /* make initial element */
//    item_t *i = malloc(sizeof(*i));
//    strcpy(i->name, "bob");
//    i->sub = NULL;
//    i->val = 0;
//    HASH_ADD_STR(items, name, i);
//
//    /* add a sub hash table off this element */
//    item_t *s = malloc(sizeof(*s));
//    strcpy(s->name, "age");
//    s->sub = NULL;
//    s->val = 37;
//    HASH_ADD_STR(i->sub, name, s);
//
//    s = malloc(sizeof(*s));
//    strcpy(s->name, "id");
//    s->sub = NULL;
//    s->val = 40;
//    HASH_ADD_STR(i->sub, name, s);
//
//    int count = HASH_COUNT(i->sub);
//    printf("there are %d users\n", count);
//
//    item_t *j = NULL;
//    HASH_FIND_STR(i, "bob", j);
//
//    /* iterate over hash elements  */
////    HASH_ITER(hh, items, item1, tmp1) {
//        HASH_ITER(hh, j, item2, tmp2) {
//            printf("$items{%s}{%s} = %d\n", j->name, item2->name, item2->val);
//        }
////    }
//
//    /* clean up both hash tables */
//    HASH_ITER(hh, items, item1, tmp1) {
//        HASH_ITER(hh, item1->sub, item2, tmp2) {
//            HASH_DEL(item1->sub, item2);
//            free(item2);
//        }
//        HASH_DEL(items, item1);
//        free(item1);
//    }
//
//    return 0;
//}

int main()
{
    play_global_config_set_app_root("/Users/Leo/play7/soft", strlen("/Users/Leo/play7/soft"));
    play_meta_hashtable *item2, *tmp2;
    play_meta_hashtable *list = play_manager_meta_get_list_by_path("/Users/Leo/play7/soft/assets/meta");
    int count = HASH_COUNT(list);
    printf("there are %d meta\n", count);
//
    HASH_ITER(hh, list, item2, tmp2) {
        printf("$items{%s} = %s\n", item2->name, item2->meta->name->val);
    }
    play_meta *meta = play_manager_meta_get_by_chars("Redis_Config", 1);
    play_action *action1 = play_manager_action_get_by_chars("shop/goods/detail", 1);


//    play_global_config_set_app_root("/Users/Leo/play7/soft", strlen("/Users/Leo/play7/soft"));
//    play_action *action1 = play_manager_action_get_by_chars("shop/goods/detail", 1);
//    play_action *action = play_manager_action_get_by_chars("setting/bind", 1);
//
//
//    return 0;
}


//int main() {
//    UT_array *strs;
//    char *s, **p;
//
//    utarray_new(strs,&ut_str_icd);
//
//    s = "hello"; utarray_push_back(strs, &s);
//    s = "world"; utarray_push_back(strs, &s);
//    s = "world"; utarray_push_back(strs, &s);
//    p = NULL;
//    while ( (p=(char**)utarray_next(strs,p))) {
//        printf("%s\n",*p);
//    }
//
//    utarray_free(strs);
//
//    return 0;
//}