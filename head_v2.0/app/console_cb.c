#include "adc.h"
#include "console.h"
#include "debug.h"
#include "fw_header.h"
#include "led.h"
#include "platform.h"
#include "servo.h"
#include <stdio.h>

static void info_cb(const char *req, int len, int *ret)
{
	for(uint32_t i = 0; i < FW_COUNT; i++)
	{
		_console_print("====== FW %s (%d) =====\n", ((const char *[]){"PREBOOT", "BOOT", "APP"})[i], g_fw_info[i].locked);
		if(g_fw_info[i].locked) break;
		_console_print("\tAddr: x%x | Size: %ld\n", g_fw_info[i].addr, g_fw_info[i].size);
		_console_print("\tVer: %ld.%ld.%ld\n", g_fw_info[i].ver_major, g_fw_info[i].ver_minor, g_fw_info[i].ver_patch);
		if(g_fw_info[i].field_product_ptr) _console_print("\tProd: %s\n", g_fw_info[i].field_product_ptr);
		if(g_fw_info[i].field_product_name_ptr) _console_print("\tName: %s\n", g_fw_info[i].field_product_name_ptr);
		const char *s = fw_fields_find_by_key_helper(&g_fw_info[i], "build_ts");
		if(s) _console_print("\tBuild: %s\n", s);
	}
	servo_print();
	debug("Vbat: %.2f\n", adc_val.v_bat);
	debug("Temp: %.2f %.2f\n", adc_val.t_ntc[0], adc_val.t_ntc[1]);
}

static void servo0_cb(const char *req, int len, int *ret)
{
	unsigned int servo = 0, val = 0;

	int c = sscanf(req, "%u %u", &servo, &val);
	if(c == 2)
	{
		debug_rf("Set Servo %d to %d\n", servo, val);
		servo_set(servo, val);
	}
	else
	{
		debug_rf("servo: fail Input\n");
	}
}

static void servo1_cb(const char *req, int len, int *ret)
{
	unsigned int servo = 0, val = 0, delay_ms = 0;

	int c = sscanf(req, "%u %u %u", &servo, &val, &delay_ms);
	if(c == 3)
	{
		debug_rf("Set Servo Smth %d to %d %d ms\n", servo, val, delay_ms);
		servo_set_smooth_and_off(servo, val, delay_ms);
	}
	else
	{
		debug_rf("Servos: fail Input\n");
	}
}

static void servo2_cb(const char *req, int len, int *ret)
{
	unsigned int servo[2], val[2], delay_ms = 0;

	int c = sscanf(req, "%u %u %u %u %u", &servo[0], &val[0], &servo[1], &val[1], &delay_ms);
	if(c == 5)
	{
		debug_rf("Smth2 %d to %d | %d to %d\t%d ms\n", servo[0], val[0], servo[1], val[1], delay_ms);
		servo_set_smooth_and_off(servo[0], val[0], delay_ms);
		servo_set_smooth_and_off(servo[1], val[1], delay_ms);
	}
	else
	{
		debug_rf("Servos2: fail Input %d\n", c);
	}
}

static void servo3_cb(const char *req, int len, int *ret)
{
	/*
		 top
		 sssservo 0 500 1 560 4 420 5 330 800
		 bottom
		 sssservo 0 340 1 380 4 600 5 500 800
		 АГАААА
		 sssservo 0 500 1 420 4 550 5 330 800
		 **
		 sssservo 0 500 1 330 4 620 5 330 800
		 *
		 sssservo 0 440 1 330 4 640 5 400 800
		 */
	unsigned int servo[4], val[4], delay_ms = 0;

	int c = sscanf(req, "%u %u %u %u %u %u %u %u %u", &servo[0], &val[0], &servo[1], &val[1], &servo[2], &val[2], &servo[3], &val[3], &delay_ms);
	if(c == 9)
	{
		debug_rf("Smth2 %d to %d | %d to %d | %d to %d | %d to %d\t%d ms\n", servo[0], val[0], servo[1], val[1], servo[2], val[2], servo[3], val[3], delay_ms);
		servo_set_smooth_and_off(servo[0], val[0], delay_ms);
		servo_set_smooth_and_off(servo[1], val[1], delay_ms);
		servo_set_smooth_and_off(servo[2], val[2], delay_ms);
		servo_set_smooth_and_off(servo[3], val[3], delay_ms);
	}
	else
	{
		debug_rf("Servos4: fail Input %d\n", c);
	}
}

static void fan_cb(const char *req, int len, int *ret)
{
	unsigned int fan = 0, val = 0;

	int c = sscanf(req, "%u %u", &fan, &val);
	if(c == 2)
	{
		val = val % 100;
		debug_rf("Set Fan %d to %d\n", fan, val);
		fan_set(fan, val);
	}
	else
	{
		debug_rf("fan: fail Input\n");
	}
}

static void led_cb(const char *req, int len, int *ret)
{
	unsigned int led = 0, val = 0;

	int c = sscanf(req, "%u %u", &led, &val);
	if(c == 2)
	{
		val = val % 100;
		debug_rf("Set LED %d to %d\n", led, val);
		led_set(led, val);
	}
	else
	{
		debug_rf("LED: fail Input\n");
	}
}

const console_cmd_t console_cmd[] = {
	{"info", info_cb},
	{"s0", servo0_cb},
	{"s1", servo1_cb},
	{"s2", servo2_cb},
	{"s3", servo3_cb},
	{"led", fan_cb},
	{"led", led_cb},
};
const uint32_t console_cmd_sz = sizeof(console_cmd) / sizeof(console_cmd[0]);