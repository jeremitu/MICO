/*
 *  Copyright (C)2008 -2014, Marvell International Ltd.
 *  All Rights Reserved.
 */

/**
 *  @brief This is the WiFi Calibration application
 */
#include <stdarg.h>
#include "MICOPlatform.h"
//#include "wlan.h"
     
#include "MICO.h"
#include "platform.h"
#include "platform_common_config.h"

#include "uart_wifi_bridge.h"
typedef unsigned short t_u16;
/** Interface header length */
#define INTF_HEADER_LEN     4
#define SDIOPKTTYPE_CMD     0x1
#define UART_BUFFER_LENGTH 1024

void *dev;
void read_wlan_resp();

#define UART_INDEX MICO_UART_1

uint8_t *local_outbuf;

static SDIOPkt *sdiopkt;
static uint8_t *rx_buf;
static int need_upgrade = 0;

static char msg_buf[200];


int wmprintf(const char *format, ...)
{
#if 0
	va_list args;
	uint32_t ret;
    
	va_start(args, format);
	vsnprintf(msg_buf, sizeof(msg_buf), &format[0], args);
	va_end(args);
	printf(msg_buf);
	return ret;
#endif
    return 0;
}

void mico_rtos_stack_overflow(char *taskname)
{
    printf("Stack Overflow Detected in task %s\r\n",taskname);
}
void connected_ap_info()
{
}
void dhcp_up(void)
{
}
int g_data_nf_last, g_data_snr_last;
void ipaddr_ntoa(void)
{
}
int is_mf_mode(void)
{
	return 0;
}
void join_fail(int type)
{
}
void net_configure_address(void)
{
}
void net_configure_dns(void)
{
}
void net_dhcp_hostname_set(void)
{
}
void net_get_if_addr(void)
{
}
void net_get_sta_handle(void)
{
}
void net_get_uap_handle(void)
{
}
void net_interface_dhcp_stop(void)
{
}
void net_interface_down(void)
{
}
void net_wlan_init(void)
{
}
void sniffer_deregister_callback(void)
{
}
void sniffer_register_callback(void)
{
}

/* Callback function called from the wifi SDIO module */
void handle_data_packet(t_u8 *rcvdata, t_u16 datalen)
{
	printf("null\r\n");
}
struct pbuf *gen_pbuf_from_data(t_u8 *payload, t_u16 datalen)
{
    return NULL;
}

void deliver_packet_above(struct pbuf *p, int recv_interface)
{
    return;
}

volatile ring_buffer_t  rx_buffer;
volatile uint8_t        rx_data[UART_BUFFER_LENGTH];


static uint32_t uart_drv_read(void *dev, uint8_t *buf, uint32_t num)
{
	uint32_t len = num;

    MicoUartRecv( UART_INDEX, buf, num, 0x7FFFFFFF );
	

	return len;
}

static int uart_drv_rx_buf_reset(void *dev)
{
    uint8_t junk;
    int len;
    do {
        len = MicoUartRecv( UART_INDEX, &junk, 1, 1 );
    } while (len == 0);
    
    return 0;
}

uint32_t uart_drv_write(void *dev, const uint8_t *buf, uint32_t num)
{
    MicoUartSend( UART_INDEX, buf, num );
    return num;
}   

/**
 * All application specific initialization is performed here
 */

int application_start()
{
    mico_uart_config_t uart_config;
    
	local_outbuf = malloc(SDIO_OUTBUF_LEN);
	if (local_outbuf == NULL) {
		wmprintf("Failed to allocate buffer\r\n");
		return -1;
	}
	sdiopkt = (SDIOPkt *) local_outbuf;

// 
//    /*UART receive thread*/
//    uart_config.baud_rate    = 115200;
//    uart_config.data_width   = DATA_WIDTH_8BIT;
//    uart_config.parity       = NO_PARITY;
//    uart_config.stop_bits    = STOP_BITS_1;
//    uart_config.flow_control = FLOW_CONTROL_DISABLED;
//    uart_config.flags = UART_WAKEUP_DISABLE;
//    ring_buffer_init  ( (ring_buffer_t *)&rx_buffer, (uint8_t *)rx_data, UART_BUFFER_LENGTH );
//    MicoUartInitialize( UART_INDEX, &uart_config, (ring_buffer_t *)&rx_buffer );
  
	rx_buf = malloc(BUF_LEN);
	if (rx_buf == NULL)
		wmprintf("failed to allocate memory\r\n");
	int ret = sd_wifi_init(1, NULL);
	task_init();
	return ret;
}

