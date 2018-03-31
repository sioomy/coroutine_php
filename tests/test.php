<?php
//phpinfo();
echo "xx1\n";
echo "xx2\n";
echo "xx3\n";
echo "xx4\n";
// coroutine_php_init();
$i=0;
$buf = php_coro_create();
var_dump($buf);
php_coro_init($buf,function(){
    global $buf,$i;

    $j = 0;
    echo "ss\n";
    echo "this is ok\n";

    $j++;

    echo "\$j:$j\n";

    if($i==0){
        php_coro_yield($buf);
    }
    
    //var_dump($buf);
    echo "\$j:$j\n";
    echo "line 3\n";
    echo "line 4\n";

    //throw new Exception("Error Processing Request", 1);
    

});
echo "xxx\n";
$i++;
echo "\$i:$i\n";
if($i==1){
    echo "php_coro_resume-----\n";


    php_coro_resume($buf);


    echo "ok\n";
}

//php_coro_free($buf);

?>
