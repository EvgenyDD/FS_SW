#include "platform.h"
#include "rfm12b.h"
#include "usbd_core_dfu.h"
#include <string.h>

#define TIMEOUT_DEFAULT 250
#define TIMEOUT_WRITE 4000

static int cb_rfm75_get_sts(USB_OTG_CORE_HANDLE *dev, USB_SETUP_REQ *req, sub_resp_t *resp, uint8_t dev_idx)
{
	return 1;
	// rmf75_delay_packets(TIMEOUT_DEFAULT + 100);
	// resp->usb_dev = dev;
	// resp->usb_req = *req;
	// resp->pend_mask = RESP_PEND_FW_STS;
	// resp->timeout_value = TIMEOUT_DEFAULT;
	// rfm75_tx_force(dev_idx, (uint8_t[]){AIRPROTO_CMD_FLASH_STS}, 1);
	// return 0;
}

static int cb_rfm75_get_fw_type(USB_OTG_CORE_HANDLE *dev, USB_SETUP_REQ *req, sub_resp_t *resp, uint8_t dev_idx)
{
	return 1;
	// rmf75_delay_packets(TIMEOUT_DEFAULT + 100);
	// resp->usb_dev = dev;
	// resp->usb_req = *req;
	// resp->pend_mask = RESP_PEND_FW_TYPE;
	// resp->timeout_value = TIMEOUT_DEFAULT;
	// rfm75_tx_force(dev_idx, (uint8_t[]){AIRPROTO_CMD_FLASH_TYPE}, 1);
	// return 0;
}

static int cb_rfm75_wr(USB_OTG_CORE_HANDLE *dev, USB_SETUP_REQ *req, sub_resp_t *resp, uint8_t dev_idx, uint8_t fw_sel, uint32_t addr_offset, const uint8_t *data, uint32_t size)
{
	return 1;
	// if(6 + size > RFM75_MAX_PAYLOAD_SIZE) return 1;
	// rmf75_delay_packets(TIMEOUT_WRITE + 100);
	// resp->usb_dev = dev;
	// resp->usb_req = *req;
	// resp->pend_mask = RESP_PEND_FW_WR;
	// resp->timeout_value = TIMEOUT_WRITE;
	// resp->fw_sel = fw_sel;
	// resp->offset = addr_offset;

	// uint8_t pkt[RFM75_MAX_PAYLOAD_SIZE] = {AIRPROTO_CMD_FLASH_WRITE, fw_sel};
	// memcpy(&pkt[2], &addr_offset, 4);
	// memcpy(&pkt[6], data, size);
	// rfm75_tx_force(dev_idx, pkt, 6 + size);
	// return USBD_POSTPONE;
}

static int cb_rfm75_rd(USB_OTG_CORE_HANDLE *dev, USB_SETUP_REQ *req, sub_resp_t *resp, uint8_t dev_idx, uint8_t fw_sel, uint32_t addr_offset)
{
	return 1;
	// rmf75_delay_packets(TIMEOUT_DEFAULT + 100);
	// resp->usb_dev = dev;
	// resp->usb_req = *req;
	// resp->pend_mask = RESP_PEND_FW_RD;
	// resp->timeout_value = TIMEOUT_DEFAULT;
	// resp->fw_sel = fw_sel;
	// resp->offset = addr_offset;

	// resp->flash_rx_size = 0;
	// uint8_t pkt[6] = {AIRPROTO_CMD_FLASH_READ, fw_sel};
	// memcpy(&pkt[2], &addr_offset, 4);
	// rfm75_tx_force(dev_idx, pkt, 6);
	// return 0;
}

static void cb_rfm75_reboot(uint8_t dev_idx)
{
	// rfm75_tx_force(dev_idx, (uint8_t[]){AIRPROTO_CMD_REBOOT}, 1);
}

void rf_fw_sts_cb(uint8_t dev_idx, const uint8_t *data, uint32_t data_len)
{
	return;
	// if(dfu_app_cb[dev_idx].resp.pend_mask != RESP_PEND_FW_STS) return;
	// dfu_app_cb[dev_idx].resp.pend_mask = 0;
	// if(data)
	// {
	// 	if(data_len == 1 + 3)
	// 	{
	// 		memcpy(dfu_app_cb[dev_idx].resp.fw_sts, &data[1], 3);
	// 		USBD_CtlSendData(dfu_app_cb[dev_idx].resp.usb_dev, dfu_app_cb[dev_idx].resp.fw_sts, 3);
	// 		return;
	// 	}
	// }
	// USBD_CtlError(dfu_app_cb[dev_idx].resp.usb_dev, &dfu_app_cb[dev_idx].resp.usb_req);
}

