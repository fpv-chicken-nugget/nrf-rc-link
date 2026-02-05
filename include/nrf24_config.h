/**
* @file nrf24_config.h
 * @brief nRF24L01+ hardware configuration
 *
 * Configure SPI peripheral and GPIO pins for your STM32.
 * Change according to your application's hardware connections.
 */

#ifndef NRF24_CONFIG_H
#define NRF24_CONFIG_H

#include "../drivers/include/stm32f1xx_hal.h"  // change to your STM32 family

/*============================================================================*/
/* SPI Configuration                                                          */
/*============================================================================*/

/** SPI peripheral handle */
#define NRF24_SPI_HANDLE        hspi1

/** SPI timeout values in ms */
#define NRF24_SPI_TIMEOUT       100

/*============================================================================*/
/* GPIO Configuration                                                         */
/*============================================================================*/

/** CSN (Chip Select Not) pin */
#define NRF24_CSN_PORT          GPIOB
#define NRF24_CSN_PIN           SPI1_CSN_NRF_Pin

/** CE (Chip Enable) pin */
#define NRF24_CE_PORT           GPIOB
#define NRF24_CE_PIN            NRF_CE_Pin

/*============================================================================*/
/* Timing Configuration                                                       */
/*============================================================================*/

/** Timer for microsecond delays */
#define NRF24_TIM_HANDLE        htim3

/** System tick function for millisecond timing */
#define NRF24_GET_TICK_MS()     HAL_GetTick()

/*============================================================================*/
/* External Declarations                                                      */
/*============================================================================*/

extern SPI_HandleTypeDef NRF24_SPI_HANDLE;
extern TIM_HandleTypeDef NRF24_TIM_HANDLE;

#endif /* NRF24_CONFIG_H */