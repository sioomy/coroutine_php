<?php
echo "test script begin\n";
php_coro_init();

php_coro_addfun(function(){
    echo "this is function 1\n";
    php_coro_yield();
    echo "this is function 1,line 2\n";

});

php_coro_addfun(function (){
    echo "this is function 2\n";
    php_coro_yield();
    echo "this is function 2,line 2\n";
});


echo "test script run\n";
php_coro_run();
echo "test script done\n";
?>
