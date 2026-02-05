/**
 * @file nrf24.c
 * @brief nRF24L01+ driver implementation
 */

#include "include/nrf24.h"
#include "nrf24_config.h"
#include "include/nrf24_registers.h"
#include <string.h>

/*============================================================================*/
/* Private Function Prototypes                                                */
/*============================================================================*/

static void nrf24_csn_low(void);
static void nrf24_csn_high(void);
static void nrf24_ce_low(void);
static void nrf24_ce_high(void);
static void nrf24_delay_us(uint32_t us);
static void nrf24_write_register_multi(nrf24_t *nrf, uint8_t reg, const uint8_t *data, uint8_t len);

/*============================================================================*/
/* GPIO Control                                                               */
/*============================================================================*/

static inline void nrf24_csn_low(void)
{
    HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_RESET);
}

static inline void nrf24_csn_high(void)
{
    HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET);
}

static inline void nrf24_ce_low(void)
{
    HAL_GPIO_WritePin(NRF24_CE_PORT, NRF24_CE_PIN, GPIO_PIN_RESET);
}

static inline void nrf24_ce_high(void)
{
    HAL_GPIO_WritePin(NRF24_CE_PORT, NRF24_CE_PIN, GPIO_PIN_SET);
}

/*============================================================================*/
/* Timing                                                                     */
/*============================================================================*/

static void nrf24_delay_us(uint32_t us)
{
    /* Use DWT cycle counter for precise microsecond delays */
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }

    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000U);

    while ((DWT->CYCCNT - start) < cycles);
}

/*============================================================================*/
/* Low-Level Register Access                                                  */
/*============================================================================*/

uint8_t nrf24_read_register(nrf24_t *nrf, uint8_t reg)
{
    uint8_t tx_buf[2] = {NRF24_CMD_R_REGISTER | reg, 0xFF};
    uint8_t rx_buf[2] = {0};

    nrf24_csn_low();
    HAL_SPI_TransmitReceive(&NRF24_SPI_HANDLE, tx_buf, rx_buf, 2, NRF24_SPI_TIMEOUT);
    nrf24_csn_high();

    return rx_buf[1];
}

void nrf24_write_register(nrf24_t *nrf, uint8_t reg, uint8_t value)
{
    uint8_t tx_buf[2] = {NRF24_CMD_W_REGISTER | reg, value};

    nrf24_csn_low();
    HAL_SPI_Transmit(&NRF24_SPI_HANDLE, tx_buf, 2, NRF24_SPI_TIMEOUT);
    nrf24_csn_high();
}

static void nrf24_write_register_multi(nrf24_t *nrf, uint8_t reg, const uint8_t *data, uint8_t len)
{
    uint8_t cmd = NRF24_CMD_W_REGISTER | reg;

    nrf24_csn_low();
    HAL_SPI_Transmit(&NRF24_SPI_HANDLE, &cmd, 1, NRF24_SPI_TIMEOUT);
    HAL_SPI_Transmit(&NRF24_SPI_HANDLE, (uint8_t *)data, len, NRF24_SPI_TIMEOUT);
    nrf24_csn_high();
}

uint8_t nrf24_get_status(nrf24_t *nrf)
{
    uint8_t cmd = NRF24_CMD_NOP;
    uint8_t status = 0;

    nrf24_csn_low();
    HAL_SPI_TransmitReceive(&NRF24_SPI_HANDLE, &cmd, &status, 1, NRF24_SPI_TIMEOUT);
    nrf24_csn_high();

    return status;
}

void nrf24_clear_interrupts(nrf24_t *nrf)
{
    /* Write 1 to clear interrupt flags */
    uint8_t clear = NRF24_STATUS_RX_DR | NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT;
    nrf24_write_register(nrf, NRF24_REG_STATUS, clear);
}

void nrf24_flush_tx(nrf24_t *nrf)
{
    uint8_t cmd = NRF24_CMD_FLUSH_TX;

    nrf24_csn_low();
    HAL_SPI_Transmit(&NRF24_SPI_HANDLE, &cmd, 1, NRF24_SPI_TIMEOUT);
    nrf24_csn_high();
}

void nrf24_flush_rx(nrf24_t *nrf)
{
    uint8_t cmd = NRF24_CMD_FLUSH_RX;

    nrf24_csn_low();
    HAL_SPI_Transmit(&NRF24_SPI_HANDLE, &cmd, 1, NRF24_SPI_TIMEOUT);
    nrf24_csn_high();
}

/*============================================================================*/
/* Initialization & Configuration                                             */
/*============================================================================*/

