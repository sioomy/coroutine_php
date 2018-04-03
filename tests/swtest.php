<?php
use Swoole\Coroutine as co;
co::create(function () {
    for($i = 0; $i < 100000; $i++) {
        co::sleep(1.0);
        echo "\$i:$i\n";
    }
});
co::create(function () {
    for($j = 0; $j < 100000; $j++) {
        co::sleep(2.5);
        echo "\$j:$j\n";
    }
});
