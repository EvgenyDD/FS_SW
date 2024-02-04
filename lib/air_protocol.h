#ifndef TRX_H
#define TRX_H

#include <stdbool.h>
#include <stdint.h>

#define RFM_NET_GATEWAY 0x6A

#define RFM_NET_ID_BROADCAST 0
#define RFM_NET_ID_CTRL 50
#define RFM_NET_ID_HEAD 51
#define RFM_NET_ID_TAIL 52

#define RFM_NET_KEY "HYHpkCeP9zJw4Uxs"
#define RFM_NET_KEY_LENGTH 16

enum RFM_NET_PROTOCOL
{
    RFM_NET_CMD_NOP = 0, /// NOP cmd
    RFM_NET_CMD_SLS, /// Slave listen enable
    RFM_NET_CMD_SLE, /// Slave listen disable

    RFM_NET_CMD_STS_HEAD = 10,
    RFM_NET_CMD_LIGHT,
    RFM_NET_CMD_FAN,
    RFM_NET_CMD_SERVO_RAW,
    RFM_NET_CMD_SERVO_SMOOTH,

    RFM_NET_CMD_DEBUG = 40,

    RFM_NET_CMD_FLASH = 50,
    RFM_NET_CMD_FLASH_STOP,
    RFM_NET_CMD_REBOOT,
    RFM_NET_CMD_OFF,
};

void air_protocol_init(void);
void air_protocol_init_node(uint8_t iterator, uint8_t node_id);

void air_protocol_poll(void);

void air_protocol_send_async(uint8_t iterator, const uint8_t *payload, uint8_t payload_length);

#ifdef AIR_PROTOCOL_MASTER
void air_protocol_set_periodic_sleep(bool state);
#endif

#endif // TRX_H
