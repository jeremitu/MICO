#ifndef __COMMAND_MENU_H
#define __COMMAND_MENU_H

#include "micocli.h"
#include "common.h"

void starteasylink( void );
void startwps( void );
void startairkiss( void );
void SoftAPModeStart( char *ssid, char *key );
void StationModeStart( char ssid[64], char key[32] );
void ScanMode( void );
void start_dns( char *domain_name );
void wifistop( void );

void start_udp_echo( int port );

void start_tcp_server( int port );
void start_tcp_client( char remote_ip[16], int port );

void start_ps_mcu( void );
void stop_ps_mcu( void );

void start_rf_mcu( void );
void stop_rf_mcu( void );

#endif