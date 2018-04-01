<?php
//phpinfo();
echo "test script begin\n";
// function hello(){
//     echo "this is function 1\n";
//     php_coro_yield();
//     echo "this is function 1,line 2\n";

// }

php_coro_init();
echo "test script addfunc1\n";
php_coro_addfun(function(){
    echo "this is function 1\n";
    php_coro_yield();
    echo "this is function 1,line 2\n";

});

//php_coro_addfun("hello");

//echo "test script addfunc2\n";
// php_coro_addfun(function (){
//     echo "this is function 2\n";
//     //php_coro_yield();
//     echo "this is function 2,line 2\n";
// });
//echo "test script addfunc3\n";
// php_coro_addfun(function (){
//     echo "this is function 3\n";
//     //php_coro_yield();
//     echo "this is function 3,line 2\n";
// });
echo "test script run\n";
php_coro_run();
echo "test script done\n";
?>
