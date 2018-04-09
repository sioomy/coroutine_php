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
#include "php_coroutine_php.h"

#include <event2/event.h>
#include <event2/http.h>
#include "event2/buffer.h"
#include <stdlib.h>
#include <stdio.h>

/* If you declare any globals in php_coroutine_php.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(coroutine_php)
*/

/* True global resources - no need for thread safety here */
#define CORO_DEFAULT 0
#define CORO_YIELD 1
#define CORO_END 2
#define CORO_RESUME 3
#define CORO_START 4


static int le_coroutine_php;

typedef struct _php_coroutine_context{
    jmp_buf *buf_ptr;
    zend_execute_data *execute_data;
    zend_execute_data *prev_execute_data;//execute for execute before yield
    struct _php_coroutine_context *next;
    struct _php_coroutine_context *prev;
    int coro_state;
    zend_vm_stack current_vm_stack;
    zval* current_vm_stack_top;
    zval* current_vm_stack_end;
    zend_fcall_info_cache* func_cache;
    zval *ret;
}php_coroutine_context;

static struct _g_coro_stack{
    zend_vm_stack vm_stack;
    zval* vm_stack_top;
    zval* vm_stack_end;
}g_coro_stack;

static int cp_zend_is_callable_ex(zval *cb, zval *object , uint a, char **cb_name,zend_fcall_info_cache *cbcache,char **error TSRMLS_DC)
{
    zend_string *key = NULL;
    int ret = zend_is_callable_ex(cb, NULL, a, &key,cbcache,error);
    char *tmp = estrndup(key->val, key->len);
    zend_string_release(key);
    *cb_name = tmp;
    return ret;
}



/* use to stor coroutine context */

static php_coroutine_context* current_coroutine_context = NULL;
static int coroutine_context_count = 0;

static void free_coroutine_context(php_coroutine_context* context){
    current_coroutine_context = context->next;
    if(coroutine_context_count>0){
        coroutine_context_count--;

        //unlink
        context->prev->next = context->next;
        context->next->prev = context->prev;
        //todo free all data
        efree(context->buf_ptr);
        context->buf_ptr = NULL;
        zend_vm_stack_free_call_frame(context->execute_data); //释放execute_data:销毁所有的PHP变量
        context->execute_data = NULL;
        efree(context->func_cache);
        context->func_cache = NULL;
        efree(context);
        context = NULL;
        if(coroutine_context_count == 0){
            current_coroutine_context = NULL;
        }
    }
}


PHP_FUNCTION(php_coro_create)
{
    zval *callback = NULL;
    zval *params = NULL;
    php_coroutine_context *context = NULL;
    char *cb_name = NULL;
    zend_object *object;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z",&callback,&params) == FAILURE) 
    {
      RETURN_FALSE;
    }
    g_coro_stack.vm_stack = EG(vm_stack);
    g_coro_stack.vm_stack_top = EG(vm_stack_top);
    g_coro_stack.vm_stack_end = EG(vm_stack_end);
    

    context = emalloc(sizeof(php_coroutine_context));
    context->coro_state = CORO_DEFAULT;
    context->func_cache = emalloc(sizeof(zend_fcall_info_cache));


    if(!cp_zend_is_callable_ex(callback, NULL, 0, &cb_name,context->func_cache,NULL TSRMLS_CC)){
      RETURN_FALSE;
    }

    zend_op_array* op_array = (zend_op_array *)context->func_cache->function_handler;

    zend_vm_stack_init();

    

    object = (((zend_function*)op_array)->common.fn_flags & ZEND_ACC_STATIC) ? NULL : context->func_cache->object;

    if(params){
        // assign zend_execute_data
        context->execute_data = zend_vm_stack_push_call_frame(ZEND_CALL_TOP_FUNCTION,
                (zend_function*)op_array, 1, context->func_cache->called_scope, object);
        
        //put params
        zval *target = ZEND_CALL_ARG(context->execute_data, 1);
        ZVAL_COPY(target, params);
    }else{
        context->execute_data = zend_vm_stack_push_call_frame(ZEND_CALL_TOP_FUNCTION,
                (zend_function*)op_array, 0, context->func_cache->called_scope, object);
    }
    


    if (EG(current_execute_data)) {
        context->execute_data->symbol_table = zend_rebuild_symbol_table();
    } else {
        context->execute_data->symbol_table = &EG(symbol_table);
    }

    zend_init_execute_data(context->execute_data,op_array,context->ret);

    context->buf_ptr = emalloc(sizeof(jmp_buf));

    if(!current_coroutine_context){
        current_coroutine_context = context;
    }


    //link linktable
    if(coroutine_context_count == 0){
        context->next = context;
        context->prev = context;
    }else{
        context->prev = current_coroutine_context->prev;
        context->prev->next = context;
        context->next = current_coroutine_context;
        context->next->prev = context; 
    }
    coroutine_context_count++;

    context->current_vm_stack = EG(vm_stack);
    context->current_vm_stack_top = EG(vm_stack_top);
    context->current_vm_stack_end = EG(vm_stack_end);

    EG(vm_stack) = g_coro_stack.vm_stack;
    EG(vm_stack_top) = g_coro_stack.vm_stack_top;
    EG(vm_stack_end) = g_coro_stack.vm_stack_end;

    RETURN_LONG((intptr_t)context);
}


