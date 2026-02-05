/*============================================================================*/
/* Register Map                                                               */
/*============================================================================*/

/* Configuration Registers */
#define NRF24_REG_CONFIG        0x00
#define NRF24_REG_EN_AA         0x01
#define NRF24_REG_EN_RXADDR     0x02
#define NRF24_REG_SETUP_AW      0x03
#define NRF24_REG_SETUP_RETR    0x04
#define NRF24_REG_RF_CH         0x05
#define NRF24_REG_RF_SETUP      0x06
#define NRF24_REG_STATUS        0x07

/* RX/TX Addresses */
#define NRF24_REG_RX_ADDR_P0    0x0A
#define NRF24_REG_TX_ADDR       0x10

/* Payload Width */
#define NRF24_REG_RX_PW_P0      0x11

/* FIFO Status */
#define NRF24_REG_FIFO_STATUS   0x17

/* Commands */
#define NRF24_CMD_R_REGISTER    0x00
#define NRF24_CMD_W_REGISTER    0x20
#define NRF24_CMD_R_RX_PAYLOAD  0x61
#define NRF24_CMD_W_TX_PAYLOAD  0xA0
#define NRF24_CMD_FLUSH_TX      0xE1
#define NRF24_CMD_FLUSH_RX      0xE2
#define NRF24_CMD_NOP           0xFF

/* CONFIG register bits */
#define NRF24_CONFIG_PRIM_RX    (1 << 0)
#define NRF24_CONFIG_PWR_UP     (1 << 1)
#define NRF24_CONFIG_CRC_EN     (1 << 3)

/* STATUS register bits */
#define NRF24_STATUS_RX_DR      (1 << 6)
#define NRF24_STATUS_TX_DS      (1 << 5)
#define NRF24_STATUS_MAX_RT     (1 << 4)

/* RF_SETUP register positions */
#define NRF24_RF_SETUP_DR_LOW   5
#define NRF24_RF_SETUP_DR_HIGH  3
#define NRF24_RF_SETUP_PWR      1