static uart_cb uartcb;
static cmd_header last_cmd_hdr;	/* holds the last cmd_hdr */

static void uart_init_crc32(uart_cb *uartcb)
{
	int i, j;
	unsigned int c;
	for (i = 0; i < 256; ++i) {
		for (c = i << 24, j = 8; j > 0; --j)
			c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);
		uartcb->crc32_table[i] = c;
	}
}

static uint32_t uart_get_crc32(uart_cb *uart, int len, unsigned char *buf)
{
	unsigned int *crc32_table = uart->crc32_table;
	unsigned char *p;
	unsigned int crc;
	crc = 0xffffffff;
	for (p = buf; len > 0; ++p, --len)
		crc = (crc << 8) ^ (crc32_table[(crc >> 24) ^ *p]);
	return ~crc;
}

/*
	send_response_to_uart() handles the response from the firmware.
	This involves
	1. replacing the sdio header with the uart header
	2. computation of the crc of the payload
	3. sending it out to the uart
*/

static int send_to_uart(uint8_t *resp)
{
	uint32_t bridge_chksum = 0;
	uint32_t msglen;
	int index;
	uart_header *uart_hdr;
    cmd_header header;
    int len = 256; // MXCHIP always set as 256
    uart_cb *uart = &uartcb;
    
    memset(rx_buf, 0, BUF_LEN);
    memset(&header, 0, sizeof(cmd_header));
    
    uart_hdr = (uart_header *) rx_buf;
    
    header.type = 0x03;
    header.length = len + sizeof(cmd_header);
    
	uart_hdr->length = len + sizeof(cmd_header);
	uart_hdr->pattern = 0x5555;
	
	memcpy(rx_buf + sizeof(uart_header) + sizeof(cmd_header),
	       resp, len);
	memcpy(rx_buf + sizeof(uart_header), (uint8_t *) &header,
	       sizeof(cmd_header));

	/* calculate CRC. The uart_header is excluded */
	msglen = len + sizeof(cmd_header);
	bridge_chksum = uart_get_crc32(uart, msglen, rx_buf +
				       sizeof(uart_header));
	index = sizeof(uart_header) + msglen;

	rx_buf[index] = bridge_chksum & 0xff;
	rx_buf[index + 1] = (bridge_chksum & 0xff00) >> 8;
	rx_buf[index + 2] = (bridge_chksum & 0xff0000) >> 16;
	rx_buf[index + 3] = (bridge_chksum & 0xff000000) >> 24;

	/* write response to uart */
	uart_drv_write(dev, rx_buf, index + 4);
	memset(rx_buf, 0, BUF_LEN);
	wmprintf("Command response sent\r\n");

	return 0;
}

/*
	send_response_to_uart() handles the response from the firmware.
	This involves
	1. replacing the sdio header with the uart header
	2. computation of the crc of the payload
	3. sending it out to the uart
*/

static int send_response_to_uart(uart_cb *uart, uint8_t *resp)
{
	uint32_t bridge_chksum = 0;
	uint32_t msglen;
	int index;
	uint32_t payloadlen;
	uart_header *uart_hdr;
	SDIOPkt *sdio = (SDIOPkt *) resp;
	payloadlen = sdio->size - INTF_HEADER_LEN;
	memset(rx_buf, 0, BUF_LEN);
	memcpy(rx_buf + sizeof(uart_header) + sizeof(cmd_header),
	       resp + INTF_HEADER_LEN, payloadlen);
	memcpy(rx_buf + sizeof(uart_header), (uint8_t *) &last_cmd_hdr,
	       sizeof(cmd_header));

	uart_hdr = (uart_header *) rx_buf;
	uart_hdr->length = payloadlen + sizeof(cmd_header);
	uart_hdr->pattern = 0x5555;

	/* calculate CRC. The uart_header is excluded */
	msglen = payloadlen + sizeof(cmd_header);
	bridge_chksum = uart_get_crc32(uart, msglen, rx_buf +
				       sizeof(uart_header));
	index = sizeof(uart_header) + msglen;

	rx_buf[index] = bridge_chksum & 0xff;
	rx_buf[index + 1] = (bridge_chksum & 0xff00) >> 8;
	rx_buf[index + 2] = (bridge_chksum & 0xff0000) >> 16;
	rx_buf[index + 3] = (bridge_chksum & 0xff000000) >> 24;

	/* write response to uart */
	uart_drv_write(dev, rx_buf, payloadlen + sizeof(cmd_header)
		       + sizeof(uart_header) + 4);
	memset(rx_buf, 0, BUF_LEN);
	wmprintf("Command response sent\r\n");

	return 0;
}

