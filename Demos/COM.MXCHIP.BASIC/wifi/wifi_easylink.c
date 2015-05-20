#include "MICO.h"
#include "MICORTOS.h"
#include "MICODefine.h"
#include "StringUtils.h"
#include "MICONotificationCenter.h"

static int is_easylink_success;
static char ssid[64], key[32];
static mico_semaphore_t      easylink_sem;
extern void StationModeStart( char ssid[64], char key[32] );
void easylink_thread(  );

#define easylink_log(M, ...) custom_log("wifi_easylink", M, ##__VA_ARGS__)
#define easylink_log_trace() custom_log_trace("wifi_easylink")

void EasyLinkNotify_EasyLinkCompleteHandler(network_InitTypeDef_st *nwkpara, mico_Context_t * const inContext)
{
  OSStatus err;
  easylink_log("EasyLink return");
  require_action(nwkpara, exit, err = kTimeoutErr);
  strcpy(ssid, nwkpara->wifi_ssid);
  strcpy(key, nwkpara->wifi_key);
  easylink_log("Get SSID: %s, Key: %s", nwkpara->wifi_ssid, nwkpara->wifi_key);
  return;
  
exit:
  easylink_log("ERROR, err: %d", err);
  mico_rtos_set_semaphore(&easylink_sem);
}

void EasyLinkNotify_EasyLinkGetExtraDataHandler(int datalen, char* data, mico_Context_t * const inContext)
{
  OSStatus err;
  int index ;
  char address[16];
  char *debugString;
  uint32_t *ipInfo, ipInfoCount;
  debugString = DataToHexStringWithSpaces( (const uint8_t *)data, datalen );
  easylink_log("Get user info: %s", debugString);
  free(debugString);
  
  for(index = datalen - 1; index>=0; index-- ){
    if(data[index] == '#' &&( (datalen - index) == 5 || (datalen - index) == 25 ) )
      break;
  }
  require_action(index >= 0, exit, err = kParamErr);
  
  data[index++] = 0x0;
  ipInfo = (uint32_t *)&data[index];
  ipInfoCount = (datalen - index)/sizeof(uint32_t);
  require_action(ipInfoCount >= 1, exit, err = kParamErr);
  
  inet_ntoa( address, *(uint32_t *)(ipInfo) );
  easylink_log("Get auth info: %s, EasyLink server ip address: %s", data, address);
  is_easylink_success = 1;
  mico_rtos_set_semaphore(&easylink_sem);
  return;
  
exit:
  easylink_log("ERROR, err: %d", err); 
}

void clean_easylink_resource( )
{
  MICORemoveNotification( mico_notify_EASYLINK_WPS_COMPLETED, (void *)EasyLinkNotify_EasyLinkCompleteHandler );
  MICORemoveNotification( mico_notify_EASYLINK_GET_EXTRA_DATA, (void *)EasyLinkNotify_EasyLinkGetExtraDataHandler );
  
  mico_rtos_deinit_semaphore(&easylink_sem);
  easylink_sem = NULL;
}

void starteasylink( void )
{
  OSStatus err;
  is_easylink_success = 0;
  memset(ssid, 0x0, sizeof(ssid));
  memset(key, 0x0, sizeof(key));
  err = MICOAddNotification( mico_notify_EASYLINK_WPS_COMPLETED, (void *)EasyLinkNotify_EasyLinkCompleteHandler );
  require_noerr(err, exit);
  err = MICOAddNotification( mico_notify_EASYLINK_GET_EXTRA_DATA, (void *)EasyLinkNotify_EasyLinkGetExtraDataHandler );
  require_noerr(err, exit);
  
  // Start the EasyLink thread
  mico_rtos_init_semaphore(&easylink_sem, 1);
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "EASYLINK", easylink_thread, 0x800, NULL );
  require_noerr_action( err, exit, easylink_log("ERROR: Unable to start the EasyLink thread.") );
  return;

exit:
  easylink_log("ERROR, err: %d", err);
}

void easylink_thread(  )
{
  micoWlanStartEasyLink( 40 );
  easylink_log("Start Easylink configuration");
  mico_rtos_get_semaphore(&easylink_sem, MICO_WAIT_FOREVER);
  
  if ( is_easylink_success == 1 )
  {
    mico_thread_msleep(10);
    StationModeStart(ssid, key);
  } else {
    easylink_log("Easylink configuration fail");
  }
  
  clean_easylink_resource();
  mico_rtos_delete_thread(NULL);
}





