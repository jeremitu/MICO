#include "MICO.h"
#include "MICODefine.h"
#include "StringUtils.h"
#include "MICONotificationCenter.h"

static int is_airkiss_success;
static uint8_t airkiss_data;
static char ssid[64], key[32];
static mico_semaphore_t      airkiss_sem;
extern void StationModeStart( char ssid[64], char key[32] );
void airkiss_thread(  );

#define Airkiss_ConnectWlan_Timeout    20000
#define airkiss_log(M, ...) custom_log("wifi_airkiss", M, ##__VA_ARGS__)
#define airkiss_log_trace() custom_log_trace("wifi_airkiss")

void AirkissNotify_WifiStatusHandler(WiFiEvent event, mico_Context_t * const inContext)
{
  airkiss_log_trace();
  switch (event) {
  case NOTIFY_STATION_UP:
    airkiss_log("Access point connected");
    mico_rtos_set_semaphore(&airkiss_sem);
    MicoRfLed(true);
    break;
  case NOTIFY_STATION_DOWN:
    MicoRfLed(false);
    break;
  case NOTIFY_AP_UP:
    MicoRfLed(true);
    break;
  case NOTIFY_AP_DOWN:
    MicoRfLed(false);
    break;
  default:
    break;
  }

}

void AirkissNotify_AirkissCompleteHandler(network_InitTypeDef_st *nwkpara, mico_Context_t * const inContext)
{
  OSStatus err;
  airkiss_log("airkiss return");
  require_action(nwkpara, exit, err = kTimeoutErr);
  strcpy(ssid, nwkpara->wifi_ssid);
  strcpy(key, nwkpara->wifi_key);
  airkiss_log("Get SSID: %s, Key: %s", nwkpara->wifi_ssid, nwkpara->wifi_key);
  return;
  
exit:
  airkiss_log("ERROR, err: %d", err);
  mico_rtos_set_semaphore(&airkiss_sem);
}

void AirkissNotify_AirkissGetExtraDataHandler(int datalen, char* data, mico_Context_t * const inContext)
{
  OSStatus err;
  require_action(inContext, exit, err = kParamErr);
  char *debugString;

  debugString = DataToHexStringWithSpaces( (const uint8_t *)data, datalen );
  airkiss_log("Get user info: %s", debugString);
  free(debugString);
  airkiss_data = data[0];
  is_airkiss_success = 1;
  mico_rtos_set_semaphore(&airkiss_sem);
  return;
  
exit:
  airkiss_log("ERROR, err: %d", err);
}

void clean_airkiss_resource( )
{
  MICORemoveNotification( mico_notify_EASYLINK_WPS_COMPLETED, (void *)AirkissNotify_AirkissCompleteHandler );
  MICORemoveNotification( mico_notify_EASYLINK_GET_EXTRA_DATA, (void *)AirkissNotify_AirkissGetExtraDataHandler );
  MICORemoveNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)AirkissNotify_WifiStatusHandler );
    
  mico_rtos_deinit_semaphore(&airkiss_sem);
  airkiss_sem = NULL;
}

void startairkiss( void )
{
  OSStatus err;
  is_airkiss_success = 0;
  memset(ssid, 0x0, sizeof(ssid));
  memset(key, 0x0, sizeof(key));
  err = MICOAddNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)AirkissNotify_WifiStatusHandler );
  require_noerr(err, exit);
  err = MICOAddNotification( mico_notify_EASYLINK_WPS_COMPLETED, (void *)AirkissNotify_AirkissCompleteHandler );
  require_noerr(err, exit);
  err = MICOAddNotification( mico_notify_EASYLINK_GET_EXTRA_DATA, (void *)AirkissNotify_AirkissGetExtraDataHandler );
  require_noerr(err, exit);
  
  // Start the Airkiss thread
  mico_rtos_init_semaphore(&airkiss_sem, 1);
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "EASYLINK", airkiss_thread, 0x800, NULL );
  require_noerr_action( err, exit, airkiss_log("ERROR: Unable to start the Airkiss thread.") );
  return;
  
exit:
  airkiss_log("ERROR, err: %d", err);
}

void airkiss_thread( )
{
  int fd;
  struct sockaddr_t addr;
  int i = 0;
  
  micoWlanStartAirkiss( 40 );
  airkiss_log("Start Airkiss configuration");
  mico_rtos_get_semaphore(&airkiss_sem, MICO_WAIT_FOREVER);
  
  if ( is_airkiss_success == 1 )
  {
    mico_thread_msleep(10);
    StationModeStart(ssid, key);
    
    mico_rtos_get_semaphore(&airkiss_sem, Airkiss_ConnectWlan_Timeout);
    fd = socket( AF_INET, SOCK_DGRM, IPPROTO_UDP );
    if (fd < 0)
      goto threadexit;
    
    addr.s_ip = INADDR_BROADCAST;
    addr.s_port = 10000;
    airkiss_log("Send UDP to WECHAT");
    while(1){
      sendto(fd, &airkiss_data, 1, 0, &addr, sizeof(addr));
      
      msleep(10);
      i++;
      if (i > 20)
        break;
    }  

  } else {
    airkiss_log("Airkiss configuration fail");
  }
  
threadexit:  
  clean_airkiss_resource( );
  mico_rtos_delete_thread(NULL);
}