/*
	check_command_complete() validates the command from the uart.
	It checks for the signature in the header and the crc of the
	payload. This assumes that the uart_buf is circular and data
	can be wrapped.
*/

int check_command_complete(uint8_t *buf)
{
	uart_header *uarthdr;
	uint32_t msglen, endofmsgoffset;
	uart_cb *uart = &uartcb;
	int checksum = 0, bridge_checksum = 0;

	uarthdr = (uart_header *) buf;

	/* out of sync */
	if (uarthdr->pattern != 0x5555) {
		wmprintf("Pattern mismatch\r\n");
		return -1;
	}
	/* check crc */
	msglen = uarthdr->length;

	/* add 4 for checksum */
	endofmsgoffset = sizeof(uart_header) + msglen + 4;

	memset((uint8_t *) local_outbuf, 0, sizeof(local_outbuf));
	if (endofmsgoffset < UART_BUF_SIZE) {
		memcpy((uint8_t *) local_outbuf, buf, endofmsgoffset);
	} else {
		memcpy((uint8_t *) local_outbuf, buf, UART_BUF_SIZE);
		/* To do : check if copying method is correct */
		memcpy((uint8_t *) local_outbuf + UART_BUF_SIZE,
		       buf, endofmsgoffset);
	}

	checksum = *(int *)((uint8_t *) local_outbuf + sizeof(uart_header) +
			    msglen);

	bridge_checksum = uart_get_crc32(uart, msglen,
					 (uint8_t *) local_outbuf +
					 sizeof(uart_header));
	if (checksum == bridge_checksum) {
		return 0;
	}
	/* Reset local outbuf */
	memset(local_outbuf, 0, sizeof(local_outbuf));
	wmprintf("command checksum error\r\n");
	return -1;
}

/*
	send_cmd_to_wlan() sends command to the wlan
	card
*/
int wifi_raw_packet_send(const uint8_t *packet, uint32_t length);
int send_cmd_to_wlan(uint8_t *buf, int m_len)
{
	uart_header *uarthdr;
	int i;
	uint8_t *s, *d;

	memset(local_outbuf, 0, BUF_LEN);

	uarthdr = (uart_header *) buf;

	/* sdiopkt = local_outbuf */
	sdiopkt->pkttype = SDIOPKTTYPE_CMD;

	sdiopkt->size = m_len - sizeof(cmd_header) + INTF_HEADER_LEN;
	d = (uint8_t *) local_outbuf + INTF_HEADER_LEN;
	s = (uint8_t *) buf + sizeof(uart_header) + sizeof(cmd_header);

	for (i = 0; i < uarthdr->length - sizeof(cmd_header); i++) {
		if (s < buf + UART_BUF_SIZE)
			*d++ = *s++;
		else {
			s = buf;
			*d++ = *s++;
		}
	}

	d = (uint8_t *) &last_cmd_hdr;
	s = (uint8_t *) buf + sizeof(uart_header);

	for (i = 0; i < sizeof(cmd_header); i++) {
		if (s < buf + UART_BUF_SIZE)
			*d++ = *s++;
		else {
			s = buf;
			*d++ = *s++;
		}
	}
	int stat = wifi_raw_packet_send(local_outbuf, BUF_LEN);
	if (stat == 0)
		return 0;
	else {
		wmprintf("Error while sending packet to Wifi\r\n");
		return -1;
	}
}

void mxchiP_need_upgrade(void)
{
    need_upgrade = 1;
}

