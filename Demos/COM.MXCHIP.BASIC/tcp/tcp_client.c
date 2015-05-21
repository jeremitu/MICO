#include "MICO.h"
#include "MICORTOS.h"
#include "SocketUtils.h"

static int tcp_port;
static char tcp_remote_ip[16];
void tcp_client_thread( );

#define BUF_LEN (3*1024)
#define tcp_client_log(M, ...) custom_log("TCP", M, ##__VA_ARGS__)
#define tcp_client_trace() custom_log_trace("TCP")


void start_tcp_client( char remote_ip[16], int port )
{
  OSStatus err;
  strcpy(tcp_remote_ip, remote_ip);
  tcp_port = port;
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "TCP_client", tcp_client_thread, 0x800, NULL );
  require_noerr_action( err, exit, tcp_client_log("ERROR: Unable to start the tcp client thread.") );
  return;

exit:
  tcp_client_log("ERROR, err: %d", err);
}

void tcp_client_thread( )
{
  OSStatus err;
  struct sockaddr_t addr;
  struct timeval_t t;
  fd_set readfds;
  int tcp_fd = -1 , len;
  char *buf;
  
  buf = (char*)malloc(BUF_LEN);
  require_action(buf, exit, err = kNoMemoryErr);
  
  while(1)
  {
    if ( tcp_fd == -1 ) 
    {
      tcp_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
      require_action(IsValidSocket( tcp_fd ), exit, err = kNoResourcesErr );
      addr.s_ip = inet_addr(tcp_remote_ip);
      addr.s_port = tcp_port;
      err = connect(tcp_fd, &addr, sizeof(addr));
      require_noerr_quiet(err, ReConnWithDelay);
      tcp_client_log("Remote server connected at port: %d, fd: %d",  addr.s_port, tcp_fd);
    }
    else
    {
      /*Check status on erery sockets */
      FD_ZERO(&readfds);
      FD_SET(tcp_fd, &readfds);
      t.tv_sec = 4;
      t.tv_usec = 0;

      select(1, &readfds, NULL, NULL, &t);
      
      /*recv wlan data using remote client fd*/
      if (FD_ISSET( tcp_fd, &readfds )) 
      {
        len = recv(tcp_fd, buf, BUF_LEN, 0);
        if( len <= 0) {
          tcp_client_log("Remote client closed, fd: %d", tcp_fd);
          goto ReConnWithDelay;
        }
        
        tcp_client_log("[tcprec][%d] = %.*s", len, len, buf);
        sendto(tcp_fd, buf, len, 0, &addr, sizeof(struct sockaddr_t));
      }
   
      continue;
      
    ReConnWithDelay:
        if(tcp_fd != -1){
          SocketClose(&tcp_fd);
        }
        tcp_client_log("Connect to %s failed! Reconnect in 5 sec...", tcp_remote_ip);
        sleep( 5 );
    }
  }
  
exit:
  mico_rtos_delete_thread(NULL);
}

