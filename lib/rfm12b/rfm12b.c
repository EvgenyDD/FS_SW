/**
 * Source: https://github.com/LowPowerLab/RFM12B
 **/
#include "rfm12b.h"
#include "air_protocol.h" // this include is architecture wrong
#include "debug.h"
#include "main.h"

#include <string.h>

#define RFM_SPI SPI3

#define DELAY_US(x)                      \
    for(uint32_t i = 0; i < 12 * x; i++) \
        asm("nop");
#warning "CHECK OVERFLOWS!!!"
// ===============

#define RFM12B_CMD_RESET 0xFE00
#define RFM12B_CMD_CONFIG 0x8000
#define RFM12B_CMD_POWER 0x8200
#define RFM12B_CMD_FREQ 0xA000
#define RFM12B_CMD_DATA_RATE 0xC600
#define RFM12B_CMD_RX_CTRL 0x9000
#define RFM12B_CMD_DATA_FILTER 0xC228
#define RFM12B_CMD_FIFO_AND_RESET 0xCA00
#define RFM12B_CMD_SYNC_PATTERN 0xCE00
#define RFM12B_CMD_AFC 0xC400
#define RFM12B_CMD_TX_CONFIG 0x9800
#define RFM12B_CMD_MCU_CLK_DIV 0xC000

#define RF12B_WAKEUP_TIMER 0xE000

#define RFM12B_SLEEP_MODE 0x8205
#define RFM12B_IDLE_MODE 0x820D

#define RFM12B_TXREG_WRITE 0xB800

#define RFM12B_RX_FIFO_READ 0xB000

#define RFM12B_RECEIVER_ON 0x82DD
#define RFM12B_XMITTER_ON 0x823D

// ===============

#define RFM12B_FREQ_BAND_433 0x0010
#define RFM12B_FREQ_BAND_868 0x0020
#define RFM12B_FREQ_BAND_915 0x0030

#define RFM12B_CRYSTAL_LOAD_12PF 0x0007
#define RFM12B_VDI_FAST 0x0000 /// valid data indicator

#define RFM12B_RX_BANDWIDTH_134KHZ 0x00A0

#define RFM12B_LNA_GAIN_0DB 0x0000

#define RFM12B_RSSI_THSH_MINUS_94 0x0002
#define RFM12B_RSSI_THSH_MINUS_103 0x0000

#define RFM12B_DQD_4 0x0004 /* data quality detection */

#define RFM12B_POWER_OUT_0DB 0

#define RFM12B_LBD_BIT 0x0400
#define RFM12B_RSSI_BIT 0x0100

#define RF12_HDR_IDMASK 0x7F
#define RF12_HDR_ACKCTLMASK 0x80
#define RF12_DESTID (rf12_hdr1 & RF12_HDR_IDMASK)
#define RF12_SOURCEID (rf12_hdr2 & RF12_HDR_IDMASK)

/// Shorthand for RF12 group byte in rf12_buf.
// #define rf12_grp rf12_buf[0]
/// pointer to 1st header byte in rf12_buf (CTL + DESTINATIONID)
#define rf12_hdr1 rf12_buf[1]
/// pointer to 2nd header byte in rf12_buf (ACK + SOURCEID)
#define rf12_hdr2 rf12_buf[2]

/// Shorthand for RF12 length byte in rf12_buf.
#define rf12_len rf12_buf[3]
/// Shorthand for first RF12 data byte in rf12_buf.
#define rf12_data (rf12_buf + 4)

// shorthands to simplify sending out the proper ACK when requested
#define RF12_WANTS_ACK ((rf12_hdr2 & RF12_HDR_ACKCTLMASK) && !(rf12_hdr1 & RF12_HDR_ACKCTLMASK))

/// Max transmit/receive buffer: 4 header + data + 2 crc bytes
#define RF_MAX (RFM12B_MAXDATA + 6)

#define PIN_SET(x) x##_GPIO_Port->BSRR = x##_Pin
#define PIN_CLR(x) x##_GPIO_Port->BSRR = ((uint32_t)(x##_Pin)) << 16
#define PIN_WR(x, v) x##_GPIO_Port->BSRR = ((uint32_t)(x##_Pin)) << ((!(v)) * 16)
#define PIN_GET(x) !!(x##_GPIO_Port->IDR & x##_Pin)