void mxchip_do_cmd(char *cmd)
{
    char resp[256];

    memset(resp, 0, sizeof(resp));
    if (strncasecmp(cmd, "echo", 4) == 0) {
        send_to_uart(cmd);
    } else if (strcmp(cmd, "gpio") == 0) {
        sprintf(resp, mxchip_gpio_test());
        send_to_uart(resp);
    } else if (strcmp(cmd, "pwr") == 0) {
        sprintf(resp, mxchip_pwr_test());
        send_to_uart(resp);
    } else if (strcmp(cmd, "upgrade") == 0) {
        sprintf(resp, mxchip_upgrade_test());
        send_to_uart(resp);
        if (need_upgrade == 1) {
            mxchip_upgrade();
            // if come here, means upgrade failed.
            need_upgrade = -1;
        }
    } else {
        sprintf(resp, "Unknown %s", cmd);
        send_to_uart(resp);
    }
    
}

/*
	uart_rx_cmd() runs in a loop. It polls the uart ring buffer
	checks it for a complete command and sends the command to the
	wlan card
*/

void uart_rx_cmd(void* arg)
{
	uart_cb *uart = &uartcb;
	uart_init_crc32(uart);
	int uart_rx_len = 0;
	int len = 0;
	int msg_len = 0;
    cmd_header *header;
    
	while (1) {
		len = 0;
		msg_len = 0;
		uart_rx_len = 0;
		memset(uart->uart_buf, 0, sizeof(uart->uart_buf));
		while (len != LABTOOL_PATTERN_HDR_LEN) {
			uart_rx_len = uart_drv_read(dev, uart->uart_buf
						    + len,
						    LABTOOL_PATTERN_HDR_LEN -
						    len);
			len += uart_rx_len;
		}
		/* Length of the packet is indicated by byte[2] & byte[3] of
		the packet excluding header[4 bytes] + checksum [4 bytes]
		*/
		msg_len = (uart->uart_buf[3] << 8) + uart->uart_buf[2];
		len = 0;
		uart_rx_len = 0;
		while (len != msg_len + LABTOOL_PATTERN_HDR_LEN) {
			/* To do: instead of reading 1 byte,
			read whole chunk of data */
			uart_rx_len = uart_drv_read(dev, uart->uart_buf +
						    LABTOOL_PATTERN_HDR_LEN +
						    len, 1);
			len++;
		}

		//dump_hex(uart->uart_buf, msg_len +
		//	 LABTOOL_PATTERN_HDR_LEN + CHECKSUM_LEN);

		/* validate the command including checksum */
		if (check_command_complete(uart->uart_buf) == 0) {
            header = (cmd_header *)(uart->uart_buf + sizeof(uart_header));
            //printf("header: type %d\r\n", header->type);
            if (header->type == 3) {
                mxchip_do_cmd(uart->uart_buf+sizeof(uart_header)+sizeof(cmd_header));
            } else {
			/* send fw cmd over SDIO after
			   stripping off uart header */
			send_cmd_to_wlan(uart->uart_buf, msg_len + 8);
			memset(uart->uart_buf, 0, sizeof(uart->uart_buf));
			read_wlan_resp();
            }
		} else
			uart_drv_rx_buf_reset(dev);
	}
	mico_rtos_delete_thread(NULL);
}

/*
	read_wlan_resp() handles the responses from the wlan card.
	It waits on wlan card interrupts on account
	of command responses are handled here. The response is
	read and then sent through the uart to the Mfg application
*/
int wifi_raw_packet_recv(uint8_t **data, uint32_t *pkt_type);
void read_wlan_resp()
{
	uart_cb *uart = &uartcb;
	uint8_t *packet;
	uint32_t pkt_type;
	int rv = wifi_raw_packet_recv(&packet, &pkt_type);
	if (rv != 0)
		wmprintf("Receive response failed\r\n");
	else {
		if (pkt_type == MLAN_TYPE_CMD)
			send_response_to_uart(uart, packet);
	}
}

/* Initialize the tasks for uart-wifi-bridge app */
int task_init()
{
	int ret = 0;
    ret = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "wifi_bridge_uart_thread", uart_rx_cmd, (5*1024), 0);
	if (ret)
		wmprintf("Failed to create wifi_bridge_uart_thread\n\r");

	return 0;
}
