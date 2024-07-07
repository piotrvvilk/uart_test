#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "serial.h"

#define SLEEP_TIME_MS   1000

#define LED0_NODE 	DT_ALIAS(led0)
#define LED1_NODE 	DT_ALIAS(led1)
//#define	RX_PIN 		DT_ALIAS(urx)

static const struct gpio_dt_spec led_0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led_1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

#define     LED1_TOGGLE             gpio_pin_toggle_dt(&led_1)

LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_DBG);

K_THREAD_DEFINE(thread_serial_id, THREAD_SERIAL_STACKSIZE, thread_serial, NULL, NULL, NULL, THREAD_SERIAL_PRIORITY, 0, 0);

//================================================================================================
int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led_0)) 
	{
		return 0;
	}
	
	if (!gpio_is_ready_dt(&led_1)) 
	{
		return 0;
	}

	ret = gpio_pin_configure_dt(&led_0, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) 
	{
		return 0;
	}

	ret = gpio_pin_configure_dt(&led_1, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) 
	{
		return 0;
	}

	LED1_TOGGLE;

	LOG_INF("Hello world\n");

//================================================================================================
	while (1) 
	{
		ret = gpio_pin_toggle_dt(&led_0);

		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
//================================================================================================