enum
{
    TXCRC1 = 0,
    TXCRC2,
    TXTAIL,
    TXDONE,
    TXIDLE,
    TXRECV,
    TXPRE1,
    TXPRE2,
    TXPRE3,
    TXSYN1,
    TXSYN2,
};

#define S_WR()                            \
    while(!(RFM_SPI->SR & SPI_FLAG_RXNE)) \
        asm("nop");

#define S_WT()                           \
    while(!(RFM_SPI->SR & SPI_FLAG_TXE)) \
        asm("nop");

#define S_WB()                     \
    while(SPI3->SR & SPI_FLAG_BSY) \
        asm("nop");

#define S_DT(x)      \
    asm("nop");      \
    RFM_SPI->DR = x; \
    asm("nop");

#define S_DR(x) \
    x = RFM_SPI->DR;

static void (*crypter)(bool) = NULL;
static uint32_t crypt_key[4] = {0}; // encryption key to use
static volatile uint16_t rf12_crc = 0;
static volatile uint32_t rf12_buf_u32[RF_MAX / sizeof(uint32_t)] = {0};
static volatile uint8_t *rf12_buf = (volatile uint8_t *)rf12_buf_u32;
static long rf12_seq = 0;
static uint32_t seqNum = 0;
static volatile int8_t rxstate = TXIDLE;
static volatile uint8_t rxfill = 0;
static uint8_t networkID = 0;

static volatile bool irq_fired = false;

uint8_t rfm12b_get_sender_id(void) { return RF12_SOURCEID; }
bool rfm12b_is_ack_requested(void) { return RF12_WANTS_ACK; }
uint8_t rfm12b_get_data_len(void) { return rf12_buf[3]; }
uint8_t rfm12b_get_dest_id(void) { return RF12_DESTID; }
volatile uint8_t *rfm12b_get_data(void) { return rf12_data; }
bool rfm12b_is_crc_pass(void) { return rf12_crc == 0; }

static uint8_t rfm12b_trx_1b(uint8_t byte)
{
    uint8_t rx;
    __disable_irq();
    PIN_CLR(RFM_CS);
    DELAY_US(1);

    S_WB();
    S_DT(byte);
    S_WT();
    S_WR();
    S_DR(rx);

    PIN_SET(RFM_CS);
    DELAY_US(1);

    __enable_irq();
    return rx;
}

static uint16_t rfm12b_trx_2b(uint16_t word)
{
    uint8_t rx[2];
    __disable_irq();
    PIN_CLR(RFM_CS);
    DELAY_US(1);

    S_WB();
    S_DT(word >> 8);
    S_WT();
    S_DT(word & 0xFF);
    S_WR();
    S_DR(rx[0]);
    S_WR();
    S_DR(rx[1]);

    PIN_SET(RFM_CS);
    DELAY_US(1);
    __enable_irq();
    return (rx[0] << 8) | rx[1];
}

static uint16_t _crc16_update(uint16_t crc, uint8_t a)
{
    crc ^= a;
    for(int i = 0; i < 8; ++i)
    {
        crc = (crc & 1)
                  ? (crc >> 1) ^ 0xA001
                  : (crc >> 1);
    }

    return crc;
}

// XXTEA by David Wheeler, adapted from http://en.wikipedia.org/wiki/XXTEA
static void _crypt_function(bool sending)
{
#define DELTA 0x9E3779B9
#define MX (((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + (crypt_key[(uint8_t)((p & 3) ^ e)] ^ z)))

    uint32_t y, z, sum;
    // #pragma GCC diagnostic push
    // #pragma GCC diagnostic ignored "-Wcast-align"
    volatile uint32_t *v = &rf12_buf_u32[1];
    // #pragma GCC diagnostic pop
    uint8_t p, e, rounds = 6;

    if(sending)
    {
        ++seqNum;
        // memcpy(rf12_data + rf12_len, &seqNum, sizeof(seqNum));
        for(uint8_t i = 0; i < sizeof(seqNum); i++)
            rf12_data[rf12_len + i] = *(((uint8_t *)&seqNum) + i);
        uint8_t pad = 3 - (rf12_len & 3);
        rf12_len += pad;
        rf12_data[rf12_len] &= 0x3F;
        rf12_data[rf12_len] |= pad << 6;
        ++rf12_len;
        // actual encoding
        char n = rf12_len / 4;
        if(n > 1)
        {
            sum = 0;
            z = v[n - 1];
            do
            {
                sum += DELTA;
                e = (sum >> 2) & 3;
                for(p = 0; p < n - 1; p++)
                    y = v[p + 1], z = v[p] += MX;
                y = v[0];
                z = v[n - 1] += MX;
            } while(--rounds);
        }
    }
    else if(rf12_crc == 0)
    {
        // actual decoding
        char n = rf12_len / 4;
        if(n > 1)
        {
            sum = rounds * DELTA;
            y = v[0];
            do
            {
                e = (sum >> 2) & 3;
                for(p = n - 1; p > 0; p--)
                    z = v[p - 1], y = v[p] -= MX;
                z = v[n - 1];
                y = v[0] -= MX;
            } while((sum -= DELTA) != 0);
        }
        // strip sequence number from the end again
        if(n > 0)
        {
            uint8_t pad = rf12_data[--rf12_len] >> 6;
            rf12_seq = rf12_data[rf12_len] & 0x3F;
            while(pad-- > 0)
                rf12_seq = (rf12_seq << 8) | rf12_data[--rf12_len];
        }
    }
}

