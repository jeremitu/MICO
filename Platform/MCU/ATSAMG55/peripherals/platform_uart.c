/**
******************************************************************************
* @file    MicoDriverUart.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide UART driver functions.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2014 MXCHIP Inc.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy 
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights 
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is furnished
*  to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
*/ 


#include "MICORTOS.h"
#include "MICOPlatform.h"

#include "platform.h"
#include "platform_peripheral.h"
#include "platformLogging.h"

/******************************************************
*                    Constants
******************************************************/

#define DMA_INTERRUPT_FLAGS  ( DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_FE )

/******************************************************
*                   Enumerations
******************************************************/

/******************************************************
*                 Type Definitions
******************************************************/

/******************************************************
*                    Structures
******************************************************/

/******************************************************
*               Variables Definitions
******************************************************/

/******************************************************
*        Static Function Declarations
******************************************************/
static OSStatus receive_bytes       ( platform_uart_driver_t* driver, void* data, uint32_t size, uint32_t timeout );

/* Interrupt service functions - called from interrupt vector table */
#ifndef NO_MICO_RTOS
static void thread_wakeup(void *arg);
static void RX_PIN_WAKEUP_handler(void *arg);
#endif

static pdc_packet_t pdc_uart_rx_packet;


/******************************************************
*               Function Definitions
******************************************************/

OSStatus platform_uart_init( platform_uart_driver_t* driver, const platform_uart_t* peripheral, const platform_uart_config_t* config, ring_buffer_t* optional_ring_buffer )
{
  pdc_packet_t      pdc_uart_packet;
  OSStatus          err = kNoErr;
  sam_usart_opt_t   usart_settings;
  bool              hardware_shaking = false;

  platform_mcu_powersave_disable();

  require_action_quiet( ( driver != NULL ) && ( peripheral != NULL ) && ( config != NULL ), exit, err = kParamErr);
  require_action_quiet( (optional_ring_buffer == NULL) || ((optional_ring_buffer->buffer != NULL ) && (optional_ring_buffer->size != 0)), exit, err = kParamErr);
    
  driver->rx_size              = 0;
  driver->tx_size              = 0;
  driver->last_transmit_result = kNoErr;
  driver->last_receive_result  = kNoErr;
  driver->peripheral           = (platform_uart_t*)peripheral;
#ifndef NO_MICO_RTOS
  mico_rtos_init_semaphore( &driver->tx_complete, 1 );
  mico_rtos_init_semaphore( &driver->rx_complete, 1 );
  mico_rtos_init_semaphore( &driver->sem_wakeup,  1 );
  mico_rtos_init_mutex    ( &driver->tx_mutex );
#else
  driver->tx_complete = false;
  driver->rx_complete = false;
#endif
    
  usart_disable_interrupt( peripheral->usart, 0xffffffff);
  
  /* Configure TX and RX pin_mapping */
  ioport_set_port_peripheral_mode( peripheral->gpio_bank, peripheral->pin_tx | peripheral->pin_rx, peripheral->mux_mode);

  if ( ( peripheral->pin_cts != NULL ) && ( config->flow_control == FLOW_CONTROL_CTS || config->flow_control == FLOW_CONTROL_CTS_RTS ) )
  {
    ioport_set_port_peripheral_mode( peripheral->gpio_bank, peripheral->pin_cts, peripheral->mux_mode );
  }

  if ( ( peripheral->pin_rts != NULL ) && ( config->flow_control == FLOW_CONTROL_RTS || config->flow_control == FLOW_CONTROL_CTS_RTS ) )
  {
    ioport_set_port_peripheral_mode( peripheral->gpio_bank, peripheral->pin_rts, peripheral->mux_mode );
  }
  
#ifndef NO_MICO_RTOS
  if(config->flags & UART_WAKEUP_ENABLE){
    mico_rtos_init_semaphore( driver->sem_wakeup, 1 );
    mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART_WAKEUP", thread_wakeup, 0x100, driver);
  }
#endif
  
    // peripheral_clock 
#if (SAMG55)
    flexcom_enable( peripheral->flexcom_base );
    flexcom_set_opmode( peripheral->flexcom_base, FLEXCOM_USART );
#else
	sysclk_enable_peripheral_clock(uart_mapping[uart].id_peripheral_clock);
#endif
    // usart init
	usart_settings.baudrate = config->baud_rate;
    switch ( config->parity ) {
        case NO_PARITY:
	        usart_settings.parity_type = US_MR_PAR_NO;
            break;
        case EVEN_PARITY:
	        usart_settings.parity_type = US_MR_PAR_EVEN;
            break;
        case ODD_PARITY:
	        usart_settings.parity_type = US_MR_PAR_ODD;
            break;
        default:
            return kGeneralErr;
    }
    switch ( config->data_width) {
        case DATA_WIDTH_5BIT: 
                              usart_settings.char_length = US_MR_CHRL_5_BIT;
                              break;
        case DATA_WIDTH_6BIT: 
                              usart_settings.char_length = US_MR_CHRL_6_BIT;
                              break;
        case DATA_WIDTH_7BIT: 
                              usart_settings.char_length = US_MR_CHRL_7_BIT;
                              break;
        case DATA_WIDTH_8BIT: 
                              usart_settings.char_length = US_MR_CHRL_8_BIT;
                              break;
        case DATA_WIDTH_9BIT: 
                              usart_settings.char_length = US_MR_MODE9;
                              break;
        default:
            return kGeneralErr;
    }
	usart_settings.stop_bits = ( config->stop_bits == STOP_BITS_1 ) ? US_MR_NBSTOP_1_BIT : US_MR_NBSTOP_2_BIT;
	usart_settings.channel_mode= US_MR_CHMODE_NORMAL;
		
    if (!hardware_shaking) {
        usart_init_rs232( peripheral->usart, &usart_settings, sysclk_get_peripheral_hz());
    } else {
        usart_init_hw_handshaking( peripheral->usart, &usart_settings, sysclk_get_peripheral_hz());
    }
  
  /**************************************************************************
  * Initialise STM32 DMA registers
  * Note: If DMA is used, USART interrupt isn't enabled.
  **************************************************************************/
   //usart_disable_interrupt(uart_mapping[uart].usart, 0xffffffff);
	pdc_disable_transfer( peripheral->dma_base, PERIPH_PTCR_TXTDIS | PERIPH_PTCR_RXTDIS);
#ifdef USE_USART_PDC
    pdc_uart_packet.ul_addr = NULL;
    pdc_uart_packet.ul_size = 1;
    pdc_tx_init(uart_mapping[uart].dma_base, &pdc_uart_packet, &pdc_uart_packet); //clear TX & RX
    pdc_rx_init(uart_mapping[uart].dma_base, &pdc_uart_packet, &pdc_uart_packet);

    usart_enable_interrupt(uart_mapping[uart].usart,  US_IER_TXBUFE | US_IER_RXBUFF); 
	/* Enable the RX and TX PDC transfer requests */
	pdc_enable_transfer(uart_mapping[uart].dma_base, PERIPH_PTCR_TXTEN | PERIPH_PTCR_RXTEN);
#endif    
    // nvic init
	NVIC_DisableIRQ( peripheral->usart_irq);
    //irq_register_handler(uart_mapping[uart].usart_irq, 0x0C);
	NVIC_ClearPendingIRQ( peripheral->usart_irq);
	NVIC_SetPriority( peripheral->usart_irq, 0x06);
	NVIC_EnableIRQ( peripheral->usart_irq);
    
    /* Enable the receiver and transmitter. */
    usart_enable_tx( peripheral->usart);
    usart_enable_rx( peripheral->usart);
    
    // dma init
#if 1
    /* setup ring buffer */
    if (optional_ring_buffer != NULL)
    {
        /* note that the ring_buffer should've been initialised first */
        driver->rx_buffer = optional_ring_buffer;
        driver->rx_size   = 0;
        //receive_bytes( driver, optional_ring_buffer->buffer, optional_ring_buffer->size, 0 );
    }
    else
    { //init rx dma nvic 
#ifdef USE_USART_PDC
        pdc_packet_t pdc_uart_packet;
	    
        pdc_uart_packet.ul_addr = (uint32_t) optional_rx_buffer->buffer;
	    pdc_uart_packet.ul_size = optional_rx_buffer->size;
	    pdc_rx_init(uart_mapping[uart].dma_base, &pdc_uart_packet, NULL);
#endif
    }
#endif
    //Enable_global_interrupt();
    //if (!Is_global_interrupt_enabled())
    //    Enable_global_interrupt();

exit:
  MicoMcuPowerSaveConfig(true);
  return err;
}

