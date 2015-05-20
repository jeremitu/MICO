#include "MICO.h"
#include "MICORTOS.h"
#include "SocketUtils.h"

static int tcp_port;
void tcp_server_thread( );
static void tcp_server_client_thread( void *inFd );

#define BUF_LEN (3*1024)
#define tcp_server_log(M, ...) custom_log("TCP", M, ##__VA_ARGS__)
#define tcp_server_trace() custom_log_trace("TCP")


void start_tcp_server( int port )
{
  OSStatus err;
  tcp_port = port;
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "TCP_server", tcp_server_thread, 0x800, NULL );
  require_noerr_action( err, exit, tcp_server_log("ERROR: Unable to start the tcp server thread.") );
  return;

exit:
  tcp_server_log("ERROR, err: %d", err);
}

void tcp_server_thread( )
{
  OSStatus err;
  int j;
  struct sockaddr_t addr;
  int sockaddr_t_size;
  fd_set readfds;
  char ip_address[16];
  
  int tcp_listener_fd = -1;

  /*Establish a TCP server fd that accept the tcp clients connections*/ 
  tcp_listener_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  require_action(IsValidSocket( tcp_listener_fd ), exit, err = kNoResourcesErr );
  addr.s_ip = INADDR_ANY;
  addr.s_port = tcp_port;
  err = bind(tcp_listener_fd, &addr, sizeof(addr));
  require_noerr( err, exit );

  err = listen(tcp_listener_fd, 0);
  require_noerr( err, exit );

  tcp_server_log("Server established at port: %d, fd: %d", addr.s_port, tcp_listener_fd);
  
  while(1){
    FD_ZERO(&readfds);
    FD_SET(tcp_listener_fd, &readfds);  
    select(1, &readfds, NULL, NULL, NULL);

    /*Check tcp connection requests */
    if(FD_ISSET(tcp_listener_fd, &readfds)){
      sockaddr_t_size = sizeof(struct sockaddr_t);
      j = accept(tcp_listener_fd, &addr, &sockaddr_t_size);
      if (j > 0) {
        inet_ntoa(ip_address, addr.s_ip );
        tcp_server_log("Client %s:%d connected, fd: %d", ip_address, addr.s_port, j);
        if(kNoErr != mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Local Clients", tcp_server_client_thread, 0x800, &j) ) 
          SocketClose(&j);
      }
    }
   }

exit:
    tcp_server_log("Exit: Local controller exit with err = %d", err);
    mico_rtos_delete_thread(NULL);
    return;
}

void tcp_server_client_thread( void *inFd )
{
  OSStatus err;
  struct sockaddr_t addr;
  struct timeval_t t;
  fd_set readfds;
  int client_fd = *(int *)inFd; 
  int len;
  char *buf;
  
  buf = (char*)malloc(BUF_LEN);
  require_action(buf, exit, err = kNoMemoryErr);
  
  t.tv_sec = 4;
  t.tv_usec = 0;
      
  while(1)
  {
    /*Check status on erery sockets */
    FD_ZERO(&readfds);
    FD_SET(client_fd, &readfds);

    select(1, &readfds, NULL, NULL, &t);

    /*recv wlan data using remote server fd*/
    if (FD_ISSET( client_fd, &readfds )) 
    {
      len = recv(client_fd, buf, BUF_LEN, 0); 
      require_action_quiet(len>0, exit, err = kConnectionErr);
      tcp_server_log("[tcprec][%d] = %.*s", len, len, buf);
      sendto(client_fd, buf, len, 0, &addr, sizeof(struct sockaddr_t));
    }
  }
  
exit:
  tcp_server_log("Exit: Client exit with err = %d, fd:%d", err, client_fd);
  SocketClose(&client_fd);
  if(buf) free(buf);
  mico_rtos_delete_thread(NULL);
}

