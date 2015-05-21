#include "MICO.h"
#include "MICONotificationCenter.h"

static int isaddscannotification = 0;

#define sacn_log(M, ...) custom_log("wifi_scan", M, ##__VA_ARGS__)
#define scan_log_trace() custom_log_trace("wifi_scan")

void micoNotify_ApListCallback(ScanResult *pApList)
{
  int i;
  
  printf("\r\ngot %d AP\r\n", pApList->ApNum);
  for(i=0; i<pApList->ApNum; i++) {
    printf("%d\t%s \t%d\r\n", i, pApList->ApList[i].ssid, pApList->ApList[i].ApPower);
  }
  isaddscannotification = 1;
}

void ScanMode( void )
{
  sacn_log("start scan mode");
  if( isaddscannotification == 0 )
    MICOAddNotification( mico_notify_WIFI_SCAN_COMPLETED, (void *)micoNotify_ApListCallback );
  micoWlanStartScan( );
}
