#ifndef DFU_SUB_FLASH_H__
#define DFU_SUB_FLASH_H__

#include "rfm12b.h"
#include "usbd_req.h"

enum
{
	RESP_PEND_FW_STS = (1 << 0),
	RESP_PEND_FW_TYPE = (1 << 1),
	RESP_PEND_FW_WR = (1 << 2),
	RESP_PEND_FW_RD = (1 << 3),
};

typedef struct
{
	uint8_t fw_sts[3], fw_type; // buffers for USB tx

	uint8_t fw_sel;
	uint32_t offset;
	// uint8_t flash_rx[RFM75_MAX_PAYLOAD_SIZE - 4 - 1]; // buffer for USB tx
	uint8_t flash_rx[1]; // buffer for USB tx
	uint32_t flash_rx_size;

	uint8_t pend_mask;
	uint32_t timeout_value;

	USB_OTG_CORE_HANDLE *usb_dev;
	USB_SETUP_REQ usb_req;
} sub_resp_t;

typedef struct
{
	const char *name;
	int (*p_get_sts)(USB_OTG_CORE_HANDLE *dev, USB_SETUP_REQ *req, sub_resp_t *resp, uint8_t dev_idx);
	int (*p_get_fw_type)(USB_OTG_CORE_HANDLE *dev, USB_SETUP_REQ *req, sub_resp_t *resp, uint8_t dev_idx);
	void (*p_reboot)(uint8_t dev_idx);
	int (*p_rd)(USB_OTG_CORE_HANDLE *dev, USB_SETUP_REQ *req, sub_resp_t *resp, uint8_t dev_idx, uint8_t fw_sel, uint32_t addr_offset);
	int (*p_wr)(USB_OTG_CORE_HANDLE *dev, USB_SETUP_REQ *req, sub_resp_t *resp, uint8_t dev_idx, uint8_t fw_sel, uint32_t addr_offset, const uint8_t *data, uint32_t size);
	uint8_t dev_idx;
	sub_resp_t resp;
} dfu_app_cb_t;

void dfu_sub_poll(uint32_t diff_ms);

void rf_fw_sts_cb(uint8_t dev_idx, const uint8_t *data, uint32_t data_len);
void rf_fw_type_cb(uint8_t dev_idx, const uint8_t *data, uint32_t data_len);
void rf_flash_read_cb(uint8_t dev_idx, const uint8_t *data, uint32_t data_len);
void rf_flash_write_cb(uint8_t dev_idx, const uint8_t *data, uint32_t data_len);

extern dfu_app_cb_t dfu_app_cb[];
extern uint32_t dfu_app_cb_count;

#endif // DFU_SUB_FLASH_H__