OSStatus platform_uart_deinit( platform_uart_driver_t* driver )
{
  uint8_t          uart_number;
  OSStatus          err = kNoErr;

  platform_mcu_powersave_disable();
  require_action_quiet( ( driver != NULL ), exit, err = kParamErr);

  usart_disable_tx( driver->peripheral->usart );
  usart_disable_rx( driver->peripheral->usart );

  NVIC_DisableIRQ( driver->peripheral->usart_irq );
#ifndef UART_NO_OS
  mico_rtos_deinit_semaphore(&driver->rx_complete);
  mico_rtos_deinit_semaphore(&driver->tx_complete);
#endif
    
exit:
    platform_mcu_powersave_enable();
    return err;
}

OSStatus platform_uart_transmit_bytes( platform_uart_driver_t* driver, const uint8_t* data_out, uint32_t size )
{
  OSStatus      err = kNoErr;
  pdc_packet_t  pdc_uart_packet;

  platform_mcu_powersave_disable();
  
#ifndef NO_MICO_RTOS
  mico_rtos_lock_mutex( &driver->tx_mutex );
#endif

  require_action_quiet( ( driver != NULL ) && ( data_out != NULL ) && ( size != 0 ), exit, err = kParamErr);
  
  /* reset DMA transmission result. the result is assigned in interrupt handler */
  driver->last_transmit_result                    = kGeneralErr;
  driver->tx_size                                 = size;

  pdc_uart_packet.ul_addr = (uint32_t) data_out;
  pdc_uart_packet.ul_size = size;
  pdc_tx_init( driver->peripheral->dma_base, &pdc_uart_packet, NULL);
    
  /* Enable the RX and TX PDC transfer requests */
  pdc_enable_transfer( driver->peripheral->dma_base, PERIPH_PTCR_TXTEN );
  
#ifndef UART_NO_OS
   mico_rtos_get_semaphore( &driver->tx_complete, MICO_NEVER_TIMEOUT );
#else 
   while( driver->last_transmit_result == false);
   driver->last_transmit_result = false;
#endif
  
  pdc_disable_transfer( driver->peripheral->dma_base, PERIPH_PTCR_TXTDIS );

  driver->tx_size = 0;
  err = driver->last_transmit_result;
  
exit:  
#ifndef NO_MICO_RTOS  
  mico_rtos_unlock_mutex( &driver->tx_mutex );
#endif
  platform_mcu_powersave_enable();
  return err;
}

