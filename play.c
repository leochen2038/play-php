/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2018 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_play.h"
#include "play_interface/play_interface.h"
#include "play_core/play_core.h"

static int set_root_path();

/* If you declare any globals in php_play.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(play)
*/

/* True global resources - no need for thread safety here */
static int le_play;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("play.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_play_globals, play_globals)
    STD_PHP_INI_ENTRY("play.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_play_globals, play_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_play_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_play_compiled)
{
	char *arg = NULL;
	size_t arg_len, len;
	zend_string *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	strg = strpprintf(0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "play", arg);

	RETURN_STR(strg);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_play_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_play_init_globals(zend_play_globals *play_globals)
{
	play_globals->global_value = 0;
	play_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(play)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/
    play_interface_action_register(module_number);
    play_interface_context_register(module_number);
    play_interface_crontab_register(module_number);
    play_interface_db_register(module_number);
    play_interface_input_register(module_number);
    play_interface_meta_router_abstract_register(module_number);
    play_interface_output_register(module_number);
    play_interface_play_register(module_number);
    play_interface_processor_register(module_number);
    play_interface_query_register(module_number);
    play_interface_render_abstract_register(module_number);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(play)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
    play_interface_utils_clean_crontab_mutex_file();
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(play)
{
#if defined(COMPILE_DL_PLAY) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	play_gloabl_config_init();
	if (set_root_path() != 0) {
		// php_printf("can not find play project in current path\n");
	}
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(play)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(play)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "play support", "enabled");
	php_info_print_table_row(2, "Version", PHP_PLAY_VERSION);
	php_info_print_table_end();

//	for (int i = 0; i < 2; i++) {
//	    ;
//	}
	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ play_functions[]
 *
 * Every user visible function must have an entry in play_functions[].
 */
const zend_function_entry play_functions[] = {
	PHP_FE(confirm_play_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in play_functions[] */
};
/* }}} */

/* {{{ play_module_entry
 */
zend_module_entry play_module_entry = {
	STANDARD_MODULE_HEADER,
	"play",
	play_functions,
	PHP_MINIT(play),
	PHP_MSHUTDOWN(play),
	PHP_RINIT(play),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(play),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(play),
	PHP_PLAY_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_PLAY
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(play)
#endif

int set_root_path()
{
	play_string *root = NULL;
	if (PG(auto_globals_jit)) {
		zend_is_auto_global_str("_SERVER", 7);
	}
	zval *path = zend_hash_str_find(Z_ARRVAL_P(&PG(http_globals)[TRACK_VARS_SERVER]), "DOCUMENT_ROOT", 13);
	if (path == NULL || Z_STRLEN_P(path) == 0) {
		path = zend_hash_str_find(Z_ARRVAL_P(&PG(http_globals)[TRACK_VARS_SERVER]), "PWD", 3);
	}
	if (path == NULL || Z_STRLEN_P(path) == 0) {
		// printf("can not get run script path\n");
		return 0;
	}
	root = play_find_project_root_by_path(Z_STRVAL_P(path));
	if (root != NULL) {
		play_global_config_set_app_root(root->val, root->len);
		play_string_free(root);
		return 0;
	}
	return -1;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