static bool can_send(void)
{
    // no need to test with interrupts disabled: state TXRECV is only reached
    // outside of ISR and we don't care if rxfill jumps from 0 to 1 here
    if(rxstate == TXRECV && rxfill == 0 && (rfm12b_trx_1b(0x00) & (RFM12B_RSSI_BIT >> 8)) == 0)
    {
        rfm12b_trx_2b(RFM12B_IDLE_MODE); // stop receiver
        //XXX just in case, don't know whether these RF12 reads are needed!
        // rf12_XFER(0x0000); // status register
        // rf12_XFER(RFM12B_RX_FIFO_READ); // fifo read
        rxstate = TXIDLE;
        return true;
    }
    return false;
}

static void send_start(uint8_t to_node_id, const void *data, uint8_t data_len, bool request_ack, bool sendACK, bool is_sleep)
{
    rf12_len = data_len;
    // #pragma GCC diagnostic push
    // #pragma GCC diagnostic ignored "-Wcast-qual"
    // memcpy((void *)rf12_data, data, data_len);
    for(uint8_t i = 0; i < data_len; i++)
        *(rf12_data + i) = *(((const uint8_t *)data) + i);
    // #pragma GCC diagnostic pop

    rf12_hdr1 = to_node_id | (sendACK ? RF12_HDR_ACKCTLMASK : 0);
    rf12_hdr2 = OWN_ID | (request_ack ? RF12_HDR_ACKCTLMASK : 0);
    if(crypter != 0) crypter(true);
    rf12_crc = 0xFFFF;
    rf12_crc = _crc16_update(rf12_crc, networkID);
    rxstate = TXPRE1;
    irq_fired = false;
    rfm12b_trx_2b(RFM12B_XMITTER_ON); // bytes will be fed via interrupts

    // wait for packet to actually finish sending
    // go into low power mode, as interrupts are going to come in very soon
    while(rxstate != TXIDLE)
    {
        if(is_sleep)
        {
            uint32_t to = HAL_GetTick() + 500;
            while(irq_fired == false)
            {
                asm("nop");
                if(to < HAL_GetTick())
                {
                    debug("STUCK @0");
                    to = HAL_GetTick() + 500;
                }
            }
        }
    }
}

/* =================================================================
*       PUBLIC FUNCTIONS
* ==================================================================*/
void rfm12b_encrypt(const uint8_t *key, uint8_t key_len)
{
    // by using a pointer to CryptFunction, we only link it in when actually used
    if(key != 0)
    {
        for(uint8_t i = 0; i < key_len; ++i)
            ((uint8_t *)crypt_key)[i] = key[i];
        crypter = _crypt_function;
    }
    else
        crypter = 0;
}

