#ifndef HB_TRACKER_H
#define HB_TRACKER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t node_id;
    uint32_t timeout_ms;
    uint32_t last_hb_ms;
    bool is_timeout;
    uint32_t to_count;
} hbt_node_t;

hbt_node_t *hb_tracker_init(uint8_t node_id, uint32_t timeout_ms);

bool hb_tracker_update(uint8_t node_id);
bool hb_tracker_update_state(uint8_t node_id, bool state);
void hb_tracker_update_state_by_iterator(uint8_t iterator, bool state);

bool hb_tracker_is_timeout(const hbt_node_t *node);
uint32_t hb_tracker_get_timeout(const hbt_node_t *node);

#endif // HB_TRACKER_H