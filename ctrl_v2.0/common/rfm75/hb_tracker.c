#include "hb_tracker.h"
#include "platform.h"
#include <stddef.h>

#define MAX_NODES 16
#define MAX_TIMEOUT 0xFFFFFFFF

static struct
{
	uint32_t timeout;
	uint32_t timeout_cnt;
} nodes[MAX_NODES] = {0};

void hb_tracker_init(uint8_t id, uint32_t timeout)
{
	if(id >= MAX_NODES) return;
	nodes[id].timeout = timeout;
	nodes[id].timeout_cnt = 0;
}

void hb_tracker_update(uint8_t id)
{
	if(id >= MAX_NODES) return;
	nodes[id].timeout_cnt = 0;
}

void hb_tracker_poll(uint32_t diff_ms)
{
	for(uint32_t i = 0; i < MAX_NODES; i++)
	{
		if(nodes[i].timeout)
		{
			nodes[i].timeout_cnt = nodes[i].timeout_cnt > MAX_TIMEOUT - diff_ms ? MAX_TIMEOUT : nodes[i].timeout_cnt + diff_ms;
		}
	}
}

bool hb_tracker_is_timeout(uint8_t id)
{
	if(id >= MAX_NODES) return true;
	return nodes[id].timeout_cnt >= nodes[id].timeout;
}

uint32_t hb_tracker_get_timeout(uint8_t id)
{
	if(id >= MAX_NODES) return MAX_TIMEOUT;
	return nodes[id].timeout_cnt;
}