OSStatus platform_uart_receive_bytes( platform_uart_driver_t* driver, uint8_t* data_in, uint32_t expected_data_size, uint32_t timeout_ms )
{
  OSStatus err = kNoErr;

  msleep(timeout_ms);
exit:
  //platform_mcu_powersave_enable();
  return kTimeoutErr;
}

static OSStatus receive_bytes( platform_uart_driver_t* driver, void* data, uint32_t size, uint32_t timeout )
{
  OSStatus err = kNoErr;

exit:
  return err;
}

OSStatus platform_uart_get_length_in_buffer( platform_uart_driver_t* driver )
{  
  return ring_buffer_used_space( driver->rx_buffer );
}


#ifndef NO_MICO_RTOS
static void thread_wakeup(void *arg)
{
}
#endif

/******************************************************
*            Interrupt Service Routines
******************************************************/

void platform_uart_irq( platform_uart_driver_t* driver )
{
    uint32_t status = 0;
    uint32_t g_pdc_count = 0;
    Usart *p_usart = driver->peripheral->usart;
#ifdef USE_USART_PDC
	pdc_packet_t pdc_uart_packet;
#endif
    // clear all inte
    status = usart_get_status(p_usart);
	//usart_disable_interrupt(p_usart, 0xffffffff);
	//pdc_disable_transfer(uart_mapping[uart].dma_base, PERIPH_PTCR_RXTDIS | PERIPH_PTCR_TXTDIS);
	/* Get USART status and check if CMP is set */
	if (status &  US_CSR_TXBUFE) { //pdc int
#ifdef USE_USART_PDC
        //clear CSR by tx_init
        pdc_uart_packet.ul_addr = NULL;
        pdc_uart_packet.ul_size = 6;
        pdc_tx_init(uart_mapping[uart].dma_base, &pdc_uart_packet, &pdc_uart_packet);
#endif
		/* Disable USART IRQ */
		//usart_disable_interrupt(p_usart, US_IDR_TXBUFE);
#if ENABLE_TX_SEMA
        #ifndef UART_NO_OS
        /* set semaphore regardless of result to prevent waiting thread from locking up */
        mico_rtos_set_semaphore( &uart_interfaces[uart].tx_complete);
        #else
        uart_interfaces[uart].tx_complete = true;
        #endif
#endif
	}
    //if (status & (US_CSR_RXRDY)) { //RXRDY for one character is ready.
        //usart_disable_interrupt(p_usart, US_IDR_RXRDY);
        //usart_restart_rx_timeout(uart_mapping[uart].usart);
    //}
    // update tail 
    if (status & (US_CSR_TIMEOUT | US_CSR_RXBUFF)) {
    //if (status & (US_CSR_RXRDY| US_CSR_RXBUFF)) {
        usart_disable_interrupt(p_usart, US_IDR_RXBUFF | US_IDR_TIMEOUT);

        if (status & US_CSR_RXBUFF) {
            pdc_rx_init( driver->peripheral->dma_base, &pdc_uart_rx_packet, NULL);
            g_pdc_count = 0;
        } else {
            g_pdc_count = pdc_read_rx_counter( driver->peripheral->dma_base);
        }
        
        driver->rx_buffer->tail = driver->rx_buffer->size - (g_pdc_count);
        

        // notify thread if sufficient data are available
        if ( ( driver->rx_size > 0 ) &&
                ( ring_buffer_used_space( driver->rx_buffer ) >= driver->rx_size ) )
        {
#ifndef UART_NO_OS
            mico_rtos_set_semaphore( &driver->rx_complete );
#else
            driver->rx_complete = true;
#endif
            driver->rx_size = 0;
        }
#if 0 
#ifndef UART_NO_OS
        if(uart_interfaces[ uart ].sem_wakeup)
            mico_rtos_set_semaphore(&uart_interfaces[ uart ].sem_wakeup);
#endif
#endif
        //g_pdc_count = 0;
    }
}
