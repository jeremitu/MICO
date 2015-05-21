#include "MICO.h"
#include "MICONotificationCenter.h"

static network_InitTypeDef_adv_st wNetConfigAdv;

#define station_log(M, ...) custom_log("wifi_station", M, ##__VA_ARGS__)
#define softap_log_trace() custom_log_trace("wifi_station")

void StationModeStart( char ssid[64], char key[32] )
{
  station_log("connect to %s...", ssid);
  
  memset(&wNetConfigAdv, 0x0, sizeof(network_InitTypeDef_adv_st));
  
  strcpy((char*)wNetConfigAdv.ap_info.ssid, ssid);
  strcpy((char*)wNetConfigAdv.key, key);
  wNetConfigAdv.key_len = strlen(key);
  wNetConfigAdv.ap_info.channel = 0; //Auto
  wNetConfigAdv.dhcpMode = DHCP_Client;
  wNetConfigAdv.ap_info.security = SECURITY_TYPE_AUTO;
  wNetConfigAdv.wifi_retry_interval = 100;
  micoWlanStartAdv(&wNetConfigAdv);
}

void wifistop( void )
{
  station_log("stop wifi");
  micoWlanSuspend();
}
