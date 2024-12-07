/**
 * AIR Protocol
 * Master Algorithm:
 *      for(slaves)
 *      {
 *           SACK slave
 *           if(SACK failed) continue;
 *           TX data to slave (if available)
 *           SEND slave SLS (Slave Listen Start)
 *           Listen to slave (with timeout)
 *           Wait until SLE (Slave Listen End) received from slave
 *      }
 *      if(no data to TX) 
 *      {
 *          wait idle timeout
 *          if(data available)
 *          {
 *              get out from idle timeout
 *          }
 *      }
 * 
 * Slave Algorithm:
 *      RX data
 *      if(SLS received with slave ID)
 *      {
 *          TX data to Master (if available)
 *          Send SLE to Master
 *      }
 * 
 */
#include "air_protocol.h"
#include "debug.h"
#include "hb_tracker.h"
#include "main.h"
#include "rfm12b.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern void process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len);
extern uint32_t get_random(void);

// 0 - none, 1 - only critical, 2 - all
#ifndef AIR_PROTO_DEBUG_LEVEL
#define AIR_PROTO_DEBUG_LEVEL 1
#endif

#if AIR_PROTO_DEBUG_LEVEL == 0
#define AIR_PROTO_DEBUG(...)
#define AIR_PROTO_DEBUG_CRITICAL(...)
#elif AIR_PROTO_DEBUG_LEVEL == 1
#define AIR_PROTO_DEBUG(...)
#define AIR_PROTO_DEBUG_CRITICAL(...) debug(__VA_ARGS__)
#else
#define AIR_PROTO_DEBUG(...) debug(__VA_ARGS__)
#define AIR_PROTO_DEBUG_CRITICAL(...) debug(__VA_ARGS__)
#endif

#define ACK_TIMEOUT_MS 20

#ifdef AIR_PROTOCOL_MASTER
#pragma message("Device is AIR PROTOCOL Master")
#else
#pragma message("Device is AIR PROTOCOL Slave")
#endif

typedef enum
{
#ifdef AIR_PROTOCOL_MASTER
    AIR_PROTOCOL_STATE_SACK,
#endif
    AIR_PROTOCOL_STATE_TX,
#ifdef AIR_PROTOCOL_MASTER
    AIR_PROTOCOL_STATE_NODE_SLS,
#endif
    AIR_PROTOCOL_STATE_RX,
#ifdef AIR_PROTOCOL_MASTER
    AIR_PROTOCOL_STATE_SELECT_NEXT_NODE,
    AIR_PROTOCOL_STATE_IDLE,
#endif
} AIR_PROTOCOL_STATE;

#ifdef AIR_PROTOCOL_MASTER
#define TIMEOUT_IDLE_MS 500
#define TIMEOUT_SLAVE_TX_LOST 300

static uint32_t current_process_node = 0;
static uint32_t timeout_slave_tx_lost = 0;
static uint32_t timeout_idle = 0;
static AIR_PROTOCOL_STATE state = AIR_PROTOCOL_STATE_SACK;

static bool periodic_sleep_enable = true;

void air_protocol_set_periodic_sleep(bool new_state) { periodic_sleep_enable = new_state; }
inline static void regenerate_timeout_slave_tx_lost(void) { timeout_slave_tx_lost = HAL_GetTick() + TIMEOUT_SLAVE_TX_LOST; }

#else

#define MASTER_HB_TIMEOUT 600

static uint32_t to_counter_master = 0;
static AIR_PROTOCOL_STATE state = AIR_PROTOCOL_STATE_RX;

#if NODES_COUNT > 1
#error "Slave can have only single NODES_COUNT"
#endif
#endif

typedef struct
{
    uint8_t data[MSG_COUNT][RFM12B_MAXDATA];
    uint8_t data_len[MSG_COUNT];
    uint32_t p_push;
    uint32_t p_pull;
    uint8_t dest_id;
} storage_t;

static storage_t storage[NODES_COUNT];

inline static bool is_tx_node_pending(uint8_t iterator) { return storage[iterator].p_push != storage[iterator].p_pull; }

