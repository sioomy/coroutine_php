# coroutine_php
This project is a php extension,which aims to provide a basic method to implement coroutine manager.

# how to install
 step 1:

 >phpize
 
 step 2:
 
 >./configure --enable-coroutine_php && make && make install

 step 3:
 add this line in php.ini

 >extension=coroutine_php.so

# methods:
 
create coroutine ,return a number of coroutine ptr.the param1 ,is a callback of coroutine,the param 2 is optional.
 
    >[cid] php_coro_create([callback],[param..])
 
run zhe coroutine of the callback which is set at php_coro_create,untill run php_coro_yield in callback
 
    >php_coro_next([cid])

run all coroutine

    >php_coro_walk()

get coroutine state:

0  defalut

1  yield

2  end

    > [state] php_coro_state([cid])

git current coroutine cid:

    > [cid] php_coro_current()

get coroutine callback return value:

    > [val] php_coro_getval([cid])

yield the coroutine ,it must run in coroutine callback

    > php_coro_yield()

free coroutine

    > php_coro_free([cid])

#demo:

    function test(){
        echo "test start~======\n";
        php_coro_yield();
        echo "test end~======\n";
    }
    $cx1 = php_coro_create(function (){
        echo "===this is function 1,line 1\n";
        php_coro_yield();
        echo "===this is function 1,line 2\n";
        // php_coro_yield();
        test();
        echo "===thi====on test done\n";
        echo "===thi====on test done bak\n";
    });
    $cx2 = php_coro_create(function (){
        echo "===this is function 2,line 1\n";
        php_coro_yield();
        echo "===this is function 2,line 2\n";
        // php_coro_yield();
        test();
        echo "===thi====on test2 done\n";
        echo "===thi====on test2 done bak\n";
    });
    var_dump(php_coro_next($cx1));
    var_dump(php_coro_next($cx2));
    var_dump(php_coro_next($cx1));
    php_coro_free($cx2);
    echo "free test2\n";
    var_dump(php_coro_next($cx2));
    var_dump(php_coro_next($cx2));
    var_dump(php_coro_next($cx1));
