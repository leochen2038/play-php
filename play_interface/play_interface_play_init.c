//
// Created by Leo on 18/5/9.
//

#include <stdio.h>
#include <sys/stat.h>
#include <memory.h>
#include <fcntl.h>

static void play_interface_play_generate_core_api(char *src);
static void play_interface_play_generate_render_json(char *src);
static void play_interface_play_generate_render_html(char *src);
static void play_interface_play_generate_router_mysql(char *src);
static void play_interface_play_generate_render_jsonp(char *src);
static void play_interface_play_init_create_config(const char *proj);
static void play_interface_play_init_create_library(const char *proj);
static void play_interface_play_init_create_htdocs(const char *proj);
static void play_interface_play_init_create_source(const char *proj);
static void play_interface_play_init_create_router(const char *proj);
static void play_interface_play_init_create_render(const char *proj);
static void play_interface_play_init_create_assets(const char *proj);
static void play_interface_play_init_create_log(const char *proj);
static void play_interface_play_init_create_tmp(const char *proj);
static void play_interface_play_init_create_tool(const char *proj);


void play_interface_play_init(const char *proj)
{
    if (memcmp(proj, ".", 1) != 0 ) {
        if (mkdir(proj, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
            printf("create project error :%s\n", proj);
            return;
        }
    }
    char play[1024];
    snprintf(play, 1024, "%s/.play", proj);
    FILE *filefd = fopen(play, "w+");
    fclose(filefd);

    play_interface_play_init_create_config(proj);
    play_interface_play_init_create_library(proj);
    play_interface_play_init_create_htdocs(proj);
    play_interface_play_init_create_source(proj);
    play_interface_play_init_create_router(proj);
    play_interface_play_init_create_render(proj);
    play_interface_play_init_create_assets(proj);
    play_interface_play_init_create_log(proj);
    play_interface_play_init_create_tmp(proj);
    play_interface_play_init_create_tool(proj);
}

static void play_interface_play_init_create_log(const char *proj)
{
    char path[1024];
    snprintf(path, 1024, "%s/log", proj);
    if (mkdir(path, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == 0) {
        printf("|--create log...success\n");
    } else {
        printf("|--create log...failure\n");
    }
}

static void play_interface_play_init_create_tmp(const char *proj)
{
    char path[1024];
    snprintf(path, 1024, "%s/tmp", proj);
    if (mkdir(path, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == 0) {
        printf("|--create tmp...success\n");
    } else {
        printf("|--create tmp...failure\n");
    }
}

static void play_interface_play_init_create_tool(const char *proj)
{
    char path[1024];
    snprintf(path, 1024, "%s/tool", proj);
    if (mkdir(path, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == 0) {
        printf("|--create tool...success\n");
    } else {
        printf("|--create tool...failure\n");
    }
}

static void play_interface_play_init_create_config(const char *proj)
{
    FILE *filefd;
    char *config_data = "<?php\nreturn [\n];";
    char config_path[1024];
    char release_config_file[1024];
    char dev_config_file[1024];

    snprintf(config_path, 1024, "%s/config", proj);
    snprintf(release_config_file, 1024, "%s/release.config.php", config_path);
    snprintf(dev_config_file, 1024, "%s/dev.config.php", config_path);

    if (mkdir(config_path, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == 0) {
        printf("|--create config...success\n");

        filefd = fopen(release_config_file, "w+");
        fwrite(config_data, strlen(config_data), 1, filefd);
        fclose(filefd);

        filefd = fopen(dev_config_file, "w+");
        fwrite(config_data, strlen(config_data), 1, filefd);
        fclose(filefd);

        printf("  |--create release.config.php...success\n");
        printf("  |--create dev.config.php...success\n");
    } else {
        printf("|--create config...failure\n");
    }
}



static void play_interface_play_init_create_library(const char *proj)
{
    char framework[1024];
    char play[1024];
    char coreapi[1024];
    char data[65535];

    snprintf(framework, 1024, "%s/library", proj);
    snprintf(play, 1024, "%s/play", framework);
    snprintf(coreapi, 1024, "%s/core.api.php", play);

    if (mkdir(framework, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("|--create library...success\n");
    } else {
        printf("|--create library...failure\n");
        return;
    }

    if (mkdir(play, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("  |--create play...success\n");
    } else {
        printf("  |--create play...failure\n");
        return;
    }

    play_interface_play_generate_core_api(data);
    FILE *filefd = fopen(coreapi, "w+");
    fwrite(data, strlen(data), 1, filefd);
    fclose(filefd);
    printf("    |--create core.api.php...success\n");
}



static void play_interface_play_init_create_htdocs(const char *proj)
{
    char htdocs[1024] = {0};
    char index[1024] = {0};
    char data[1024];

    snprintf(htdocs, 1024, "%s/htdocs", proj);
    snprintf(index, 1024, "%s/index.php", htdocs);
    if (mkdir(htdocs, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("|---create htdocs...success\n");
    } else {
        printf("|---create htdocs...failure\n");
        return;
    }

    FILE *filefd = fopen(index, "w+");
    snprintf(data, 64, "<?php\nAction::boot();");
    fwrite(data, strlen(data), 1, filefd);
    fclose(filefd);
    printf("    |--create index.php...success\n");
}

static void play_interface_play_init_create_source(const char *proj)
{
    char source[1024];
    char processor[1024];
    char templates[1024];
    char class[1024];
    char crontab[1024];
    char services[1024];

    snprintf(source, 1024, "%s/source", proj);
    snprintf(processor, 1024, "%s/processors", source);
    snprintf(templates, 1024, "%s/templates", source);
    snprintf(class, 1024, "%s/class", source);
    snprintf(crontab, 1024, "%s/crontab", source);
    snprintf(services, 1024, "%s/services", source);

    if (mkdir(source, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("|---create source...success\n");
    } else {
        printf("|---create source...failure\n");
        return;
    }
    if (mkdir(processor, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("  |---create processors...success\n");
    } else {
        printf("  |---create processors...failure\n");
        return;
    }
    if (mkdir(templates, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("  |---create templates...success\n");
    } else {
        printf("  |---create templates...failure\n");
        return;
    }

    if (mkdir(class, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("  |---create class...success\n");
    } else {
        printf("  |---create class...failure\n");
        return;
    }
    if (mkdir(crontab, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("  |---create crontab...success\n");
    } else {
        printf("  |---create crontab...failure\n");
        return;
    }
    if (mkdir(services, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("  |---create services...success\n");
    } else {
        printf("  |---create services...failure\n");
        return;
    }
}

static void play_interface_play_init_create_router(const char *proj)
{
    char router[1024];
    char mysql[1024];
    char mysqlData[65535];

    snprintf(router, 1024, "%s/database", proj);
    snprintf(mysql, 1024, "%s/Meta_Router_Mysql.php", router);

    if (mkdir(router, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("|---create database...success\n");
    } else {
        printf("|---create database...failure\n");
        return;
    }

    FILE *mysqlfd = fopen(mysql, "w+");
    play_interface_play_generate_router_mysql(mysqlData);
    fwrite(mysqlData, strlen(mysqlData), 1, mysqlfd);
    fclose(mysqlfd);
    printf("  |--create Meta_Router_Mysql.php...success\n");
}

static void play_interface_play_init_create_render(const char *proj)
{
    char render[1024];
    char json[1024];
    char html[1024];
    char jsonp[1024];

    char jsonData[65535];
    char htmlData[65535];
    char jsonpData[65535];

    snprintf(render, 1024, "%s/render", proj);
    snprintf(json, 1024, "%s/Render_Json.php", render);
    snprintf(html, 1024, "%s/Render_Html.php", render);
    snprintf(jsonp, 1024, "%s/Render_Jsonp.php", render);

    if (mkdir(render, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("|---create render...success\n");
    } else {
        printf("|---create render...failure\n");
        return;
    }

    FILE *jsonfd = fopen(json, "w+");
    play_interface_play_generate_render_json(jsonData);
    fwrite(jsonData, strlen(jsonData), 1, jsonfd);
    fclose(jsonfd);
    printf("  |--create Render_Json.php...success\n");

    FILE *htmlfd = fopen(html, "w+");
    play_interface_play_generate_render_html(htmlData);
    fwrite(htmlData, strlen(htmlData), 1, htmlfd);
    fclose(htmlfd);
    printf("  |--create Render_Html.php...success\n");

    FILE *jsonpfd = fopen(jsonp, "w+");
    play_interface_play_generate_render_jsonp(jsonpData);
    fwrite(jsonpData, strlen(jsonpData), 1, jsonpfd);
    fclose(jsonpfd);
    printf("  |--create Render_Jsonp.php...success\n");
}

static void play_interface_play_init_create_assets(const char *proj)
{
    char assets[1024];
    char action[1024];
    char meta[1024];

    snprintf(assets, 1024, "%s/assets",proj);
    snprintf(action, 1024, "%s/action", assets);
    snprintf(meta, 1024, "%s/meta", assets);

    if (mkdir(assets, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("|---create assets...success\n");
    } else {
        printf("|---create assets...failure\n");
        return;
    }
    if (mkdir(action, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("  |---create action...success\n");
    } else {
        printf("  |---create action...failure\n");
        return;
    }
    if (mkdir(meta, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        printf("  |---create meta...success\n");
    } else {
        printf("  |---create meta...failure\n");
        return;
    }
}

static void play_interface_play_generate_render_json(char *src)
{
    sprintf(src, "<?php\n\
class Render_Json extends Render_Abstract\n\
{\n\
    public function setHeader()\n\
    {\n\
        header('Content-Type:application/json; charset=utf-8');\n\
        header('Cache-Control: no-cache, must-revalidate, max-age=0');\n\
    }\n\
\n\
    public function run()\n\
    {\n\
        $data = Output::get();\n\
        $doc = array_merge(array('rc'=>0, 'tm'=>time()), $data);\n\
        echo json_encode($doc);\n\
    }\n\
\n\
    public function exception(Exception $e)\n\
    {\n\
        var_dump($e);\n\
    }\n\
}\
");
}

static void play_interface_play_generate_render_jsonp(char *src)
{
    sprintf(src, "<?php\n\
class Render_Jsonp extends Render_Abstract\n\
{\n\
    public function setHeader()\n\
    {\n\
        header('Content-Type:application/json; charset=utf-8');\n\
        header('Cache-Control: no-cache, must-revalidate, max-age=0');\n\
    }\n\
\n\
    public function run()\n\
    {\n\
        $data = Output::get();\n\
        $doc = array_merge(array('rc'=>0, 'tm'=>time()), $data);\n\
        $callback = Action::$input->getInGet('callback', '/\\S/');\n\
        echo $callback.'('.json_encode($doc).')';\n\
    }\n\
\n\
    public function exception(Exception $e)\n\
    {\n\
        var_dump($e);\n\
    }\n\
}\
");
}

static void play_interface_play_generate_router_mysql(char *src)
{
    sprintf(src, "<?php\n\
class Meta_Router_Mysql extends Meta_Router_Abstract\n\
{\n\
    private $_config;\n\
\n\
    public function __construct(Query $query)\n\
    {\n\
\n\
    }\n\
\n\
    public function getList(Query $query)\n\
    {\n\
        \n\
    }\n\
\n\
    public function getOne(Query $query)\n\
    {\n\
        \n\
    }\n\
\n\
    public function insert(Query $query)\n\
    {\n\
        \n\
    }\n\
\n\
    public function update(Query $query)\n\
    {\n\
        \n\
    }\n\
\n\
    public function count(Query $query)\n\
    {\n\
        \n\
    }\n\
\n\
    public function delete(Query $query)\n\
    {\n\
        \n\
    }\n\
}\
");
}

static void play_interface_play_generate_render_html(char *src)
{
    sprintf(src, "<?php\n\
class Render_Html extends Render_Abstract\n\
{\n\
    public function setHeader()\n\
    {\n\
        header('Content-Type:text/html;charset=utf-8');\n\
        header('Cache-Control: no-cache, must-revalidate, max-age=0');\n\
    }\n\
\n\
    public function run()\n\
    {\n\
        $data = Output::get();\n\
        require '../source/templates/' . Action::$template . '.html';\n\
    }\n\
\n\
    public function exception($e)\n\
    {\n\
        var_dump($e);\n\
    }\n\
}\
");
}

static void play_interface_play_generate_core_api(char *src)
{
    sprintf(src, "<?php\n\
class Input\
{\n\
    public static function getInPost($key = null, $regex = null, $default = null){}\n\
    public static function getInHeader($key = null, $regex = null, $default = null){}\n\
    public static function getInGet($key = null, $regex = null, $default = null){}\n\
    public static function getInCookie($key = null, $regex = null, $default = null){}\n\
    public static function getInRequest($key = null, $regex = null, $default = null){}\n\
}\n\
\n\
class Output\n\
{\n\
    public static function get($key = null){}\n\
    public static function set($key, $value){}\n\
}\n\
\n\
class Context\n\
{\n\
    public static function get($key = null){}\n\
    public static function set($key, $value){}\n\
}\n\
\n\
class Action\n\
{\n\
    public static $url;\n\
    public static $name;\n\
    public static $render;\n\
    public static $template;\n\
}\n\
\n\
abstract class Processor\n\
{\n\
    public abstract function run();\n\
}\n\
\n\
abstract class Render_Abstract\n\
{\n\
    public abstract function __construct();\n\
    public abstract function setHeader();\n\
    public abstract function run();\n\
    public abstract function exception(Throwable $e);\n\
}\n\
\n\
class Query\n\
{\n\
    public function exec($arg){}\n\
    public function getList($field = null){}\n\
    public function getOne($field = null){}\n\
    public function insert(){}\n\
    public function count(){}\n\
    public function update(){}\n\
    public function delete(){}\n\
    public function limit($start, $count){return $this;}\n\
    public function orderBy($field, $desc){return $this;}\n\
}\n\
\n\
abstract class Meta_Router_Abstract\n\
{\n\
    public abstract function getList(Query $q);\n\
    public abstract function getOne(Query $q);\n\
    public abstract function insert(Query $q);\n\
    public abstract function count(Query $q);\n\
    public abstract function update(Query $q);\n\
    public abstract function delete(Query $q);\n\
}\n\
\n\
abstract class Crontab\n\
{\n\
    public abstract function run();\n\
    public function checkHit(){}\n\
    public function getCrontab(){}\n\
    public static function debug($app_root, $class){}\n\
    protected function setCrontab(){return $this;}\n\
    protected function setMutex(){return $this;}\n\
    protected function everySeconds(){return $this;}\n\
}\n\
\n\
abstract class WebSocket\n\
{\n\
    public abstract function onShakehands($conn_sock, $message);\n\
    public abstract function onCommunicate($conn_sock, $message);\n\
    public abstract function onClose($conn_sock);\n\
    protected function send($sock_conn, $message){}\n\
}\n\
\n\
class Play\n\
{\n\
    public static $auto = false;\n\
    public static $environment = null;\n\
    public static $render = 'html';\n\
    public static function init($projName=''){}\n\
    public static function reconst(){}\n\
    public static function crontab($root_path, $environment='release'){}\n\
}\n\
\n\
class NetKit\n\
{\n\
    public static function socket_protocol($host, $port, $cmd, $data, $respond = true, $timeout = 1){}\n\
    public static function socket_fastcgi($host, $port, $params, $body, $respond = true, $timeout = 1){}\n\
}\
");
}