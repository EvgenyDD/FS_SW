#include "hb_tracker.h"
#include "main.h"

#define MAX_NODES 2

static hbt_node_t nodes[MAX_NODES];
static uint32_t nodes_count = 0;

hbt_node_t *hb_tracker_init(uint8_t node_id, uint32_t timeout_ms)
{
    if(nodes_count >= MAX_NODES) return NULL;

    nodes[nodes_count].node_id = node_id;
    nodes[nodes_count].timeout_ms = timeout_ms;
    nodes[nodes_count].last_hb_ms = 0;
    nodes[nodes_count].is_timeout = true;
    nodes[nodes_count].to_count = 0;

    nodes_count++;

    return &nodes[nodes_count - 1];
}

bool hb_tracker_update(uint8_t node_id)
{
    for(uint32_t i = 0; i < nodes_count; i++)
    {
        if(node_id == nodes[i].node_id)
        {
            nodes[i].last_hb_ms = HAL_GetTick();
            return false;
        }
    }
    return true;
}

bool hb_tracker_update_state(uint8_t node_id, bool state)
{
    for(uint32_t i = 0; i < nodes_count; i++)
    {
        if(node_id == nodes[i].node_id)
        {
            nodes[i].is_timeout = state;
            return false;
        }
    }
    return true;
}

void hb_tracker_update_state_by_iterator(uint8_t iterator, bool state)
{
    nodes[iterator].is_timeout = state;
    if(state) nodes[iterator].to_count++;
}

bool hb_tracker_is_timeout(const hbt_node_t *node) { return node->is_timeout; }
// uint32_t hb_tracker_get_timeout(const hbt_node_t *node) { return HAL_GetTick() - node->last_hb_ms; }