void rf_fw_type_cb(uint8_t dev_idx, const uint8_t *data, uint32_t data_len)
{
	return;
	// if(dfu_app_cb[dev_idx].resp.pend_mask != RESP_PEND_FW_TYPE) return;
	// dfu_app_cb[dev_idx].resp.pend_mask = 0;
	// if(data)
	// {
	// 	if(data_len == 1 + 1)
	// 	{
	// 		memcpy(&dfu_app_cb[dev_idx].resp.fw_type, &data[1], 1);
	// 		USBD_CtlSendData(dfu_app_cb[dev_idx].resp.usb_dev, &dfu_app_cb[dev_idx].resp.fw_type, 1);
	// 		return;
	// 	}
	// }
	// USBD_CtlError(dfu_app_cb[dev_idx].resp.usb_dev, &dfu_app_cb[dev_idx].resp.usb_req);
}

void rf_flash_read_cb(uint8_t dev_idx, const uint8_t *data, uint32_t data_len)
{
	return;
	// if(dfu_app_cb[dev_idx].resp.pend_mask != RESP_PEND_FW_RD) return;
	// dfu_app_cb[dev_idx].resp.pend_mask = 0;
	// if(data)
	// {
	// 	if(data_len >= 6)
	// 	{
	// 		dfu_app_cb[dev_idx].resp.flash_rx_size = data_len - 6;
	// 		uint32_t offset;
	// 		memcpy(&offset, &data[2], 4);
	// 		if(dfu_app_cb[dev_idx].resp.fw_sel == data[1] && dfu_app_cb[dev_idx].resp.offset == offset)
	// 		{
	// 			memcpy(dfu_app_cb[dev_idx].resp.flash_rx, &data[6], dfu_app_cb[dev_idx].resp.flash_rx_size);
	// 			USBD_CtlSendData(dfu_app_cb[dev_idx].resp.usb_dev, dfu_app_cb[dev_idx].resp.flash_rx, dfu_app_cb[dev_idx].resp.flash_rx_size);
	// 			return;
	// 		}
	// 	}
	// }
	// USBD_CtlError(dfu_app_cb[dev_idx].resp.usb_dev, &dfu_app_cb[dev_idx].resp.usb_req);
}

void rf_flash_write_cb(uint8_t dev_idx, const uint8_t *data, uint32_t data_len)
{
	return;
	// if(dfu_app_cb[dev_idx].resp.pend_mask != RESP_PEND_FW_WR) return;
	// dfu_app_cb[dev_idx].resp.pend_mask = 0;
	// uint8_t err = 1;
	// if(data)
	// {
	// 	if(data_len == 7)
	// 	{
	// 		uint32_t offset;
	// 		memcpy(&offset, &data[2], 4);
	// 		if(dfu_app_cb[dev_idx].resp.fw_sel == data[1] && dfu_app_cb[dev_idx].resp.offset == offset) err = data[6];
	// 	}
	// }
	// if(err)
	// {
	// 	DCD_EP_Stall(dfu_app_cb[dev_idx].resp.usb_dev, 0x80);
	// 	DCD_EP_Stall(dfu_app_cb[dev_idx].resp.usb_dev, 0);
	// 	USB_OTG_EP0_OutStart(dfu_app_cb[dev_idx].resp.usb_dev);
	// }
	// else
	// {
	// 	USBD_CtlSendStatus(dfu_app_cb[dev_idx].resp.usb_dev);
	// }
}

dfu_app_cb_t dfu_app_cb[] = {
	{"fs_head2", cb_rfm75_get_sts, cb_rfm75_get_fw_type, cb_rfm75_reboot, cb_rfm75_rd, cb_rfm75_wr, 0, {{0}, 0, 0, 0, {0}, 0, 0, 0, NULL, {0}}},
};

uint32_t dfu_app_cb_count = sizeof(dfu_app_cb) / sizeof(dfu_app_cb[0]);

void dfu_sub_poll(uint32_t diff_ms)
{
	for(uint32_t i = 0; i < dfu_app_cb_count; i++)
	{
		if(dfu_app_cb[i].resp.timeout_value)
		{
			if(dfu_app_cb[i].resp.timeout_value <= diff_ms)
			{
				if(dfu_app_cb[i].resp.pend_mask == RESP_PEND_FW_STS) rf_fw_sts_cb(i, NULL, 0);
				if(dfu_app_cb[i].resp.pend_mask == RESP_PEND_FW_TYPE) rf_fw_type_cb(i, NULL, 0);
				if(dfu_app_cb[i].resp.pend_mask == RESP_PEND_FW_WR) rf_flash_write_cb(i, NULL, 0);
				if(dfu_app_cb[i].resp.pend_mask == RESP_PEND_FW_RD) rf_flash_read_cb(i, NULL, 0);
			}
			dfu_app_cb[i].resp.timeout_value = dfu_app_cb[i].resp.timeout_value <= diff_ms ? 0 : dfu_app_cb[i].resp.timeout_value - diff_ms;
		}
	}
}