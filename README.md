# FPV RC Link Protocol Library

A lightweight, robust bidirectional RC link protocol for nRF24L01+ radios on STM32 microcontrollers. Designed for FPV aircraft with built-in failsafe mechanisms and telemetry downlink.

# Table of Contents

- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
  - [Components](#components)
  - [Connections](#connections)
- [Software Requirements](#software-requirements)
- [Quick Start](#quick-start)
  - [1. STM32CubeMX Configuration](#1-stm32cubemx-configuration)
  - [2. Add Library to Project](#2-add-library-to-project)
  - [3. Configure Hardware (`nrf24_config.h`)](#3-configure-hardware-nrf24_configh)
  - [4. Configure Protocol (`rc_config.h`)](#4-configure-protocol-configh)
  - [5. Ground Station Code](#5-ground-station-code)
  - [6. Aircraft Code](#6-aircraft-code)
- [Architecture](#architecture)
  - [Layer 1: nRF24 Hardware Driver](#layer-1-nrf24-hardware-driver)
  - [Layer 2: RC Protocol](#layer-2-rc-protocol)
  - [Layer 3: Application](#layer-3-application)
- [Protocol Packet Format](#protocol-packet-format)
- [Link Loss Detection](#link-loss-detection)
  - [1. Timeout-Based](#1-timeout-based)
  - [2. Sequence Gap Detection](#2-sequence-gap-detection)
- [API Reference](#api-reference)
  - [Initialization](#initialization)
  - [Ground Station Functions](#ground-station-functions)
  - [Aircraft Functions](#aircraft-functions)
  - [Common Functions](#common-functions)
  - [Status Codes](#status-codes)
- [Configuration Options](#configuration-options)
  - [RF Settings (`config.h`)](#rf-settings-configh)
  - [Timing Settings](#timing-settings)
  - [Features](#features-1)
- [RF Channel Selection](#rf-channel-selection)
- [Performance Characteristics](#performance-characteristics)
- [File Structure](#file-structure)
- [License](#license)

## Features

- **Bidirectional Communication** - Commands uplink, telemetry downlink
- **Automatic Failsafe** - Configurable safe values on link loss
- **Link Monitoring** - Timeout detection, sequence tracking, quality metrics
- **Data Integrity** - CRC-8 validation, protocol versioning
- **User-Configurable Payloads** - Define your own command/telemetry structures
- **Statistics Tracking** - Packet loss, link quality, error counts

## Hardware Requirements

### Components
- **STM32 microcontroller**
- **nRF24L01+PA module** (2.4 GHz transceiver)
- **SPI connection** (MOSI, MISO, SCK, CSN, CE)

### Connections
*Source is configured with default connections and can be changed in `nrf24_config.h` to suit your application.*
```
nRF24L01+          STM32
─────────────────────────
VCC        →       3.3V
GND        →       GND
CE         →       GPIO (configurable)
CSN        →       GPIO (configurable)
SCK        →       SPI_SCK
MOSI       →       SPI_MOSI
MISO       →       SPI_MISO
IRQ        →       (not used)
```

## Software Requirements

- **STM32CubeMX** or **STM32CubeIDE** - Peripheral configuration
- **STM32CubeIDE** or **Keil MDK** or **IAR** - Compilation
- **STM32 HAL Library** - Provided by STM

## Quick Start

### 1. STM32CubeMX Configuration

Configure the following in CubeMX:

**SPI:**
- Mode: Full-Duplex Master
- Speed: ≤ 8 Mbps
- Data size: 8 bits
- Clock polarity: Low
- Clock phase: 1 Edge

**GPIO:**
- CSN pin: Output (push-pull, high speed)
- CE pin: Output (push-pull, high speed)

**Timer (for microsecond delays):**
- Any timer (TIM1, TIM2, etc.)
- Clock source: Internal (or otherwise)
- **PSC (Prescaler)**: Set for 1 MHz (1µs ticks)
  - Example: 64 MHz clock → PSC = 63
  - Formula: PSC = (SystemClock / 1,000,000) - 1

### 2. Add Library to Project

Add the library as a git submodule or simply copy the files to your project. 

To add as a submodule:

After running
```bash
git submodule add https://github.com/fpv-chicken-nugget/nrf-rc-link.git Drivers/nrf-rc-link
git submodule update --init --recursive
```
<details>
<summary>**Add include paths in STM32CubeIDE**</summary>
<br>
In STM32CubeIDE:
1. Project → Properties
2. C/C++ General → Paths and Symbols → Includes
3. Select GNU C
4. Add the following include directories (workspace-relative or project-relative):
* `Drivers/nrf-rc-link/include`
* `Drivers/nrf-rc-link/drivers/include`
</details>

<details>
<summary>**Compile the library source files**</summary>
<br>
Ensure CubeIDE builds the library .c files:
- `Middlewares/nrf-rc-link/src/crc.c`
- `Middlewares/nrf-rc-link/src/nrf_rc_driver.c`
- `Middlewares/nrf-rc-link/drivers/nrf24.c`
You can do this by either:
- Adding the folders as Source Folders in the Project Explorer, or
- Linking/adding the `.c` files directly to your project so they are compiled
</details>

Alternatively, copy files directly into your project:

<details>
<summary>**Copy into your project**</summary>
<br>
In STM32CubeIDE:
Copy the following folders into your project (under `Drivers/`):
   - `include/`
   - `src/`
   - `drivers/`
   - `drivers/include/`

Ensure the `.c` files are part of the build:
   - `src/crc.c`
   - `src/nrf_rc_driver.c`
   - `drivers/nrf24.c`
</details>
<details>
<summary>**Add include paths in STM32CubeIDE**</summary>
<br>
In STM32CubeIDE:
1. Project → Properties
2. C/C++ General → Paths and Symbols → Includes
3. Select GNU C
4. Add the following include directories (workspace-relative or project-relative):
* `Drivers/nrf-rc-link/include`
* `Drivers/nrf-rc-link/drivers/include`
</details>

To use the library in your application, include the public header in your code:
```c
#include "nrf_rc_driver.h"
```

### 3. Configure Hardware (`nrf24_config.h`)

```c
// Set to match your STM32 family
#include "stm32f1xx_hal.h"  // Change to f4xx, f7xx, etc.

// SPI peripheral
#define NRF24_SPI_HANDLE        hspi1

// GPIO pins
#define NRF24_CSN_PORT          GPIOA
#define NRF24_CSN_PIN           GPIO_PIN_4

#define NRF24_CE_PORT           GPIOA
#define NRF24_CE_PIN            GPIO_PIN_3

// Timer for microsecond delays
#define NRF24_TIM_HANDLE        htim1
```

### 4. Configure Protocol (`config.h`)

**Customize your payload structures:**

```c
// Example: Simple 8-channel RC
typedef struct __attribute__((packed)) {
    uint16_t channels[8];   // 0-2047 (11-bit resolution)
    uint8_t switches;       // 8 binary switches
    uint8_t mode;           // Flight mode
} rc_command_payload_t;

typedef struct __attribute__((packed)) {
    int32_t gps_lat;        // Latitude * 1e7
    int32_t gps_lon;        // Longitude * 1e7
    uint16_t battery_mv;    // Battery voltage (mV)
    uint8_t gps_sats;       // Satellite count
    // Add your own fields (max 26 bytes total)
} rc_telemetry_payload_t;
```

**Configure RF settings:**

```c
#define RC_RF_CHANNEL               76    // 2476 MHz
#define RC_TX_POWER                 3     // 0dBm (max)
#define RC_DATA_RATE                2     // 2 Mbps
#define RC_UPDATE_RATE_HZ           50    // 50 Hz
#define RC_LINK_TIMEOUT_MS          1000  // 1 second
```

### 5. Ground Station Code

```c
#include "nrf_rc_driver.h"

rc_link_t rc_link;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_TIM1_Init();
    
    // Start timer for nRF24 delays
    HAL_TIM_Base_Start(&htim1);
    
    // Initialize RC link
    rc_hardware_config_t hw_config = {
        .get_tick_ms = HAL_GetTick
    };
    rc_link_init(&rc_link, &hw_config);
    
    uint32_t last_update = 0;
    uint32_t interval = 1000 / RC_UPDATE_RATE_HZ;  // 20ms at 50Hz
    
    while (1) {
        // Update state machine
        rc_link_update(&rc_link);
        
        // Send commands at configured rate
        if (HAL_GetTick() - last_update >= interval) {
            last_update = HAL_GetTick();
            
            // Read your joystick/controls
            rc_command_payload_t cmd;
            cmd.channels[0] = read_joystick_throttle();
            cmd.channels[1] = read_joystick_aileron();
            // ... set other channels
            
            rc_link_send_command(&rc_link, &cmd);
        }
        
        // Receive telemetry
        rc_telemetry_payload_t telem;
        if (rc_link_receive_telemetry(&rc_link, &telem) == RC_OK) {
            display_battery(telem.battery_mv);
            display_gps(telem.gps_lat, telem.gps_lon);
        }
        
        // Check link status
        if (!rc_link_is_active(&rc_link)) {
            display_warning("LINK LOST");
        }
    }
}
```

### 6. Aircraft Code

```c
#include "nrf_rc_driver.h"

rc_link_t rc_link;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI2_Init();
    MX_TIM1_Init();
    
    HAL_TIM_Base_Start(&htim1);
    
    // Initialize RC link
    rc_hardware_config_t hw_config = {
        .get_tick_ms = HAL_GetTick
    };
    rc_link_init(&rc_link, &hw_config);
    
    // Set failsafe values
    rc_command_payload_t failsafe = RC_FAILSAFE_COMMAND;
    rc_link_set_failsafe(&rc_link, &failsafe);
    
    uint32_t last_update = 0;
    uint32_t interval = 1000 / RC_UPDATE_RATE_HZ;
    
    while (1) {
        rc_link_update(&rc_link);
        
        // Receive commands (returns failsafe if link lost)
        rc_command_payload_t cmd;
        if (rc_link_receive_command(&rc_link, &cmd) == RC_OK) {
            set_throttle(cmd.channels[0]);
            set_aileron(cmd.channels[1]);
            // ... control servos/motors
        }
        
        // Check link and activate RTH if needed
        if (!rc_link_is_active(&rc_link)) {
            activate_return_to_home();
        }
        
        // Send telemetry at configured rate
        if (HAL_GetTick() - last_update >= interval) {
            last_update = HAL_GetTick();
            
            rc_telemetry_payload_t telem;
            telem.gps_lat = gps_get_latitude();
            telem.gps_lon = gps_get_longitude();
            telem.battery_mv = adc_read_battery();
            telem.gps_sats = gps_get_satellite_count();
            
            rc_link_send_telemetry(&rc_link, &telem);
        }
    }
}
```

## Architecture

### Layer 1: nRF24 Hardware Driver
- **Files:** `nrf24.h`, `nrf24.c`, `nrf24_config.h`
- **Purpose:** STM32-specific nRF24L01+ chip driver
- **Provides:** Register access, TX/RX control, data transfer

### Layer 2: RC Protocol
- **Files:** `nrf_rc_driver.h`, `nrf_rc_driver.c`, `packet.h`, `crc.h`, `crc.c`, `config.h`
- **Purpose:** Protocol logic, packet handling, link management
- **Provides:** High-level send/receive API, failsafe, statistics

### Layer 3: Application
- **Files:** `main.c` (application code)
- **Purpose:** Flight controller, ground station logic
- **Uses:** Simple API from Layer 2

## Protocol Packet Format

**32-byte packet structure:**

```
┌─────────────────┬──────────────────────┬────────┐
│ Header (5 bytes)│  Payload (26 bytes)  │CRC (1) │
└─────────────────┴──────────────────────┴────────┘

Header:
  - Version (1 byte)
  - Type (1 byte)      // Command, Telemetry, etc.
  - Sequence (1 byte)  // Wraps at 255
  - Flags (1 byte)     // Reserved
  - Length (1 byte)    // Payload length

Payload:
  - User-defined structure (max 26 bytes)

CRC:
  - CRC-8-CCITT checksum
```

## Link Loss Detection

The library uses **two mechanisms** to detect link loss:

### 1. Timeout-Based
- If no packet received within `RC_LINK_TIMEOUT_MS` (default: 1000ms)
- Link declared lost

### 2. Sequence Gap Detection
- Tracks sequence numbers in packet headers
- If `RC_LINK_LOSS_THRESHOLD` consecutive packets missed (default: 10)
- Link declared lost

**Link is lost if EITHER condition is met.**

## API Reference

### Initialization

```c
rc_status_t rc_link_init(rc_link_t *link, const rc_hardware_config_t *hw_config);
void rc_link_deinit(rc_link_t *link);
```

### Ground Station Functions

```c
// Send RC commands to aircraft
rc_status_t rc_link_send_command(rc_link_t *link, const rc_command_payload_t *command);

// Receive telemetry from aircraft
rc_status_t rc_link_receive_telemetry(rc_link_t *link, rc_telemetry_payload_t *telemetry);
```

### Aircraft Functions

```c
// Receive commands (returns failsafe if link lost)
rc_status_t rc_link_receive_command(rc_link_t *link, rc_command_payload_t *command);

// Send telemetry to ground
rc_status_t rc_link_send_telemetry(rc_link_t *link, const rc_telemetry_payload_t *telemetry);
```

### Common Functions

```c
// Update state machine (call in main loop)
rc_status_t rc_link_update(rc_link_t *link);

// Check link status
bool rc_link_is_active(rc_link_t *link);
uint32_t rc_link_get_time_since_rx(rc_link_t *link);

// Failsafe management
rc_status_t rc_link_set_failsafe(rc_link_t *link, const rc_command_payload_t *failsafe);
rc_status_t rc_link_get_failsafe(rc_link_t *link, rc_command_payload_t *failsafe);

// Statistics (if RC_ENABLE_STATISTICS = 1)
rc_status_t rc_link_get_stats(rc_link_t *link, rc_stats_t *stats);
void rc_link_reset_stats(rc_link_t *link);
```

### Status Codes

```c
RC_OK                    // Success
RC_ERROR_INVALID_PARAM   // Invalid parameter
RC_ERROR_TIMEOUT         // Operation timed out
RC_ERROR_NO_DATA         // No data available
RC_ERROR_CRC_FAIL        // CRC validation failed
RC_ERROR_VERSION_MISMATCH // Protocol version mismatch
RC_ERROR_HARDWARE        // Hardware/SPI error
RC_ERROR_NOT_INITIALIZED // Driver not initialized
```

## Configuration Options

### RF Settings (`config.h`)

```c
RC_RF_CHANNEL              // 0-125 (2400-2525 MHz, 1MHz steps)
RC_TX_POWER                // 0-3 (0=-18dBm, 3=0dBm)
RC_DATA_RATE               // 0=250k, 1=1M, 2=2M bps
RC_AUTO_RETRANSMIT_COUNT   // 0-15 retries
RC_AUTO_RETRANSMIT_DELAY   // 0-15 ((value+1)*250µs)
```

### Timing Settings

```c
RC_UPDATE_RATE_HZ          // Packet rate (default: 50 Hz)
RC_LINK_TIMEOUT_MS         // Link loss timeout (default: 1000 ms)
RC_LINK_LOSS_THRESHOLD     // Missed packet threshold (default: 10)
```

### Features

```c
RC_ENABLE_STATISTICS       // 1 = enable stats tracking
RC_ENABLE_LOGGING          // 1 = enable debug logging
```

## RF Channel Selection

**Recommended channels for 2 Mbps mode (2 MHz spacing):**
- Channel 2 (2402 MHz)
- Channel 26 (2426 MHz)
- Channel 50 (2450 MHz)
- Channel 76 (2476 MHz) ← default
- Channel 98 (2498 MHz)
- Channel 122 (2522 MHz)

**Avoid WiFi overlap:**
- WiFi Ch 1: 2412 MHz (±11 MHz) → avoid RC channels 7-17
- WiFi Ch 6: 2437 MHz (±11 MHz) → avoid RC channels 32-42
- WiFi Ch 11: 2462 MHz (±11 MHz) → avoid RC channels 57-67

## Performance Characteristics

| Parameter | Value |
|-----------|-------|
| **Max range** | ~1 km (line of sight, PA module, 0dBm) |
| **Latency** | ~20 ms (at 50 Hz update rate) |
| **Packet loss tolerance** | 10 consecutive packets (configurable) |
| **Max payload** | 26 bytes per direction |
| **Throughput** | ~13 kbps (26 bytes × 50 Hz × 2 directions) |

## File Structure

```
fpv-rc-protocol/
├── include/
│   ├── nrf24_config.h       # Hardware pin config (user edits)
│   ├── nrf24.h              # nRF24 driver interface
│   ├── rc_config.h          # Protocol config (user edits)
│   ├── rc_driver.h          # RC link API
│   ├── rc_packet.h          # Packet structures
│   └── rc_crc.h             # CRC interface
│
├── src/
│   ├── nrf24.c              # nRF24 driver implementation
│   ├── rc_driver.c          # RC link implementation
│   └── rc_crc.c             # CRC implementation
│
├── examples/
│   ├── main_ground.c        # Ground station example
│   └── main_aircraft.c      # Aircraft example
│
└── README.md                # This file
```

---

## License

This library is provided as-is for hobby use. Use at your own risk.