void rfm12b_reset(void)
{
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);

    rfm12b_trx_2b(0); // intitial SPI transfer added to avoid power-up problem

    rfm12b_trx_2b(RFM12B_SLEEP_MODE); // DC (disable clk pin), enable lbd

    // wait until RFM12B is out of power-up reset, this takes several *seconds*
    rfm12b_trx_2b(RFM12B_TXREG_WRITE); // in case we're still in OOK mode

    while(PIN_GET(RFM_IRQ) == 0)
        rfm12b_trx_2b(0);

    DELAY_US(150000);

    __HAL_GPIO_EXTI_CLEAR_IT(RFM_IRQ_Pin);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void rfm12b_config(uint8_t sync_pattern)
{
    networkID = sync_pattern;

    rfm12b_trx_2b(RFM12B_CMD_CONFIG |
                  (1 << 7) | /* internal data reg enable */
                  (1 << 6) | /* FIFO mode enable */
                  RFM12B_FREQ_BAND_433 |
                  RFM12B_CRYSTAL_LOAD_12PF);

    rfm12b_trx_2b(RFM12B_CMD_POWER |
                  (0 << 7) | /* rx enable */
                  (0 << 6) | /* rx baseband can be switched on */
                  (0 << 5) | /* tx enable */
                  (1 << 4) | /* synthesizer enable */
                  (1 << 3) | /* crystal enable */
                  (0 << 2) | /* low bat enable */
                  (0 << 1) | /* wake up timer */
                  (1 << 0)   /* disable clock out @pin8 */
    );

    rfm12b_trx_2b(RFM12B_CMD_FREQ |
                  1600 /* exact freq: 96 to 3903 */
    );

    rfm12b_trx_2b(RFM12B_CMD_DATA_RATE |
                  0x08 /* 344.827Kbps / (R+1), ~38.31Kbps */
    );

    rfm12b_trx_2b(RFM12B_CMD_RX_CTRL |
                  (1 << 10) | /* pin16 - valid data indicator */
                  RFM12B_VDI_FAST |
                  RFM12B_RX_BANDWIDTH_134KHZ |
                  RFM12B_LNA_GAIN_0DB |
                  RFM12B_RSSI_THSH_MINUS_94);

    rfm12b_trx_2b(RFM12B_CMD_DATA_FILTER |
                  (1 << 7) | /* auto clock recovery */
                  (0 << 6) | /* clock recovery lock control */
                  (0 << 4) | /* digital filter */
                  RFM12B_DQD_4);

    rfm12b_trx_2b(RFM12B_CMD_FIFO_AND_RESET |
                  (8 << 4) | /* FIFO level: 1-15  FIFO gen IT when the number of rx bits reaches this level */
                  (0 << 3) | /* sync pattern - 2 byte - 0x2Dxx */
                  (0 << 0) | /* sync patter fifo start cond */
                  (1 << 1) | /* fifo enable after sync pattern rx */
                  (1 << 0)   /* non-sensitive to power reset */
    );

    rfm12b_trx_2b(RFM12B_CMD_SYNC_PATTERN | sync_pattern);

    rfm12b_trx_2b(RFM12B_CMD_AFC |
                  (2 << 6) | /* keed Foffset only in rx (VDI=high) */
                  (0 << 4) | /* rt01 - no restriction */
                  (0 << 3) | /* strobe edge */
                  (0 << 2) | /* not high-accuracy mode */
                  (1 << 1) | /* en freq offset register */
                  (1 << 0)   /* en calc offset freq by AFC */
    );

    rfm12b_trx_2b(RFM12B_CMD_TX_CONFIG |
                  (5 << 4) | /* FSK out freq deviation - 90kHz */
                  RFM12B_POWER_OUT_0DB);

    rfm12b_trx_2b(RF12B_WAKEUP_TIMER);
    rfm12b_trx_2b(0xC800); /* low duty cycle */

    rfm12b_trx_2b(RFM12B_CMD_MCU_CLK_DIV |
                  (2 << 5) | /* CLK OUT freq - 1.66 MHz */
                  (0 << 0)   /* Vlb = 2.25 + V * 0.1V */
    );

    DELAY_US(150000);
}

void rfm12b_init(uint8_t sync_pattern)
{
    RFM_SPI->CR1 |= SPI_CR1_SPE; // enable SPI
    PIN_SET(RFM_CS);

    rfm12b_reset();
    rfm12b_config(sync_pattern);
}

void rfm12b_sleep(void)
{
    rfm12b_trx_2b(RF12B_WAKEUP_TIMER | 0x0500 | 0);
    rfm12b_trx_2b(RFM12B_SLEEP_MODE);
}

void rfm12b_wakeup(void)
{
    rfm12b_trx_2b(RFM12B_IDLE_MODE);
}

