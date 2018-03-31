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
static int le_coroutine_php;


void generic_handler(struct evhttp_request *req, void *arg )
{
    
    struct evbuffer *buf = evbuffer_new();
    if(!buf)
    {
        puts("failed to create response buffer \n");
        return;
    }

    evbuffer_add_printf(buf, "Server Responsed. Requested: %s\n", evhttp_request_get_uri(req));
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
    
}

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("coroutine_php.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_coroutine_php_globals, coroutine_php_globals)
    STD_PHP_INI_ENTRY("coroutine_php.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_coroutine_php_globals, coroutine_php_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string coroutine_php_init(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(coroutine_php_init)
{
    short          http_port = 8003;
    char          *http_addr = "127.0.0.1";
    
    struct event_base * base = event_base_new();
    struct evhttp * http_server = evhttp_new(base);
    if(!http_server)
    {
        php_printf("http_server error !");
    }
    int ret = evhttp_bind_socket(http_server,http_addr,http_port);
    if(ret!=0)
    {
        php_printf("http_server error !");
    }
    
    evhttp_set_gencb(http_server, generic_handler, NULL);

    php_printf("HTTP START !");
    event_base_dispatch(base);

    evhttp_free(http_server);

    //WSACleanup();

	/*
	char *arg = NULL;
	size_t arg_len, len;
	zend_string *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	strg = strpprintf(0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "coroutine_php", arg);

	RETURN_STR(strg);
	*/
	//php_printf("hello world!");
}


static int cp_zend_is_callable_ex(zval *cb, zval *object , uint a, char **cb_name,zend_fcall_info_cache *cbcache,char **error TSRMLS_DC)
{
    zend_string *key = NULL;
    int ret = zend_is_callable_ex(cb, NULL, a, &key,cbcache,error);
    char *tmp = estrndup(key->val, key->len);
    zend_string_release(key);
    *cb_name = tmp;
    return ret;
}



typedef struct _php_coroutine_context php_coroutine_context;
struct _php_coroutine_context
{
  jmp_buf *buf_ptr;
  zval *callback;
  zend_op_array *op_array;
  zend_fcall_info_cache *func_cache;
  zend_execute_data *execute_data;
  zend_execute_data *origin_execute_data;
  intptr_t op_idx;
  zend_vm_stack origin_vm_stack;
  intptr_t origin_vm_stack_top;
  intptr_t origin_vm_stack_end;
  zend_vm_stack current_vm_stack;
  intptr_t current_vm_stack_top;
  intptr_t current_vm_stack_end;

}Php_coroutine_context;



ZEND_API void cp_zend_execute(zend_op_array *op_array, zval *return_value,php_coroutine_context *context)
{
    zend_execute_data *execute_data;

    if (EG(exception) != NULL) {
        return;
    }

    //分配zend_execute_data
    execute_data = zend_vm_stack_push_call_frame(ZEND_CALL_TOP_CODE,
            (zend_function*)op_array, 0, zend_get_called_scope(EG(current_execute_data)), zend_get_this_object(EG(current_execute_data)));
    if (EG(current_execute_data)) {
        execute_data->symbol_table = zend_rebuild_symbol_table();
    } else {
        execute_data->symbol_table = &EG(symbol_table);
    }

    //EX(prev_execute_data) = EG(current_execute_data); //=> execute_data->prev_execute_data = EG(current_execute_data);
    //i_init_execute_data(execute_data, op_array, return_value); //初始化execute_data
    zend_init_execute_data(execute_data,op_array,return_value);
    php_printf("cp_zend_execute execute_data:%d\n",execute_data);
    context->execute_data = execute_data;
    zend_execute_ex(execute_data); //执行opcode
    php_printf("cp_zend_execute execute_data:%d\n",execute_data);
    
    //zend_vm_stack_free_call_frame(execute_data); //释放execute_data:销毁所有的PHP变量
}

ZEND_API void cp_zend_execute_resume(zend_op_array *op_array, zval *return_value)
{

    zend_execute_data *execute_data;



    if (EG(exception) != NULL) {
        return;
    }

    //分配zend_execute_data
    execute_data = zend_vm_stack_push_call_frame(ZEND_CALL_TOP_CODE,
            (zend_function*)op_array, 0, zend_get_called_scope(EG(current_execute_data)), zend_get_this_object(EG(current_execute_data)));
    // if (EG(current_execute_data)) {
    //     execute_data->symbol_table = zend_rebuild_symbol_table();
    // } else {
    //     execute_data->symbol_table = &EG(symbol_table);
    // }





    //EX(prev_execute_data) = EG(current_execute_data); //=> execute_data->prev_execute_data = EG(current_execute_data);
    //i_init_execute_data(execute_data, op_array, return_value); //初始化execute_data
    zend_init_execute_data(execute_data,op_array,return_value);

    zend_execute_ex(execute_data); //执行opcode
   
    //zend_vm_stack_free_call_frame(execute_data); //释放execute_data:销毁所有的PHP变量
}


PHP_FUNCTION(php_coro_create)
{
  php_coroutine_context *context = NULL;
  jmp_buf *php_setjmp_jmp_buf = NULL;
  context = emalloc(sizeof(php_coroutine_context));
  php_setjmp_jmp_buf = emalloc(sizeof(jmp_buf));
  context->buf_ptr = php_setjmp_jmp_buf;

  //context->callback = emalloc(sizeof(zval));
  context->func_cache = emalloc(sizeof(zend_fcall_info_cache));

  php_printf("php_coro_create buf_ptr:%d\n",php_setjmp_jmp_buf);
  php_printf("php_coro_create context:%d\n",context);
  RETURN_LONG((intptr_t)context);
}

PHP_FUNCTION(php_coro_init)
{
    
    php_coroutine_context *context = NULL;
    char *cb_name = NULL;
    zval *ret;
    zval *callback = NULL;

    int r;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &context,&callback,sizeof(intptr_t)) == FAILURE) 
    {
      RETURN_FALSE;
    }

    context->callback = callback;

    context->origin_vm_stack = EG(vm_stack);
    context->origin_vm_stack_top = EG(vm_stack_top);
    context->origin_vm_stack_end = EG(vm_stack_end);

    context->origin_execute_data = EG(current_execute_data);

    php_printf("php_coro_init EG(vm_stack):%d\n",EG(vm_stack));
    php_printf("php_coro_init EG(vm_stack_top):%d\n",EG(vm_stack_top));
    php_printf("php_coro_init EG(vm_stack_end):%d\n",EG(vm_stack_end));
    php_printf("php_coro_init context->origin_vm_stack:%d\n",context->origin_vm_stack);
    php_printf("php_coro_init context->origin_vm_stack_top:%d\n",context->origin_vm_stack_top);
    php_printf("php_coro_init context->origin_vm_stack_end:%d\n",context->origin_vm_stack_end);
    php_printf("php_coro_init context->current_vm_stack:%d\n",context->current_vm_stack);
    php_printf("php_coro_init context->current_vm_stack_top:%d\n",context->current_vm_stack_top);
    php_printf("php_coro_init context->current_vm_stack_end:%d\n",context->current_vm_stack_end);
    php_printf("php_coro_init EG(current_execute_data)->prev_execute_data:%d\n",EG(current_execute_data)->prev_execute_data);


    //声明function cache
    php_printf("setjmp struct:%d\n",context->buf_ptr);
    //setjmp 放在cp_zend_is_callable_ex下面就会有问题
    r = setjmp(*context->buf_ptr);
    //赋值
    if(!cp_zend_is_callable_ex(context->callback, NULL, 0, &cb_name,context->func_cache,NULL TSRMLS_CC)){
      RETURN_FALSE;
    } 

    //获取oparray
    context->op_array = (zend_op_array *)context->func_cache->function_handler;
    //cp_zend_execute(context->op_array,ret);


    // zend_execute_data *execute_data2;

    // if (EG(exception) != NULL) {
    //   return;
    // }

    // execute_data2 = zend_vm_stack_push_call_frame(ZEND_CALL_TOP_CODE | ZEND_CALL_HAS_SYMBOL_TABLE,
    //   (zend_function*)context->op_array, 0, zend_get_called_scope(EG(current_execute_data)), zend_get_this_object(EG(current_execute_data)));
    // if (EG(current_execute_data)) {
    //   execute_data2->symbol_table = zend_rebuild_symbol_table();
    // } else {
    //   execute_data2->symbol_table = &EG(symbol_table);
    // }
    // // EX(prev_execute_data) = EG(current_execute_data);
    // // i_init_execute_data(execute_data, context->op_array, return_value);
    // zend_init_execute_data(execute_data2,context->op_array,ret);
    // zend_execute_ex(execute_data2);
    // zend_vm_stack_free_call_frame(execute_data2);










    //efree(cb_name);先不释放
    //printf("php_setjmp_jmp_buf:%ld\n",(intptr_t)context->buf_ptr);//输出跳转就报错

    switch(r){
      case 0://init
        //printf("init\n",r);
        //cp_zend_execute(context->op_array,ret); 
        php_printf("php_coro_resume zend_execute_ex:%d\n",zend_execute_ex);

        cp_zend_execute(context->op_array,ret,context);
        /*
        if (EG(exception) != NULL) {
            return;
        }

        //分配zend_execute_data
        context->execute_data = zend_vm_stack_push_call_frame(ZEND_CALL_TOP_CODE,
                (zend_function*)context->op_array, 0, zend_get_called_scope(EG(current_execute_data)), zend_get_this_object(EG(current_execute_data)));
        if (EG(current_execute_data)) {
            context->execute_data->symbol_table = zend_rebuild_symbol_table();
        } else {
            context->execute_data->symbol_table = &EG(symbol_table);
        }

        //EX(prev_execute_data) = EG(current_execute_data); //=> execute_data->prev_execute_data = EG(current_execute_data);
        //i_init_execute_data(execute_data, op_array, ret); //初始化execute_data
        zend_init_execute_data(context->execute_data,context->op_array,ret);

        zend_execute_ex(context->execute_data); //执行opcode
        zend_vm_stack_free_call_frame(context->execute_data); //释放execute_data:销毁所有的PHP变量

        */
        break;
      case 1://yield
        //printf("yield\n",r);
        break;
    }

    //printf("jmpretrun\n",r);
    RETURN_TRUE;
    
}