static void coro_controler_run(){
    loopstart:
    
    switch (setjmp(*current_coroutine_context->buf_ptr)){
        case CORO_DEFAULT:
            g_coro_stack.vm_stack = EG(vm_stack);
            g_coro_stack.vm_stack_top = EG(vm_stack_top);
            g_coro_stack.vm_stack_end = EG(vm_stack_end);
            EG(vm_stack) = current_coroutine_context->current_vm_stack;
            EG(vm_stack_top) = current_coroutine_context->current_vm_stack_top;
            EG(vm_stack_end) = current_coroutine_context->current_vm_stack_end;
            EG(current_execute_data) = current_coroutine_context->execute_data;
            break;
        case CORO_YIELD:
            current_coroutine_context = current_coroutine_context->next;
            //state of next coroutine
            if(current_coroutine_context->coro_state == CORO_DEFAULT){
                goto loopstart;
            }else if(current_coroutine_context->coro_state == CORO_YIELD){
                longjmp(*current_coroutine_context->buf_ptr,CORO_RESUME); 
            }
            break;
        case CORO_RESUME:
            g_coro_stack.vm_stack = EG(vm_stack);
            g_coro_stack.vm_stack_top = EG(vm_stack_top);
            g_coro_stack.vm_stack_end = EG(vm_stack_end);
            EG(vm_stack) = current_coroutine_context->current_vm_stack;
            EG(vm_stack_top) = current_coroutine_context->current_vm_stack_top;
            EG(vm_stack_end) = current_coroutine_context->current_vm_stack_end;
            EG(current_execute_data) = current_coroutine_context->prev_execute_data;
            EG(current_execute_data)->opline++;
                
            break;
        default:
            break;
    }
    zend_execute_ex(EG(current_execute_data));

    EG(vm_stack) = g_coro_stack.vm_stack;
    EG(vm_stack_top) = g_coro_stack.vm_stack_top;
    EG(vm_stack_end) = g_coro_stack.vm_stack_end;

    //end
    free_coroutine_context(current_coroutine_context);
    if(coroutine_context_count>0){ 
        if(current_coroutine_context->coro_state == CORO_DEFAULT){
            goto loopstart;
        }else if(current_coroutine_context->coro_state == CORO_YIELD){
            longjmp(*current_coroutine_context->buf_ptr,CORO_RESUME); 
        }
    }


}

PHP_FUNCTION(php_coro_yield)
{

    current_coroutine_context->coro_state = CORO_YIELD;
    current_coroutine_context->current_vm_stack = EG(vm_stack);
    current_coroutine_context->current_vm_stack_top = EG(vm_stack_top);
    current_coroutine_context->current_vm_stack_end = EG(vm_stack_end);

    EG(vm_stack) = g_coro_stack.vm_stack;
    EG(vm_stack_top) = g_coro_stack.vm_stack_top;
    EG(vm_stack_end) = g_coro_stack.vm_stack_end;
    current_coroutine_context->prev_execute_data = EG(current_execute_data)->prev_execute_data;
    longjmp(*current_coroutine_context->buf_ptr,CORO_YIELD);
}

// PHP_FUNCTION(php_coro_savestack)
// {
//     // current_coroutine_context->current_vm_stack = EG(vm_stack);
//     // current_coroutine_context->current_vm_stack_top = EG(vm_stack_top);
//     // current_coroutine_context->current_vm_stack_end = EG(vm_stack_end);
//     g_coro_stack.vm_stack = EG(vm_stack);
//     g_coro_stack.vm_stack_top = EG(vm_stack_top);
//     g_coro_stack.vm_stack_end = EG(vm_stack_end);
//     // EG(vm_stack) = g_coro_stack.vm_stack;
//     // EG(vm_stack_top) = g_coro_stack.vm_stack_top;
//     // EG(vm_stack_end) = g_coro_stack.vm_stack_end;
    
//     RETURN_TRUE;
// }

