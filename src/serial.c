//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>
#include <soc.h>
#include <assert.h>

#include <zephyr/logging/log.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

//---------------------------------------------------------------------------
// Definitions 
//---------------------------------------------------------------------------
/* UART payload buffer element size. */
#define UART1_TX_BUF_SIZE				16
#define UART1_RX_BUF_SIZE				16
#define UART1_RX_TIMEOUT 				1000
#define UART1_TX_TIMEOUT_MS				100

LOG_MODULE_REGISTER(UART0, LOG_LEVEL_DBG);

//----------------------------------------------------------------------------------------------- service uart 
static const struct device *uart_service = DEVICE_DT_GET(DT_NODELABEL(uart1));

uint8_t uart1_tx_buffer[UART1_TX_BUF_SIZE];

uint8_t uart1_double_buffer[2][UART1_RX_BUF_SIZE];
uint8_t *uart1_buf_next = uart1_double_buffer[1];

uint8_t complete_message_1[UART1_RX_BUF_SIZE];
uint8_t complete_message_counter_1 = 0;

volatile bool currently_active_buffer_1 = 1; 				// 0 - uart_double_buffer[0] is active, 1 - uart_double_buffer[1] is active
volatile uint32_t uart1_ready_flag;

//---------------------------------------------------------------------------
// Implementation
//---------------------------------------------------------------------------
static void check_message(uint8_t *data)
{
	switch(data[0])
	{
		case 0xA3:
		case 0xA5:
		case 0xA8:
			uart1_tx_buffer[0]=0xAA;
			break;
		
		default:
		 	uart1_tx_buffer[0]=0xAF;
	}

	uart_tx(uart_service, uart1_tx_buffer, 1, SYS_FOREVER_MS);		
}

//=================================================================================================
static void uart1_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);
	uint8_t test;

	switch (evt->type) {
	case UART_TX_DONE:
		break;

	case UART_RX_RDY:
		if (currently_active_buffer_1 == 0)
		{
			for(int i = 0 + evt->data.rx.offset; i < UART1_RX_BUF_SIZE; i++)
			{
				complete_message_1[complete_message_counter_1] = uart1_double_buffer[0][i];
				complete_message_counter_1++;
				
				test =  uart1_double_buffer[0][i]&0xF0;
								
				if(test == 0xA0)
				{
					complete_message_counter_1 = 0;
					uart1_ready_flag=true;
					break;
				}
				else
				{
					complete_message_counter_1 = 0;
					memset(&complete_message_1, 0, sizeof(complete_message_1)); // clear out the buffer to prepare for next read.
				}
			}
		}

		if (currently_active_buffer_1 == 1)
		{
			for (int i = 0 + evt->data.rx.offset; i < UART1_RX_BUF_SIZE; i++)
			{
				complete_message_1[complete_message_counter_1] = uart1_double_buffer[1][i];
				complete_message_counter_1++;

				test =  uart1_double_buffer[0][i]&0xF0;
				
				if (test == 0x0A)
				{
					complete_message_counter_1 = 0;
					uart1_ready_flag=true;
					break;
				}
				else
				{
					complete_message_counter_1 = 0;
					memset(&complete_message_1, 0, sizeof(complete_message_1)); // clear out the buffer to prepare for next read.
				}
			}
		}
		break;

	case UART_RX_DISABLED:
		break;

	case UART_RX_BUF_REQUEST:
		uart_rx_buf_rsp(uart_service, uart1_buf_next, UART1_RX_BUF_SIZE);
		currently_active_buffer_1 = !currently_active_buffer_1;
		break;

	case UART_RX_BUF_RELEASED:
		uart1_buf_next = evt->data.rx_buf.buf;
		break;

	case UART_TX_ABORTED:
		break;

	default:
		break;
	}
}

//=================================================================================================
static int uart1_init(void)
{
	if (!device_is_ready(uart_service)) 
	{
		LOG_ERR("UART device not ready\n");
		return -ENODEV;
	}

	uart_callback_set(uart_service, uart1_cb, NULL);
	uart_rx_enable(uart_service, uart1_double_buffer[0], UART1_RX_BUF_SIZE, UART1_RX_TIMEOUT);
	LOG_INF("UART1 started\n");

	return 0;
}

//=================================================================================================
void thread_serial(void)
{
	uart1_init();
	
	while(1)
	{
		if(uart1_ready_flag)
		{
			uart1_ready_flag=0;
			check_message(complete_message_1);
			memset(&complete_message_1, 0, sizeof(complete_message_1)); 
		}

		k_sleep(K_MSEC(50));
	}
}

//=================================================================================================