inline static void change_state(AIR_PROTOCOL_STATE new)
{
    if(state != new)
    {
        AIR_PROTO_DEBUG(DBG_INFO ">%d\n", new);
    }
    state = new;
}

static bool waitForAck(uint8_t dest_node_id)
{
    uint32_t now = HAL_GetTick();
    while(HAL_GetTick() - now <= ACK_TIMEOUT_MS)
        if(rfm12b_is_ack_received(dest_node_id))
            return true;
    return false;
}

static void trx_send_nack(uint8_t node_id, const uint8_t *payload, uint8_t payload_length)
{
    // rfm12b_wakeup();
    AIR_PROTO_DEBUG(DBG_INFO "n%d<%d\n", node_id, payload[0]);
    rfm12b_send(node_id, payload, payload_length, false, false);
}

/**
 * @brief 
 * 
 * @param node_id 
 * @param payload 
 * @param payload_length 
 * @return true ACK not received
 * @return false ACK received
 */
static bool _trx_send_ack(uint8_t node_id, const uint8_t *payload, uint8_t payload_length)
{
    // rfm12b_wakeup();
    rfm12b_send(node_id, payload, payload_length, true, true);

    if(waitForAck(node_id))
        return false;
    else
        return true;
}

/// true - NACKed, false - ACKed
static bool trx_send_ack(uint8_t node_id, const uint8_t *payload, uint8_t payload_length)
{
    for(uint32_t i = 0; i < 3; i++)
    {
        if(_trx_send_ack(node_id, payload, payload_length) == false)
        {
            AIR_PROTO_DEBUG(DBG_INFO "A%d<%d(%d)\n", node_id, payload[0], payload_length);
            return false;
        }
        AIR_PROTO_DEBUG_CRITICAL(DBG_ERR "a%d<%d(%d)\n", node_id, payload[0], payload_length);
    }
    return true;
}

static bool trx_send_ack_single(uint8_t node_id, const uint8_t *payload, uint8_t payload_length)
{
    if(_trx_send_ack(node_id, payload, payload_length) == false)
    {
        AIR_PROTO_DEBUG(DBG_INFO "A%d<%d(%d)\n", node_id, payload[0], payload_length);
        return false;
    }

    if(payload[0] == RFM_NET_CMD_NOP)
    {
        AIR_PROTO_DEBUG(DBG_ERR "SACK %d\n", storage[current_process_node].dest_id);
    }
    else if(payload[0] == RFM_NET_CMD_SLS)
    {
        AIR_PROTO_DEBUG_CRITICAL(DBG_ERR "SLS %d\n");
    }
    else
    {
        AIR_PROTO_DEBUG_CRITICAL(DBG_ERR "a%d<%d(%d)\n", node_id, payload[0], payload_length);
    }

    return true;
}

#ifdef AIR_PROTOCOL_MASTER
static bool send_nop(uint8_t dest_id)
{
    static uint8_t nop_data[] = {RFM_NET_CMD_NOP, OWN_ID};
    return trx_send_ack_single(dest_id, nop_data, sizeof(nop_data));
}
inline static bool send_sls(uint8_t dest_id)
{
    static uint8_t sls_data[] = {RFM_NET_CMD_SLS, OWN_ID};
    return trx_send_ack(dest_id, sls_data, sizeof(sls_data));
}
#endif
#ifndef AIR_PROTOCOL_MASTER
static bool send_sle(uint8_t dest_id)
{
    static uint8_t sle_data[] = {RFM_NET_CMD_SLE, OWN_ID};
    return trx_send_ack(dest_id, sle_data, sizeof(sle_data));
}
#endif