bool nrf24_init(nrf24_t *nrf, uint8_t channel, uint8_t payload_size)
{
    if (!nrf || payload_size == 0 || payload_size > 32 || channel > 125) {
        return false;
    }

    /* Clear handle */
    memset(nrf, 0, sizeof(nrf24_t));

    nrf->channel = channel;
    nrf->payload_size = payload_size;
    nrf->is_rx_mode = false;

    /* Ensure CE is low (standby) */
    nrf24_ce_low();
    nrf24_csn_high();

    /* Wait for power-on reset */
    HAL_Delay(5);

    /* Power down first */
    nrf24_write_register(nrf, NRF24_REG_CONFIG, 0x00);
    nrf24_delay_us(1500);

    /* Set RF channel */
    nrf24_write_register(nrf, NRF24_REG_RF_CH, channel);

    /* Set data rate to 2Mbps and TX power to 0dBm */
    nrf24_set_data_rate(nrf, NRF24_DATA_RATE_2MBPS);
    nrf24_set_tx_power(nrf, NRF24_TX_POWER_0DBM);

    /* Set address width to 5 bytes */
    nrf24_write_register(nrf, NRF24_REG_SETUP_AW, 0x03);

    /* Enable auto-ACK on pipe 0 */
    nrf24_write_register(nrf, NRF24_REG_EN_AA, 0x01);

    /* Enable RX pipe 0 */
    nrf24_write_register(nrf, NRF24_REG_EN_RXADDR, 0x01);

    /* Set auto-retransmit: 500µs delay, 3 retries */
    nrf24_set_auto_retransmit(nrf, 1, 3);

    /* Set RX payload width */
    nrf24_write_register(nrf, NRF24_REG_RX_PW_P0, payload_size);

    /* Set default addresses */
    uint8_t addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    nrf24_set_addresses(nrf, addr, addr);

    /* Clear status flags */
    nrf24_clear_interrupts(nrf);

    /* Flush FIFOs */
    nrf24_flush_tx(nrf);
    nrf24_flush_rx(nrf);

    /* Power up in RX mode with CRC enabled (8-bit) */
    uint8_t config = NRF24_CONFIG_PWR_UP | NRF24_CONFIG_CRC_EN | NRF24_CONFIG_PRIM_RX;
    nrf24_write_register(nrf, NRF24_REG_CONFIG, config);
    nrf->is_rx_mode = true;

    nrf24_delay_us(1500);  /* Wait for power-up */

    nrf->initialized = true;

    return true;
}

void nrf24_set_channel(nrf24_t *nrf, uint8_t channel)
{
    if (!nrf || channel > 125) {
        return;
    }

    nrf->channel = channel;
    nrf24_write_register(nrf, NRF24_REG_RF_CH, channel);
}

void nrf24_set_tx_power(nrf24_t *nrf, nrf24_tx_power_t power)
{
    if (!nrf) {
        return;
    }

    uint8_t rf_setup = nrf24_read_register(nrf, NRF24_REG_RF_SETUP);
    rf_setup &= ~(0x06);  /* Clear power bits */
    rf_setup |= (power << NRF24_RF_SETUP_PWR);
    nrf24_write_register(nrf, NRF24_REG_RF_SETUP, rf_setup);
}

void nrf24_set_data_rate(nrf24_t *nrf, nrf24_data_rate_t rate)
{
    if (!nrf) {
        return;
    }

    uint8_t rf_setup = nrf24_read_register(nrf, NRF24_REG_RF_SETUP);
    rf_setup &= ~((1 << NRF24_RF_SETUP_DR_LOW) | (1 << NRF24_RF_SETUP_DR_HIGH));

    switch (rate) {
        case NRF24_DATA_RATE_250KBPS:
            rf_setup |= (1 << NRF24_RF_SETUP_DR_LOW);
            break;
        case NRF24_DATA_RATE_1MBPS:
            /* Bits already cleared */
            break;
        case NRF24_DATA_RATE_2MBPS:
            rf_setup |= (1 << NRF24_RF_SETUP_DR_HIGH);
            break;
    }

    nrf24_write_register(nrf, NRF24_REG_RF_SETUP, rf_setup);
}

void nrf24_set_addresses(nrf24_t *nrf, const uint8_t *tx_addr, const uint8_t *rx_addr)
{
    if (!nrf || !tx_addr || !rx_addr) {
        return;
    }

    nrf24_write_register_multi(nrf, NRF24_REG_TX_ADDR, tx_addr, 5);
    nrf24_write_register_multi(nrf, NRF24_REG_RX_ADDR_P0, rx_addr, 5);
}

void nrf24_set_auto_retransmit(nrf24_t *nrf, uint8_t delay, uint8_t count)
{
    if (!nrf) {
        return;
    }

    uint8_t setup_retr = ((delay & 0x0F) << 4) | (count & 0x0F);
    nrf24_write_register(nrf, NRF24_REG_SETUP_RETR, setup_retr);
}

