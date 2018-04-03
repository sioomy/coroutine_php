<?php
function test(){
    echo "test start~======\n";
    php_coro_yield();
    echo "test end~======\n";
}
php_coro_init();
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





exit();
php_coro_init();




$cx = php_coro_create(function (){
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
});

var_dump($cx2);
var_dump(php_coro_state($cx2));
//php_coro_free($cx2);

php_coro_create(function (){
    global $cx2;
    echo "===this is function 3,line 1\n";

    var_dump($cx2);
    var_dump(php_coro_state($cx2));

    php_coro_yield();
    echo "===this is function 3,line 2\n";
 
});

php_coro_create(function (){
    echo "===this is function 4,line 1\n";
    php_coro_yield();
    echo "===this is function 4,line 2\n";
});

php_coro_create(function (){
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



php_coro_loop();

