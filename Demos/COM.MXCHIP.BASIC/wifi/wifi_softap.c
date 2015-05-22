#include "MICO.h"
#include "MICONotificationCenter.h"

static network_InitTypeDef_st wNetConfig;

#define softap_log(M, ...) custom_log("wifi_softap", M, ##__VA_ARGS__)
#define softap_log_trace() custom_log_trace("wifi_softap")


void SoftAPModeStart( char ssid[64], char key[32] )
{
  softap_log("Soft AP Start");
  
  memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));
  
  strcpy((char*)wNetConfig.wifi_ssid, ssid);
  strcpy((char*)wNetConfig.wifi_key, key);
  
  wNetConfig.wifi_mode = Soft_AP;
  wNetConfig.dhcpMode = DHCP_Server;
  wNetConfig.wifi_retry_interval = 100;
  strcpy((char*)wNetConfig.local_ip_addr, "192.168.0.1");
  strcpy((char*)wNetConfig.net_mask, "255.255.255.0");
  strcpy((char*)wNetConfig.dnsServer_ip_addr, "192.168.0.1");
  micoWlanStart(&wNetConfig);
  
  softap_log("SoftAP ssid:%s key:%s", wNetConfig.wifi_ssid, wNetConfig.wifi_key);
}
