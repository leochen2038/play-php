# play
New php framework written in c, built in php extension

## 安装play扩展
```
$ /path/to/phpize
$ ./configure --with-php-config=/path/to/php-config
$ make && make install
```

## 创建项目
```
php -r "Play::init('proj');"
```

## 项目目录结构
```
 assets/
  - action/
  - meta/
 config/
 database/
  - Meta_Router_Mysql.php
 htdocs/
  - index.php
 library/
  - play/
   - core.api.php
 render/
  - Render_Html.php
  - Render_Json.php
  - Render_Jsonp.php
 source/
  - class/
  - processors/
  - templates/
```