PHP_FUNCTION(php_coro_yield)
{
    jmp_buf *ptr = NULL;
    php_coroutine_context *context = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &context,sizeof(intptr_t)) == FAILURE) {
        return;//
    }

    context->current_vm_stack = EG(vm_stack);
    context->current_vm_stack_top = EG(vm_stack_top);
    context->current_vm_stack_end = EG(vm_stack_end);


    php_printf("php_coro_yield EG(vm_stack):%d\n",EG(vm_stack));
    php_printf("php_coro_yield EG(vm_stack_top):%d\n",EG(vm_stack_top));
    php_printf("php_coro_yield EG(vm_stack_end):%d\n",EG(vm_stack_end));
    php_printf("php_coro_yield context->current_vm_stack:%d\n",context->current_vm_stack);
    php_printf("php_coro_yield context->current_vm_stack_top:%d\n",context->current_vm_stack_top);
    php_printf("php_coro_yield context->current_vm_stack_end:%d\n",context->current_vm_stack_end);
    php_printf("php_coro_yield context->origin_vm_stack:%d\n",context->origin_vm_stack);
    php_printf("php_coro_yield context->origin_vm_stack_top:%d\n",context->origin_vm_stack_top);
    php_printf("php_coro_yield context->origin_vm_stack_end:%d\n",context->origin_vm_stack_end);




    // EG(vm_stack) = context->origin_vm_stack;
    // EG(vm_stack_top) = context->origin_vm_stack_top;
    // EG(vm_stack_end) = context->origin_vm_stack_end;

    php_printf("php_coro_yield context:%d\n",context);

    ptr = context->buf_ptr;
    context->op_idx = context->op_array->opcodes;
    php_printf("php_coro_yield context->op_idx:%d\n",context->op_idx);

    php_printf("php_coro_resume zend_execute_ex:%d\n",zend_execute_ex);
    longjmp(*ptr,1);//1 yield
    RETURN_TRUE;
}



