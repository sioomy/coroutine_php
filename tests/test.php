<?php

php_coro_init();


function test(){
    echo "test start~======\n";
    php_coro_yield();
    echo "test end~======\n";
}

php_coro_addfun(function (){
    echo "===this is function 1,line 1\n";
    php_coro_yield();
    echo "===this is function 1,line 2\n";
    // php_coro_yield();
    test();
    echo "===thi====on test done\n";
    echo "===thi====on test done bak\n";
});

php_coro_addfun(function (){
    echo "===this is function 2,line 1\n";
    php_coro_yield();
    echo "===this is function 2,line 2\n";
});

php_coro_addfun(function (){
    echo "===this is function 3,line 1\n";
    php_coro_yield();
    echo "===this is function 3,line 2\n";
 
});

php_coro_addfun(function (){
    echo "===this is function 4,line 1\n";
    php_coro_yield();
    echo "===this is function 4,line 2\n";
});

php_coro_addfun(function (){
    echo "===this is function 5,line 1\n";
    php_coro_yield();
    echo "===this is function 5,line 2\n";
    php_coro_yield();
    echo "===this is function 5,line 3\n";
    php_coro_yield();
    echo "===this is function 5,line 4\n";
    php_coro_yield();
    echo "===this is function 5,line 5\n";
    php_coro_yield();
    echo "===this is function 5,line 6\n";
    php_coro_yield();
    echo "===this is function 5,line 7\n";
});



php_coro_run();