static void poll_rx(void)
{
    if(rfm12b_rx_complete())
    {
        if(rfm12b_is_crc_pass())
        {
            hb_tracker_update_state(rfm12b_get_sender_id(), false);

            // debug(DBG_WARN"\tR %d %d\n", rfm12b_get_data()[0],rfm12b_get_data_len() );

            if(rfm12b_get_dest_id() == RFM_NET_ID_BROADCAST)
            {
                // __disable_irq();
                // volatile uint8_t *data = rfm12b_get_data();
                // uint8_t data_len = rfm12b_get_data_len();
                // __enable_irq();
            }
            else if(rfm12b_get_dest_id() == OWN_ID)
            {
                // __disable_irq();
                HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);

#ifdef AIR_PROTOCOL_MASTER
                regenerate_timeout_slave_tx_lost();
#endif

                bool precessed = false;
                if(rfm12b_get_data_len() >= 2)
                {
#ifdef AIR_PROTOCOL_MASTER
                    if(rfm12b_get_data()[0] == RFM_NET_CMD_SLE)
                    {
                        AIR_PROTO_DEBUG(DBG_INFO "R SLE\n");
                        change_state(AIR_PROTOCOL_STATE_SELECT_NEXT_NODE);
                        hb_tracker_update_state(rfm12b_get_sender_id(), false);
                        precessed = true;
                    }
#endif

#ifndef AIR_PROTOCOL_MASTER
                    if(rfm12b_get_data()[0] == RFM_NET_CMD_SLS)
                    {
                        AIR_PROTO_DEBUG(DBG_INFO "R SLS\n");
                        change_state(AIR_PROTOCOL_STATE_TX);
                        precessed = true;
                    }
                    if(rfm12b_get_data()[0] == RFM_NET_CMD_NOP)
                    {
                        precessed = true;
                    }
#endif
                }

                if(precessed == false) process_data(rfm12b_get_sender_id(), rfm12b_get_data(), rfm12b_get_data_len());

                // __enable_irq();
                HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

                if(rfm12b_is_ack_requested())
                {
                    rfm12b_sendACK("", 0, false);
                }
            }
            else
            {
                // listen other node - somebody is transmitting
                AIR_PROTO_DEBUG_CRITICAL(DBG_WARN ":");
            }
        }
        else
        {
            AIR_PROTO_DEBUG_CRITICAL(DBG_ERR "BAD-CRC\n");
        }
    }
}

