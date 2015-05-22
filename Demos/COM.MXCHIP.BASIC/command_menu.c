#include "command_menu.h"

char os_help[] =
"\r\n"
"+--------- command --------------+------- function ---------+\r\n"
"| os                             | Os command help          |\r\n"
"| os     sem                     | Open semaphore example   |\r\n"
"| os     mutex                   | Open mutex example       |\r\n"
"| os     queue                   | Open queue example       |\r\n"
"| os     timer   <seconds>       | Open timer example       |\r\n"
"+--------------------------------+--------------------------+\r\n"
;

char wifi_help[] =
"\r\n"
"+--------- command --------------+------- function ---------+\r\n"
"| wifi                           | WiFi command help        |\r\n"
"| wifi   easylink                | Open easylink mode       |\r\n"
"| wifi   wps                     | Open wps mode            |\r\n"
"| wifi   airkiss                 | Open airkiss mode        |\r\n"
"| wifi   ap    <ssid> <key>      | Start AP mode            |\r\n"
"| wifi   sta   <ssid> <key>      | Start Station mode       |\r\n"
"| wifi   scan                    | Start Scan mode          |\r\n"
"| wifi   dns   <domain>          | Start domain name system |\r\n"
"| wifi   stop                    | Suspend wifi             |\r\n"
"+--------------------------------+--------------------------+\r\n"
;

char udp_help[] =
"\r\n"
"+--------- command --------------+------- function ---------+\r\n"
"| udp                            | UDP command help         |\r\n"
"| udp    bonjour                 | Open bonjour example     |\r\n"
"| udp    echo  <port>            | Start UDP echo mode      |\r\n"
"+--------------------------------+--------------------------+\r\n"
;

char tcp_help[] =
"\r\n"
"+--------- command --------------+------- function ---------+\r\n"
"| tcp                            | TCP command help         |\r\n"
"| tcp    server  <port>          | Start TCP server mode    |\r\n"
"| tcp    client  <ip> <port>     | Start TCP client mode    |\r\n"
"+--------------------------------+--------------------------+\r\n"
;

char http_help[] =
"\r\n"
"+--------- command --------------+------- function ---------+\r\n"
"| http                           | HTTP command help        |\r\n"
"| http   server                  | Start http server mode   |\r\n"
"| http   client  <domain>        | Start http client mode   |\r\n"
"| http   ssl  client  <domain>   | Start http client mode   |\r\n"
"+--------------------------------+--------------------------+\r\n"
;

char ps_help[] =
"\r\n"
"+--------- command --------------+------- function ---------+\r\n"
"| ps                             | PS command help          |\r\n"
"| ps     mcu     start           | Start mcu powersave mode |\r\n"
"| ps     mcu     stop            | Stop mcu powersave mode  |\r\n"
"| ps     rf      start           | Start rf powersave mode  |\r\n"
"| ps     rf      stop            | Stop rf powersave mode   |\r\n"
"+--------------------------------+--------------------------+\r\n"
;

static void command_list_print( void )
{
    
    printf("%s", os_help);
    printf("%s", wifi_help);
    printf("%s", udp_help);
    printf("%s", tcp_help);
    printf("%s", http_help);
    printf("%s", ps_help);
}

static void eg_list_Command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
    command_list_print( );
}

static void os_Command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
    if (strcmp(argv[1], "sem") == 0) {
        return;
    } else if (strcmp(argv[1], "mutex") == 0) {
        return;
    } else if (strcmp(argv[1], "queue") == 0) {
        return;
    } else if (strcmp(argv[1], "timer") == 0) {
        if ( argc == 3 ) {
            return;
        }
    } 
    cmd_printf("%s", os_help);
}

static void wifi_Command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{   
    if (strcmp(argv[1], "easylink") == 0) {
          starteasylink( );
          return;
    } else if (strcmp(argv[1], "wps") == 0) {
        startwps( );
        return;
    } else if (strcmp(argv[1], "airkiss") == 0) {
        startairkiss( );
        return;
    } else if (strcmp(argv[1], "ap") == 0) {
        if ( argc == 3 ) {
            SoftAPModeStart( argv[2], "\0" );
            return;
        } else if ( argc == 4 ) {
            SoftAPModeStart( argv[2], argv[3] );
            return;
        }
    } else if (strcmp(argv[1], "sta") == 0) {
        if( argc == 3 ) {
            StationModeStart( argv[2], "\0" );
            return;
        } else if ( argc == 4 ) {
            StationModeStart( argv[2], argv[3] );
            return;
        }
    } else if (strcmp(argv[1], "scan") == 0) {
        ScanMode( );
        return;
    } else if (strcmp(argv[1], "dns") == 0) {
        if ( argc == 3 ) {
            start_dns(argv[2]);
            return;
        }
    }
    else if (strcmp(argv[1], "stop") == 0) {
        wifistop( );
        return;
    } else 
    cmd_printf("%s", wifi_help);
}

static  void udp_Command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
    int udp_port;
    if (strcmp(argv[1], "echo") == 0) {
        if ( argc == 3 ) {
            udp_port = atoi(argv[2]);
            start_udp_echo( udp_port );
            return;
        }
    } else if (strcmp(argv[1], "bonjour") == 0) {
        return;
    }
    cmd_printf("%s", udp_help);
}

static  void tcp_Command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
    int tcp_port;
    if (strcmp(argv[1], "server") == 0) {
        if ( argc == 3 ) {
            tcp_port = atoi(argv[2]);
            start_tcp_server( tcp_port );
            return;
        }
    } else if (strcmp(argv[1], "client") == 0) {
        if ( argc == 4 ) {
            tcp_port = atoi(argv[3]);
            start_tcp_client( argv[2], tcp_port );
            return;
        }
    } 
    cmd_printf("%s", tcp_help);
}

static  void http_Command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
    if (strcmp(argv[1], "server") == 0) {
        return;
    } else if (strcmp(argv[1], "client") == 0) {
        if ( argc == 3 ) {
            return;
        }
    } else if (strcmp(argv[1], "client") == 0) {
        if ( argc == 4 ) {
            return;
        }
    }
    cmd_printf("%s", tcp_help);
}

static  void ps_Command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
    if (strcmp(argv[1], "mcu") == 0) {
        if (strcmp(argv[2], "start") == 0) {
            start_ps_mcu( );
            return;
        } else if (strcmp(argv[2], "stop") == 0) {
            stop_ps_mcu( );
            return;
        }
    } else if (strcmp(argv[1], "rf") == 0) {
        if (strcmp(argv[2], "start") == 0) {
            start_rf_mcu( );
            return;
        } else if (strcmp(argv[2], "stop") == 0) {
            stop_rf_mcu( );
            return;
        }
    }
    cmd_printf("%s", ps_help);
}

static const struct cli_command user_clis[] = {
    {"eg_list",   "show example command", eg_list_Command},
    {"os",        "os example",           os_Command},
    {"wifi",      "wifi example",         wifi_Command},
    {"udp",       "udp example",          udp_Command},
    {"tcp",       "tcp example",          tcp_Command},
    {"http",      "http example",         http_Command},
    {"ps",        "powersave mode",       ps_Command},
};

void register_commands( void )
{
    cli_register_commands(user_clis, sizeof(user_clis)/sizeof(struct cli_command));
    command_list_print( );
}

