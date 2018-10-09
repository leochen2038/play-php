#!/bin/bash
phpize
# CFLAGS=-std=c99 ./configure
./configure
make && make install
make clean
/etc/init.d/php-fpm restart