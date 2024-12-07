
#ifndef AIR_PROTOCOL_H__
#define AIR_PROTOCOL_H__

#include "rfm75.h"
#include <stdint.h>

enum
{
	AIRPROTO_CMD_POLL = 0,
	AIRPROTO_CMD_OFF,
	AIRPROTO_CMD_REBOOT,
	AIRPROTO_CMD_DEBUG,
	AIRPROTO_CMD_STS,

	AIRPROTO_CMD_FLASH_STS = 0x10,
	AIRPROTO_CMD_FLASH_TYPE,
	AIRPROTO_CMD_FLASH_WRITE,
	AIRPROTO_CMD_FLASH_READ,

	AIRPROTO_CMD_LIGHT = 0x20,
	AIRPROTO_CMD_FAN,
	AIRPROTO_CMD_SERVO_RAW,
	AIRPROTO_CMD_SERVO_SMOOTH,
};

typedef struct __attribute__((packed))
{
	uint8_t hdr;
	uint16_t vbat_mv;
	int8_t t_mcu;
	int8_t t_ntc[2];
	uint16_t s_pos[7];
} airproto_head_sts_t; // size + 1 <= RFM75_MAX_PAYLOAD_SIZE

enum
{
	_check_size_airproto_head_sts_t = 1 / (sizeof(airproto_head_sts_t) <= RFM75_MAX_PAYLOAD_SIZE ? 1 : 0),
};

#endif // AIR_PROTOCOL_H__