void air_protocol_poll(void)
{
    switch(state)
    {
#ifdef AIR_PROTOCOL_MASTER
    case AIR_PROTOCOL_STATE_SACK:
    {
        if(++current_process_node >= NODES_COUNT)
        {
            current_process_node = 0;
        }
        bool sts = send_nop(storage[current_process_node].dest_id);
        hb_tracker_update_state_by_iterator(current_process_node, sts);
        if(sts)
        {
            return;
        }
        change_state(AIR_PROTOCOL_STATE_TX);
    }
        return;

    case AIR_PROTOCOL_STATE_TX:
        if(storage[current_process_node].p_push != storage[current_process_node].p_pull)
        {
            storage[current_process_node].p_pull++;
            if(storage[current_process_node].p_pull >= MSG_COUNT) storage[current_process_node].p_pull = 0;

            if(storage[current_process_node].dest_id == RFM_NET_ID_BROADCAST)
                trx_send_nack(storage[current_process_node].dest_id,
                              storage[current_process_node].data[storage[current_process_node].p_pull],
                              storage[current_process_node].data_len[storage[current_process_node].p_pull]);
            else
                hb_tracker_update_state_by_iterator(current_process_node,
                                                    trx_send_ack(storage[current_process_node].dest_id,
                                                                 storage[current_process_node].data[storage[current_process_node].p_pull],
                                                                 storage[current_process_node].data_len[storage[current_process_node].p_pull]));
            return;
        }

        regenerate_timeout_slave_tx_lost();
        change_state(AIR_PROTOCOL_STATE_NODE_SLS);
        return;

    case AIR_PROTOCOL_STATE_NODE_SLS:
    {
        AIR_PROTO_DEBUG(DBG_INFO "T SLS %d\n", storage[current_process_node].dest_id);
        bool sts = send_sls(storage[current_process_node].dest_id);
        hb_tracker_update_state_by_iterator(current_process_node, sts);
        if(sts)
        {
            change_state(AIR_PROTOCOL_STATE_SACK);
            return;
        }
        change_state(AIR_PROTOCOL_STATE_RX);
    }
        return;

    case AIR_PROTOCOL_STATE_RX:
        poll_rx();
        if(timeout_slave_tx_lost < HAL_GetTick())
        {
            AIR_PROTO_DEBUG_CRITICAL(DBG_ERR "%d RX TO\n", storage[current_process_node].dest_id);
            change_state(AIR_PROTOCOL_STATE_SACK);
        }
        return;

    case AIR_PROTOCOL_STATE_SELECT_NEXT_NODE:
        for(uint32_t i = 0; i < NODES_COUNT; i++)
        {
            if(is_tx_node_pending(i))
            {
                change_state(AIR_PROTOCOL_STATE_SACK);
                return;
            }
        }
        timeout_idle = HAL_GetTick() + TIMEOUT_IDLE_MS;
        change_state(periodic_sleep_enable ? AIR_PROTOCOL_STATE_IDLE : AIR_PROTOCOL_STATE_SACK);
        return;

    case AIR_PROTOCOL_STATE_IDLE:
        for(uint32_t i = 0; i < NODES_COUNT; i++)
        {
            if(is_tx_node_pending(i))
            {
                change_state(AIR_PROTOCOL_STATE_SACK);
                return;
            }
        }
        if(timeout_idle < HAL_GetTick())
        {
            change_state(AIR_PROTOCOL_STATE_SACK);
        }
        return;

#else
    case AIR_PROTOCOL_STATE_RX:
        poll_rx();
        if(to_counter_master + MASTER_HB_TIMEOUT < HAL_GetTick())
        {
            hb_tracker_update_state_by_iterator(0, true);
#warning "Add master ACK & request monitor "
        }
        return;

    case AIR_PROTOCOL_STATE_TX:
    {
        if(storage[0].p_push != storage[0].p_pull)
        {
            storage[0].p_pull++;
            if(storage[0].p_pull >= MSG_COUNT) storage[0].p_pull = 0;

            if(storage[0].dest_id == RFM_NET_ID_BROADCAST)
                trx_send_nack(storage[0].dest_id,
                              storage[0].data[storage[0].p_pull],
                              storage[0].data_len[storage[0].p_pull]);
            else
                hb_tracker_update_state_by_iterator(0,
                                                    trx_send_ack(storage[0].dest_id,
                                                                 storage[0].data[storage[0].p_pull],
                                                                 storage[0].data_len[storage[0].p_pull]));
            return;
        }

        bool sts = send_sle(RFM_NET_ID_MASTER);
        hb_tracker_update_state_by_iterator(0, sts);
        to_counter_master = HAL_GetTick() + MASTER_HB_TIMEOUT;
        change_state(AIR_PROTOCOL_STATE_RX);
    }
        return;
#endif

    default:
        return;
    }
}

void air_protocol_send_async(uint8_t iterator, const uint8_t *payload, uint8_t payload_length)
{
    if(iterator < NODES_COUNT && payload_length < RFM12B_MAXDATA)
    {
        uint8_t next_push = storage[iterator].p_push + 1;
        if(next_push >= MSG_COUNT) next_push = 0;
        if(next_push != storage[iterator].p_pull)
        {
            memcpy(&storage[iterator].data[next_push], payload, payload_length);
            storage[iterator].data_len[next_push] = payload_length;
            storage[iterator].p_push = next_push;
        }
    }
}

void air_protocol_init(void)
{
    static uint8_t air_protocol_net_key[RFM_NET_KEY_LENGTH + 1] = RFM_NET_KEY;

    memset(storage, 0, sizeof(storage));

    rfm12b_init(RFM_NET_GATEWAY);
    rfm12b_encrypt(air_protocol_net_key, RFM_NET_KEY_LENGTH);

    // rfm12b_sleep();
}

void air_protocol_init_node(uint8_t iterator, uint8_t node_id)
{
    if(iterator < NODES_COUNT)
    {
        storage[iterator].dest_id = node_id;
    }
}
