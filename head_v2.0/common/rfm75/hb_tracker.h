#ifndef HB_TRACKER_H__
#define HB_TRACKER_H__

#include <stdbool.h>
#include <stdint.h>

void hb_tracker_init(uint8_t id, uint32_t timeout);
void hb_tracker_update(uint8_t id);
void hb_tracker_poll(uint32_t diff_ms);
bool hb_tracker_is_timeout(uint8_t id);
uint32_t hb_tracker_get_timeout(uint8_t id);

#endif // HB_TRACKER_H__