PHP_FUNCTION(php_coro_walk)
{
    coro_controler_run();
    RETURN_TRUE;
}

PHP_FUNCTION(php_coro_current)
{
    RETURN_LONG((intptr_t)current_coroutine_context);
}

PHP_FUNCTION(php_coro_state)
{
    php_coroutine_context *context = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&context,sizeof(intptr_t)) == FAILURE) 
    {
      RETURN_FALSE;
    }
    RETURN_LONG(context->coro_state);
}

PHP_FUNCTION(php_coro_next)
{
    php_coroutine_context *context = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&context,sizeof(intptr_t)) == FAILURE) 
    {
      RETURN_FALSE;
    }
    current_coroutine_context = context;
    if(current_coroutine_context->coro_state == CORO_DEFAULT){

        if(setjmp(*current_coroutine_context->buf_ptr) == CORO_DEFAULT){//first run
            
            g_coro_stack.vm_stack = EG(vm_stack);
            g_coro_stack.vm_stack_top = EG(vm_stack_top);
            g_coro_stack.vm_stack_end = EG(vm_stack_end);
            EG(vm_stack) = current_coroutine_context->current_vm_stack;
            EG(vm_stack_top) = current_coroutine_context->current_vm_stack_top;
            EG(vm_stack_end) = current_coroutine_context->current_vm_stack_end;

            EG(current_execute_data) = current_coroutine_context->execute_data;
            zend_execute_ex(EG(current_execute_data));
        }else{//yield checkout
            RETURN_TRUE;
        }


    }else if(current_coroutine_context->coro_state == CORO_YIELD){//resume
        if(setjmp(*current_coroutine_context->buf_ptr) == CORO_DEFAULT){//first run

            g_coro_stack.vm_stack = EG(vm_stack);
            g_coro_stack.vm_stack_top = EG(vm_stack_top);
            g_coro_stack.vm_stack_end = EG(vm_stack_end);
            EG(vm_stack) = current_coroutine_context->current_vm_stack;
            EG(vm_stack_top) = current_coroutine_context->current_vm_stack_top;
            EG(vm_stack_end) = current_coroutine_context->current_vm_stack_end;

            EG(current_execute_data) = current_coroutine_context->prev_execute_data;
            EG(current_execute_data)->opline++;
            zend_execute_ex(EG(current_execute_data));

        }else{//yield checkout
            RETURN_TRUE;
        }

    }

    //finish
    RETURN_FALSE;
}

PHP_FUNCTION(php_coro_getval)
{
    php_coroutine_context *context = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&context,sizeof(intptr_t)) == FAILURE) 
    {
      RETURN_FALSE;
    }
    RETURN_ZVAL(&context->ret,1,0);
}

PHP_FUNCTION(php_coro_free)
{
    php_coroutine_context *context = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&context,sizeof(intptr_t)) == FAILURE) 
    {
      RETURN_FALSE;
    }
    free_coroutine_context(context);

    RETURN_TRUE;
}



/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_coroutine_php_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_coroutine_php_init_globals(zend_coroutine_php_globals *coroutine_php_globals)
{
	coroutine_php_globals->global_value = 0;
	coroutine_php_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(coroutine_php)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(coroutine_php)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(coroutine_php)
{
#if defined(COMPILE_DL_COROUTINE_PHP) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(coroutine_php)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(coroutine_php)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "coroutine_php support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ coroutine_php_functions[]
 *
 * Every user visible function must have an entry in coroutine_php_functions[].
 */
const zend_function_entry coroutine_php_functions[] = {
    PHP_FE(php_coro_walk,	NULL)
    // PHP_FE(php_coro_savestack,   NULL)
    PHP_FE(php_coro_state,    NULL)
    PHP_FE(php_coro_next,    NULL)
    PHP_FE(php_coro_current,    NULL)
    PHP_FE(php_coro_getval,    NULL)
    PHP_FE(php_coro_create,  NULL)
    PHP_FE(php_coro_yield,  NULL)
    PHP_FE(php_coro_free,  NULL)
	PHP_FE_END	/* Must be the last line in coroutine_php_functions[] */
};
/* }}} */

/* {{{ coroutine_php_module_entry
 */
zend_module_entry coroutine_php_module_entry = {
	STANDARD_MODULE_HEADER,
	"coroutine_php",
	coroutine_php_functions,
	PHP_MINIT(coroutine_php),
	PHP_MSHUTDOWN(coroutine_php),
	PHP_RINIT(coroutine_php),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(coroutine_php),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(coroutine_php),
	PHP_COROUTINE_PHP_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_COROUTINE_PHP
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(coroutine_php)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