/*============================================================================*/
/* Mode Control                                                               */
/*============================================================================*/

void nrf24_mode_tx(nrf24_t *nrf)
{
    if (!nrf || !nrf->is_rx_mode) {
        return;  /* Already in TX mode */
    }

    nrf24_ce_low();

    uint8_t config = nrf24_read_register(nrf, NRF24_REG_CONFIG);
    config &= ~NRF24_CONFIG_PRIM_RX;  /* Clear RX bit for TX mode */
    nrf24_write_register(nrf, NRF24_REG_CONFIG, config);

    nrf24_delay_us(130);  /* Tpd2stby + Tstby2a */

    nrf->is_rx_mode = false;
}

void nrf24_mode_rx(nrf24_t *nrf)
{
    if (!nrf || nrf->is_rx_mode) {
        return;  /* Already in RX mode */
    }

    nrf24_ce_low();

    uint8_t config = nrf24_read_register(nrf, NRF24_REG_CONFIG);
    config |= NRF24_CONFIG_PRIM_RX;  /* Set RX bit */
    nrf24_write_register(nrf, NRF24_REG_CONFIG, config);

    nrf24_ce_high();  /* Start listening */
    nrf24_delay_us(130);  /* Tpd2stby + Tstby2a */

    nrf->is_rx_mode = true;
}

void nrf24_power_down(nrf24_t *nrf)
{
    if (!nrf) {
        return;
    }

    nrf24_ce_low();

    uint8_t config = nrf24_read_register(nrf, NRF24_REG_CONFIG);
    config &= ~NRF24_CONFIG_PWR_UP;
    nrf24_write_register(nrf, NRF24_REG_CONFIG, config);

    nrf->initialized = false;
}

/*============================================================================*/
/* Data Transfer                                                              */
/*============================================================================*/

bool nrf24_transmit(nrf24_t *nrf, const uint8_t *data, uint8_t len)
{
    if (!nrf || !data || len != nrf->payload_size) {
        return false;
    }

    /* Switch to TX mode */
    nrf24_mode_tx(nrf);

    /* Write payload */
    uint8_t cmd = NRF24_CMD_W_TX_PAYLOAD;
    nrf24_csn_low();
    HAL_SPI_Transmit(&NRF24_SPI_HANDLE, &cmd, 1, NRF24_SPI_TIMEOUT);
    HAL_SPI_Transmit(&NRF24_SPI_HANDLE, (uint8_t *)data, len, NRF24_SPI_TIMEOUT);
    nrf24_csn_high();

    /* Pulse CE to start transmission */
    nrf24_ce_high();
    nrf24_delay_us(15);  /* Minimum 10µs pulse */
    nrf24_ce_low();

    /* Wait for TX complete or max retries (with timeout) */
    uint32_t start = NRF24_GET_TICK_MS();
    while ((NRF24_GET_TICK_MS() - start) < 10) {  /* 10ms timeout */
        uint8_t status = nrf24_get_status(nrf);

        if (status & NRF24_STATUS_TX_DS) {
            /* TX successful */
            nrf24_clear_interrupts(nrf);
            return true;
        }

        if (status & NRF24_STATUS_MAX_RT) {
            /* Max retries reached */
            nrf24_clear_interrupts(nrf);
            nrf24_flush_tx(nrf);
            return false;
        }
    }

    /* Timeout */
    nrf24_flush_tx(nrf);
    return false;
}

bool nrf24_receive(nrf24_t *nrf, uint8_t *buffer, uint8_t *len)
{
    if (!nrf || !buffer || !len) {
        return false;
    }

    /* Switch to RX mode */
    nrf24_mode_rx(nrf);

    /* Check if data available */
    uint8_t status = nrf24_get_status(nrf);
    if (!(status & NRF24_STATUS_RX_DR)) {
        return false;
    }

    /* Read payload */
    uint8_t cmd = NRF24_CMD_R_RX_PAYLOAD;
    nrf24_csn_low();
    HAL_SPI_Transmit(&NRF24_SPI_HANDLE, &cmd, 1, NRF24_SPI_TIMEOUT);
    HAL_SPI_Receive(&NRF24_SPI_HANDLE, buffer, nrf->payload_size, NRF24_SPI_TIMEOUT);
    nrf24_csn_high();

    *len = nrf->payload_size;

    /* Clear RX interrupt */
    nrf24_clear_interrupts(nrf);

    return true;
}

bool nrf24_is_data_available(nrf24_t *nrf)
{
    if (!nrf) {
        return false;
    }

    uint8_t status = nrf24_get_status(nrf);
    return (status & NRF24_STATUS_RX_DR) != 0;
}