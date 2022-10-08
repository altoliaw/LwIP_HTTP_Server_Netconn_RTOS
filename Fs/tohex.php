<?php
$fp = fopen('404.html', 'r');

$output = [];
while (false !== ($char = fgetc($fp))) {
    $output[] = '0x'.bin2hex($char);
}

foreach (array_chunk($output, 16) as $size16) {
    echo implode(',', $size16).",\n";
}
