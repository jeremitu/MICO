#include "MICO.h"
#include "MICORTOS.h"

static int udp_port;
void udp_echo_thread( );

#define BUF_LEN (3*1024)
#define udp_echo_log(M, ...) custom_log("UDP", M, ##__VA_ARGS__)
#define udp_echo_trace() custom_log_trace("UDP")


void start_udp_echo( int port )
{
  OSStatus err;
  udp_port = port;
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UDP_ECHO", udp_echo_thread, 0x800, NULL );
  require_noerr_action( err, exit, udp_echo_log("ERROR: Unable to start the UDP echo thread.") );
  return;

exit:
  udp_echo_log("ERROR, err: %d", err);
}

void udp_echo_thread( )
{
  OSStatus err;
  struct sockaddr_t addr;
  struct timeval_t t;
  fd_set readfds;
  socklen_t addrLen;
  int udp_fd = -1 , len;
  char *buf;
  
  buf = (char*)malloc(BUF_LEN);
  require_action(buf, exit, err = kNoMemoryErr);
  
  /*Establish a UDP port to receive any data sent to this port*/
  udp_fd = socket( AF_INET, SOCK_DGRM, IPPROTO_UDP );
  require_action(IsValidSocket( udp_fd ), exit, err = kNoResourcesErr );
  addr.s_ip = INADDR_ANY;
  addr.s_port = udp_port;
  err = bind(udp_fd, &addr, sizeof(addr));
  require_noerr( err, exit );
  udp_echo_log("Start UDP echo mode, Open UDP port %d", addr.s_port);
  
  t.tv_sec = 20;
  t.tv_usec = 0;
  
  while(1)
  {
    /*Check status on erery sockets */
    FD_ZERO(&readfds);
    FD_SET(udp_fd, &readfds);

    select(1, &readfds, NULL, NULL, &t);
    
    /*Read data from udp and send data back */ 
    if (FD_ISSET( udp_fd, &readfds )) 
    {
      len = recvfrom(udp_fd, buf, BUF_LEN, 0, &addr, &addrLen);
      udp_echo_log("[udprec][%d] = %.*s", len, len, buf);
      sendto(udp_fd, buf, len, 0, &addr, sizeof(struct sockaddr_t));
    }
  }
  
exit:
  mico_rtos_delete_thread(NULL);
}

