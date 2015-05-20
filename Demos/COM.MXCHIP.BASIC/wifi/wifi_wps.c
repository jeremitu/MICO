#include "MICO.h"
#include "MICODefine.h"
#include "StringUtils.h"
#include "MICONotificationCenter.h"

static int is_wps_success;
static char ssid[64], key[32];
static mico_semaphore_t      wps_sem;
extern void StationModeStart( char ssid[64], char key[32] );
void wps_thread(  );

#define wps_log(M, ...) custom_log("wifi_wps", M, ##__VA_ARGS__)
#define wps_log_trace() custom_log_trace("wifi_wps")

void WPSNotify_WPSCompleteHandler(network_InitTypeDef_st *nwkpara, mico_Context_t * const inContext)
{
  OSStatus err;
  wps_log("WPS return");
  require_action(nwkpara, exit, err = kTimeoutErr);
  strcpy(ssid, nwkpara->wifi_ssid);
  strcpy(key, nwkpara->wifi_key);
  wps_log("Get SSID: %s, Key: %s", nwkpara->wifi_ssid, nwkpara->wifi_key);
  is_wps_success = 1;
  mico_rtos_set_semaphore(&wps_sem);
  return;
  
exit:
  wps_log("ERROR, err: %d", err); 
  mico_rtos_set_semaphore(&wps_sem);
}

void clean_wps_sesource( )
{
  MICORemoveNotification( mico_notify_EASYLINK_WPS_COMPLETED, (void *)WPSNotify_WPSCompleteHandler );
  micoWlanStopWPS();
  mico_rtos_deinit_semaphore(&wps_sem);
  wps_sem = NULL;
}

void startwps( void )
{
  OSStatus err;
  is_wps_success = 0;
  memset(ssid, 0x0, sizeof(ssid));
  memset(key, 0x0, sizeof(key));
  err = MICOAddNotification( mico_notify_EASYLINK_WPS_COMPLETED, (void *)WPSNotify_WPSCompleteHandler );
  require_noerr(err, exit);
  
  // Start the WPS thread
  mico_rtos_init_semaphore(&wps_sem, 1);
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "WPS", wps_thread, 0x1000, NULL );
  require_noerr_action( err, exit, wps_log("ERROR: Unable to start the WPS thread.") );
  return;
  
exit:
  wps_log("ERROR, err: %d", err);
}

void wps_thread( )
{
  micoWlanStartWPS( 40 );
  wps_log("Start WPS configuration");
  mico_rtos_get_semaphore(&wps_sem, MICO_WAIT_FOREVER);
  
  if ( is_wps_success == 1 )
  {
    mico_thread_msleep(10);
    StationModeStart(ssid, key);
  } else {
    wps_log("WPS configuration fail");
  }
  msleep(20);
  clean_wps_sesource();
  mico_rtos_delete_thread(NULL);
}

