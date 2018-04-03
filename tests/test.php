<?php

php_coro_init();

php_coro_addfun(function(){
    echo "===this is function 1\n";
    php_coro_yield();
    echo "===this is function 1,line 2\n";

});


function test(){
    echo "test start~======\n";
    echo "test start~======\n";
    echo "test start~======\n";
    echo "test start~======\n";
    echo "test start~======\n";
    echo "test start~======\n";
    php_coro_yield();
    echo "test end~======\n";
    echo "test end~======\n";
    echo "test end~======\n";
}

php_coro_addfun(function (){
    echo "===this is function 3\n";
    php_coro_yield();
    echo "===this is function 3,line 2\n";

    test();
    echo "===thi====on test done\n";

});

php_coro_addfun(function (){
    echo "===this is function 3\n";
    php_coro_yield();
    echo "===this is function 3,line 2\n";

    test();
    echo "===thi====on test done\n";

});


php_coro_addfun(function (){
    echo "===this is function 3\n";
    php_coro_yield();
    echo "===this is function 3,line 2\n";

    test();
    echo "===thi====on test done\n";

});


php_coro_addfun(function (){
    echo "===this is function 3\n";
    //php_coro_yield();
    echo "===this is function 3,line 2\n";

    test();
    echo "===thi====on test done\n";

});





php_coro_run();

