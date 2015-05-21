#include "MICO.h"
#include "MICORTOS.h"

#define dns_log(M, ...) custom_log("wifi_dns", M, ##__VA_ARGS__)
#define dns_log_trace() custom_log_trace("wifi_dns")

void dns_thread( );
static char domain[32];

void start_dns( char *domain_name )
{
  OSStatus err;
  strcpy(domain, domain_name);
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "DNS", dns_thread, 0x800, NULL );
  require_noerr_action( err, exit, dns_log("ERROR: Unable to start the DNS thread.") );
  return;

exit:
  dns_log("ERROR, err: %d", err);
}

void dns_thread( )
{
  char ipstr[16];
  
  OSStatus err;
  dns_log("start domain name system");
  
  while(1) 
  {
     err = gethostbyname(domain, (uint8_t *)ipstr, 16);
     require_noerr(err, ReConnWithDelay);
     dns_log("%s ip address is %s",domain, ipstr);
     break;

   ReConnWithDelay:
     dns_log("please wait 5s...");
     mico_thread_sleep(5);
  }
      
  mico_rtos_delete_thread(NULL);
}