PHP_FUNCTION(php_coro_resume)
{
    
    zval *ret;
    zend_execute_data *current_execute_data;
    //jmp_buf *ptr = NULL;

    php_coroutine_context *context = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &context,sizeof(intptr_t)) == FAILURE) 
    {
        return;//
    }


    current_execute_data = context->execute_data;
    // current_execute_data->prev_execute_data = EG(current_execute_data);
    // current_execute_data->return_value = ret;
    // current_execute_data->opline -=4;
    //current_execute_data->opline++;
    // //current_execute_data->symbol_table = zend_rebuild_symbol_table();


    php_printf("php_coro_resume EG(vm_stack):%d\n",EG(vm_stack));
    php_printf("php_coro_resume EG(vm_stack_top):%d\n",EG(vm_stack_top));
    php_printf("php_coro_resume EG(vm_stack_end):%d\n",EG(vm_stack_end));
    php_printf("php_coro_resume context->current_vm_stack:%d\n",context->current_vm_stack);
    php_printf("php_coro_resume context->current_vm_stack_top:%d\n",context->current_vm_stack_top);
    php_printf("php_coro_resume context->current_vm_stack_end:%d\n",context->current_vm_stack_end);
    php_printf("php_coro_resume context->origin_vm_stack:%d\n",context->origin_vm_stack);
    php_printf("php_coro_resume context->origin_vm_stack_top:%d\n",context->origin_vm_stack_top);
    php_printf("php_coro_resume context->origin_vm_stack_end:%d\n",context->origin_vm_stack_end);
    php_printf("php_coro_resume EG(current_execute_data):%d\n",EG(current_execute_data));
    php_printf("php_coro_resume EG(current_execute_data)->prev_execute_data:%d\n",EG(current_execute_data)->prev_execute_data);
    php_printf("php_coro_resume current_execute_data:%d\n",current_execute_data);



    // if (ZEND_CALL_INFO(current_execute_data) & ZEND_CALL_RELEASE_THIS)
    // {
    //     zval_ptr_dtor(&(current_execute_data->This));
    // }



    // zend_vm_stack_free_args(current_execute_data);
    // zend_vm_stack_free_call_frame(current_execute_data);

    current_execute_data->prev_execute_data = EG(current_execute_data);
    EG(current_execute_data) = current_execute_data;

    EG(current_execute_data)->opline +=1;

    // php_printf("php_coro_resume context->origin_vm_stack:%d\n",context->origin_vm_stack);
    // EG(vm_stack) = context->current_vm_stack;
    //EG(vm_stack_top) = context->current_vm_stack_top;
    // EG(vm_stack_end) = context->current_vm_stack_end;
    // //EX(prev_execute_data) = EG(current_execute_data);
    // EX(prev_execute_data) = context->origin_execute_data;
    



    //zend_init_execute_data(context->execute_data,context->op_array,ret);
    // zend_function *func = current_execute_data->func;
    // zend_op_array *op_array = &func->op_array;
    // EX(func) = (zend_function*)current_execute_data->func;
    // EX(opline) = op_array->line_start;
    // op_array->opcodes = op_array->line_start;
    // EX(call) = NULL;
    // EX(return_value) = ret;
    // EX_LOAD_RUN_TIME_CACHE(op_array);
    // EX_LOAD_LITERALS(op_array);
    //EG(current_execute_data) = current_execute_data;
    php_printf("php_coro_resume current_execute_data:%d\n",current_execute_data);
    //EG(current_execute_data)->opline -= 2;

    current_execute_data->prev_execute_data->opline += 2;
    zend_execute_ex(EG(current_execute_data));

    // EG(current_execute_data) = current_execute_data;
    // EG(current_execute_data)->opline -= 2;
    // zend_execute_ex(current_execute_data);

    // EG(current_execute_data) = current_execute_data;
    // EG(current_execute_data)->opline -= 2;
    // zend_execute_ex(current_execute_data);

    // EG(current_execute_data) = current_execute_data;
    // EG(current_execute_data)->opline -= 2;
    // zend_execute_ex(current_execute_data);


    // php_printf("php_coro_resume zend_execute_ex:%d\n",zend_execute_ex);
    
    //zend_execute(context->op_array,ret);



    //zend_op_array *op_array = (zend_op_array *)context->func_cache->function_handler;

    // zend_init_execute_data(context->execute_data,context->op_array,ret);
    // zend_execute_ex(context->execute_data); //执行opcode
    //cp_zend_execute_resume(context->op_array,ret);

    // context->origin_vm_stack = EG(vm_stack);
    // context->origin_vm_stack_top = EG(vm_stack_top);
    // context->origin_vm_stack_end = EG(vm_stack_end);
    

    // EG(vm_stack) = context->current_vm_stack;
    // EG(vm_stack_top) = context->current_vm_stack_top;
    // EG(vm_stack_end) = context->current_vm_stack_end;
    // EX(prev_execute_data) = context->execute_data->prev_execute_data;
        
    //     EG(current_execute_data) = context->execute_data;


    //     EG(current_execute_data)->opline++;
    //     EG(current_execute_data)->return_value = ret;

        

    //     //分配zend_execute_data
    //     // context->execute_data = zend_vm_stack_push_call_frame(ZEND_CALL_TOP_CODE,
    //     //         (zend_function*)context->op_array, 0, zend_get_called_scope(EG(current_execute_data)), zend_get_this_object(EG(current_execute_data)));
    //     // if (EG(current_execute_data)) {
    //          context->execute_data->symbol_table = zend_rebuild_symbol_table();
    //     // } else {
    //     //    context->execute_data->symbol_table = &EG(symbol_table);
    //     // }

    //     //EX(prev_execute_data) = EG(current_execute_data); //=> execute_data->prev_execute_data = EG(current_execute_data);

    //     //i_init_execute_data(execute_data, op_array, ret); //初始化execute_data
    //     //zend_init_execute_data(context->execute_data,context->op_array,ret);

    //     zend_execute_ex(context->execute_data); //执行opcode
    //     // zend_vm_stack_free_call_frame(context->execute_data); //释放execute_data:销毁所有的PHP变量
    //     // //printf("xxxxxjdljalajdf\n");

    //     // efree(EG(vm_stack));
    //     // //恢复执行栈
    //     // EG(vm_stack) = context->origin_vm_stack;
    //     // EG(vm_stack_top) = context->origin_vm_stack_top;
    //     // EG(vm_stack_end) = context->origin_vm_stack_end;
    //     // EG(current_execute_data) = context->origin_execute_data;







    // // ptr = context->buf_ptr;
    // // //context->op_array->opcodes +=3;
    // // // context->op_array->opcodes = context->op_idx;
    // // // EX(opline) = context->op_array->opcodes;
    // // php_printf("context->op_idx:%d,op_codes:%d\n",context->op_idx,context->op_array->opcodes);
    // // //php_printf("php_coro_resume:%d\n",context);
    // // cp_zend_execute(context->op_array,ret); 

    // //php_printf("php_coro_resume done:%d\n",context->op_array);

    // //longjmp(*ptr,2);//2 resume
    // //RETURN_TRUE;
}

PHP_FUNCTION(php_coro_free)
{
  jmp_buf *buf_ptr = NULL;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &buf_ptr,sizeof(intptr_t)) == FAILURE) 
  {
    RETURN_FALSE;
  }
  efree(buf_ptr);
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
	PHP_FE(coroutine_php_init,	NULL)		
  PHP_FE(php_coro_create,  NULL)
  PHP_FE(php_coro_init,  NULL)
  PHP_FE(php_coro_yield,  NULL)
  PHP_FE(php_coro_resume,  NULL)
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
