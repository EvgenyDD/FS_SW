#include "adc.h"
#include "debug.h"
#include "led.h"
#include "platform.h"
#include "servo.h"
#include <stdio.h>

static int strcmp_(char *s1, char *s2)
{
	for(int cnt = 0;; cnt++)
	{
		if(*s2 == '\n' || *s2 == '\0') return cnt == 0 ? 0 : 1;
		if(*s1 != *s2) return 0;
		s1++;
		s2++;
	}
}

static int find_space(char *s)
{
	for(char *ss = s;; ss++)
	{
		if(*ss == '\n' || *ss == '\0') return 0;
		if(*ss == ' ') return ss - s + 1;
	}
}

void debug_parse(char *s)
{
	if(strcmp_(s, "info"))
	{
		// unsigned int state = 0;
		// int c = sscanf(&s[find_space(s)], "%u", &state);
		// if(c > 0)
		// {
		//     debug_rf("YEAH %d\n", state);
		// }
		servo_print();
		debug("Vbat: %.2f\n", adc_val.v_bat);
		debug("Temp: %.2f %.2f\n", adc_val.t_ntc[0]);
	}
	else if(strcmp_(s, "servo"))
	{
		unsigned int servo = 0, val = 0;

		int c = sscanf(&s[find_space(s)], "%u %u", &servo, &val);
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
	else if(strcmp_(s, "sservo"))
	{
		unsigned int servo = 0, val = 0, delay_ms = 0;

		int c = sscanf(&s[find_space(s)], "%u %u %u", &servo, &val, &delay_ms);
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
	else if(strcmp_(s, "ssservo"))
	{
		unsigned int servo[2], val[2], delay_ms = 0;

		int c = sscanf(&s[find_space(s)], "%u %u %u %u %u", &servo[0], &val[0], &servo[1], &val[1], &delay_ms);
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
	else if(strcmp_(s, "sssservo"))
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

		int c = sscanf(&s[find_space(s)], "%u %u %u %u %u %u %u %u %u", &servo[0], &val[0], &servo[1], &val[1], &servo[2], &val[2], &servo[3], &val[3], &delay_ms);
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
	else if(strcmp_(s, "fan"))
	{
		unsigned int fan = 0, val = 0;

		int c = sscanf(&s[find_space(s)], "%u %u", &fan, &val);
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
	else if(strcmp_(s, "led"))
	{
		unsigned int led = 0, val = 0;

		int c = sscanf(&s[find_space(s)], "%u %u", &led, &val);
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
	else
	{
		debug_rf("Not Found!\n");
	}
}