bool rfm12b_rx_complete(void)
{
    if(rxstate == TXRECV && (rxfill >= (uint8_t)(rf12_len + 6) || rxfill >= RF_MAX))
    {
        rxstate = TXIDLE;
        if(rf12_len > RFM12B_MAXDATA)
            rf12_crc = 1; // force bad crc if packet length is invalid
        // if(RF12_DESTID == 0 || RF12_DESTID == OWN_ID)
        { //if (!(rf12_hdr & RF12_HDR_DST) || (OWN_ID & NODE_ID) == 31 || (rf12_hdr & RF12_HDR_MASK) == (OWN_ID & NODE_ID)) {
            if(rf12_crc == 0 && crypter != 0)
                crypter(false);
            else
                rf12_seq = -1;
            return true; // it's a broadcast packet or it's addressed to this node
        }
    }
    if(rxstate == TXIDLE)
    {
        // start receive
        rxfill = rf12_len = 0;
        rf12_crc = 0xFFFF;
        if(networkID != 0)
            rf12_crc = _crc16_update(0xFFFF, networkID);
        rxstate = TXRECV;
        rfm12b_trx_2b(RFM12B_RECEIVER_ON);
    }
    return false;
}

void rfm12b_send(uint8_t to_node_id, const void *data, uint8_t data_len, bool request_ack, bool is_sleep)
{
    uint32_t to = HAL_GetTick() + 500;

    while(!can_send())
    {
        rfm12b_rx_complete();
        if(to < HAL_GetTick())
        {
            debug("Stuck @1");
            to = HAL_GetTick() + 500;
        }
    }

    send_start(to_node_id, data, data_len, request_ack, false, is_sleep);
}

void rfm12b_sendACK(const void *data, uint8_t data_len, bool is_sleep)
{
    uint32_t to = HAL_GetTick() + 500;

    while(!can_send())
    {
        rfm12b_rx_complete();
        if(to < HAL_GetTick())
        {
            debug("Stuck @2");
            to = HAL_GetTick() + 500;
        }
    }

    send_start(RF12_SOURCEID, data, data_len, false, true, is_sleep);
}

bool rfm12b_is_ack_received(uint8_t drom_node_id)
{
    if(rfm12b_rx_complete())
        return rfm12b_is_crc_pass() &&
               RF12_DESTID == OWN_ID &&
               (RF12_SOURCEID == drom_node_id || drom_node_id == 0) &&
               (rf12_hdr1 & RF12_HDR_ACKCTLMASK) &&
               !(rf12_hdr2 & RF12_HDR_ACKCTLMASK);
    return false;
}

void rfm12b_irq_handler(void)
{
    // a transfer of 2x 16 bits @ 2 MHz over SPI takes 2x 8 us inside this ISR
    // correction: now takes 2 + 8 µs, since sending can be done at 8 MHz
    rfm12b_trx_2b(0x0000);

    if(rxstate == TXRECV)
    {
        uint8_t in = rfm12b_trx_2b(RFM12B_RX_FIFO_READ); /* note: MAX SPI freq = 2.5MHZ here*/

        if(rxfill == 0 && networkID != 0)
            rf12_buf[rxfill++] = networkID;

        //Serial.print(out, HEX); Serial.print(' ');
        rf12_buf[rxfill++] = in;
        rf12_crc = _crc16_update(rf12_crc, in);

        if(rxfill >= (uint8_t)(rf12_len + 6) || rxfill >= RF_MAX)
            rfm12b_trx_2b(RFM12B_IDLE_MODE);
    }
    else
    {
        uint8_t out;

        if(rxstate < 0)
        {
            uint8_t pos = 4 + rf12_len + rxstate++;
            out = rf12_buf[pos];
            rf12_crc = _crc16_update(rf12_crc, out);
        }
        else
            switch(rxstate++)
            {
            case TXSYN1: out = 0x2D; break;
            case TXSYN2:
                out = networkID;
                rxstate = -(3 + rf12_len);
                break;
            case TXCRC1: out = rf12_crc; break;
            case TXCRC2: out = rf12_crc >> 8; break;
            case TXDONE: rfm12b_trx_2b(RFM12B_IDLE_MODE); // fall through
            default: out = 0xAA;
            }

        //Serial.print(out, HEX); Serial.print(' ');
        rfm12b_trx_2b(RFM12B_TXREG_WRITE + out);
    }

    irq_fired = true;
}
