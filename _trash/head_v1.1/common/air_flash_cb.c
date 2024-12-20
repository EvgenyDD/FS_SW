#include "config_system.h"
#include "fw_header.h"
#include "platform.h"
#include "rfm12b.h"
#include <string.h>

#define TIMEOUT_DEFAULT 250
#define TIMEOUT_WRITE 4000

#if FW_TYPE == FW_LDR
#define FW_TARGET FW_APP
#define ADDR_ORIGIN ((uint32_t) & __app_start)
#define ADDR_END ((uint32_t) & __app_end)
#elif FW_TYPE == FW_APP
#define FW_TARGET FW_LDR
#define ADDR_ORIGIN ((uint32_t) & __ldr_start)
#define ADDR_END ((uint32_t) & __ldr_end)
#endif

extern bool g_stay_in_boot;

void air_flash_sts_cb(uint8_t dev_idx, const uint8_t *data, uint8_t data_len)
{
	return;
	// g_stay_in_boot = true;
	// rmf75_delay_packets(TIMEOUT_DEFAULT + 100);
	// fw_header_check_all();
	// rfm75_tx_force(dev_idx, (uint8_t[]){AIRPROTO_CMD_FLASH_STS, g_fw_info[0].locked, g_fw_info[1].locked, g_fw_info[2].locked}, 4);
}

void air_flash_type_cb(uint8_t dev_idx, const uint8_t *data, uint8_t data_len)
{
	return;
	// g_stay_in_boot = true;
	// rmf75_delay_packets(TIMEOUT_DEFAULT + 100);
	// rfm75_tx_force(dev_idx, (uint8_t[]){AIRPROTO_CMD_FLASH_TYPE, FW_TYPE}, 2);
}

void air_flash_write_cb(uint8_t dev_idx, const uint8_t *data, uint8_t data_len)
{
	return;
	// g_stay_in_boot = true;
	// rmf75_delay_packets(TIMEOUT_WRITE + 100);
	// int sts = 1;
	// uint8_t fw_sel = 0xFF;
	// uint32_t offset = 0;
	// if(data_len >= 6)
	// {
	// 	uint32_t size_to_write = data_len - 6;
	// 	fw_sel = data[1];
	// 	memcpy(&offset, &data[2], 4);
	// 	if(fw_sel == FW_APP + 1)
	// 	{
	// 		if(offset <= CFG_END - CFG_ORIGIN && CFG_END - CFG_ORIGIN - size_to_write >= offset)
	// 		{
	// 			if(offset == 0) platform_flash_erase_flag_reset_sect_cfg();
	// 			sts = platform_flash_write(CFG_ORIGIN + offset, &data[6], size_to_write);
	// 		}
	// 	}
	// 	else
	// 	{
	// 		if(offset <= ADDR_END - ADDR_ORIGIN && ADDR_END - ADDR_ORIGIN - size_to_write >= offset)
	// 		{
	// 			if(offset == 0) platform_flash_erase_flag_reset();
	// 			sts = platform_flash_write(ADDR_ORIGIN + offset, &data[6], size_to_write);
	// 		}
	// 	}
	// }
	// uint8_t pkt[7] = {AIRPROTO_CMD_FLASH_WRITE, fw_sel};
	// memcpy(&pkt[2], &offset, 4);
	// pkt[6] = sts;
	// rfm75_tx_force(dev_idx, pkt, 7);
}

void air_flash_read_cb(uint8_t dev_idx, const uint8_t *data, uint8_t data_len)
{
	return;
	// g_stay_in_boot = true;
	// rmf75_delay_packets(TIMEOUT_DEFAULT + 100);
	// int sts = 1;
	// if(data_len == 6)
	// {
	// 	uint8_t fw_sel = data[1];
	// 	uint32_t offset;
	// 	memcpy(&offset, &data[2], 4);

	// 	sts = 2;
	// 	if(fw_sel == FW_APP + 1)
	// 	{
	// 		sts = 0;
	// 		if(config_validate() == CONFIG_STS_OK)
	// 		{
	// 			uint32_t size_to_send = config_get_size() - offset;
	// 			if(size_to_send > RFM75_MAX_PAYLOAD_SIZE - 6) size_to_send = RFM75_MAX_PAYLOAD_SIZE - 6;
	// 			uint8_t pkt[RFM75_MAX_PAYLOAD_SIZE] = {AIRPROTO_CMD_FLASH_READ, fw_sel};
	// 			memcpy(&pkt[2], &offset, 4);
	// 			memcpy(&pkt[6], (uint8_t *)CFG_ORIGIN + offset, size_to_send);
	// 			rfm75_tx_force(dev_idx, pkt, 6 + size_to_send);
	// 		}
	// 		else
	// 		{
	// 			uint8_t pkt[RFM75_MAX_PAYLOAD_SIZE] = {AIRPROTO_CMD_FLASH_READ, fw_sel};
	// 			memcpy(&pkt[2], &offset, 4);
	// 			rfm75_tx_force(dev_idx, pkt, 6);
	// 		}
	// 	}
	// 	else if(fw_sel < FW_COUNT)
	// 	{
	// 		sts = 0;
	// 		if(g_fw_info[fw_sel].locked == 0 && offset < g_fw_info[fw_sel].size)
	// 		{
	// 			uint32_t size_to_send = g_fw_info[fw_sel].size - offset;
	// 			if(size_to_send > RFM75_MAX_PAYLOAD_SIZE - 6) size_to_send = RFM75_MAX_PAYLOAD_SIZE - 6;
	// 			uint8_t pkt[RFM75_MAX_PAYLOAD_SIZE] = {AIRPROTO_CMD_FLASH_READ, fw_sel};
	// 			memcpy(&pkt[2], &offset, 4);
	// 			memcpy(&pkt[6], (uint8_t *)g_fw_info[fw_sel].addr + offset, size_to_send);
	// 			rfm75_tx_force(dev_idx, pkt, 6 + size_to_send);
	// 		}
	// 		else
	// 		{
	// 			uint8_t pkt[RFM75_MAX_PAYLOAD_SIZE] = {AIRPROTO_CMD_FLASH_READ, fw_sel};
	// 			memcpy(&pkt[2], &offset, 4);
	// 			rfm75_tx_force(dev_idx, pkt, 6);
	// 		}
	// 	}
	// }
	// if(sts) rfm75_tx_force(dev_idx, (uint8_t[]){AIRPROTO_CMD_FLASH_READ, sts}, 2);
}