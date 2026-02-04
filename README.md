## Overview

This is a **protocol-only library** for building reliable RC radio links. It defines packet formats, handles encoding/decoding, validates data integrity, and tracks link health. 

## Features

### Packet Structure
**32-byte packets** optimized for nRF24L01+ hardware:

```
┌─────────────┬──────────────────────┬────────┐
│   Header    │       Payload        │  CRC   │
│   5 bytes   │     0-26 bytes       │ 1 byte │
└─────────────┴──────────────────────┴────────┘
     ↓                  ↓                ↓
  Metadata         Actual data       Validation
```
**wire format**:
```
[Header: 5 bytes] [Payload: 0-26 bytes] [CRC: 1 byte]
└─ version (1)
└─ type (1)
└─ sequence (1)
└─ flags (1)
└─ payload_len (1)
                  └─ application data      └─ CRC-8-CCITT
```
### Packet Types
- Command Packets
- Telemetry Packets
- Heartbeat Packets

### Protocol Features
- **CRC-8 validation** on every packet
- **Sequence number tracking** for detecting lost/duplicate packets
- **Version negotiation** for future compatibility
- **Configurable timeouts** and link loss thresholds

## Installation

### As a Git Submodule

**In your ground station / aircraft firmware repo:**
```bash
cd your-firmware-repo
git submodule add https://github.com/fpv-chicken-nugget/nrf-rc-link.git lib/nrf-rc-link
git submodule update --init --recursive
```

## API Reference

### Packet Encoding/Decoding

```c
// generic payload encoding
void rc_encode_payload(rc_packet_t *pkt, rc_packet_type_t type,
                       const void *payload, uint8_t payload_len, uint8_t seq);

// generic payload decoding
bool rc_decode_payload(const rc_packet_t *pkt, void *payload, uint8_t expected_len);

// empty packets
void rc_encode_heartbeat(rc_packet_t *pkt, uint8_t seq);

// low-level packet operations
bool rc_packet_validate(const rc_packet_t *pkt);
uint8_t rc_packet_size(const rc_packet_t *pkt);
```

### Link Management

```c
// initialize link state
void rc_link_init(rc_link_state_t *link, uint32_t current_time_ms);

// update link state
void rc_link_update(rc_link_state_t *link, uint32_t current_time_ms);

// mark successful packet reception
void rc_link_mark_received(rc_link_state_t *link, uint8_t seq, uint32_t current_time_ms);

// mark packet error
void rc_link_mark_error(rc_link_state_t *link);

// query link status
bool rc_link_is_active(const rc_link_state_t *link);
uint32_t rc_link_time_since_rx(const rc_link_state_t *link, uint32_t current_time_ms);
```
## Configuration

Edit `include/nrf-rc-link/config.h` to adjust payload sizes:

```c
// payload size (default: 26 bytes)
#define RC_PAYLOAD_SIZE 26

// link timing
#define RC_PACKET_TIMEOUT_MS 100      // link timeout
#define RC_LINK_LOSS_THRESHOLD 10     // packets missed before link lost
```

**Important:** Header (5) + Payload + CRC (1) ≤ 32 bytes (nRF24L01 hardware limit)
## Quick Start

### Ground Station Example

```c
#include "nrf-rc-link/protocol.h"
#include "your_nrf24_driver.h"

// define semantic mapping
#define STICK_AILERON  0
#define STICK_ELEVATOR 1
#define STICK_THROTTLE 2
#define STICK_RUDDER   3
#define SWITCH_ARM     0
#define SWITCH_RTH     2

rc_link_state_t link;
rc_packet_t tx_packet;
uint16_t channels[RC_NUM_ANALOG_CHANNELS];
uint8_t switches = 0;

void setup() {
    rc_link_init(&link, HAL_GetTick());
    
    // set safe defaults
    for (int i = 0; i < RC_NUM_ANALOG_CHANNELS; i++) {
        channels[i] = 1024;
    }
    channels[STICK_THROTTLE] = 0;
    
    nrf24_init();
    nrf24_set_channel(76);  // 2476 MHz
}

void loop() {
    // read hardware
    channels[STICK_AILERON] = read_adc(ADC_CH0);
    channels[STICK_THROTTLE] = read_adc(ADC_CH1);
    
    if (read_gpio(PIN_ARM_SWITCH)) {
        switches |= (1 << SWITCH_ARM);
    }
    
    // protocol encodes provided payload
    rc_encode_command(&tx_packet, channels, switches, link.tx_seq++);
    nrf24_transmit(&tx_packet, rc_packet_size(&tx_packet));
    
    // handle telemetry reception
    if (nrf24_data_ready()) {
        rc_packet_t rx_packet;
        nrf24_receive(&rx_packet, sizeof(rx_packet));
        
        rc_telemetry_payload_t telem;
        if (rc_decode_telemetry(&rx_packet, &telem)) {
            rc_link_mark_received(&link, rx_packet.header.sequence, HAL_GetTick());
            display_telemetry(&telem);  // Your code
        }
    }
    
    rc_link_update(&link, HAL_GetTick());
}
```

### Aircraft Example

```c
#include "nrf-rc-link/protocol.h"
#include "your_nrf24_driver.h"

#define STICK_THROTTLE 2
#define SWITCH_ARM     0

rc_link_state_t link;
rc_packet_t rx_packet, tx_packet;
uint16_t failsafe_channels[RC_NUM_ANALOG_CHANNELS] = {1024, 1024, 0, 1024, /*...*/};

void loop() {
    if (nrf24_data_ready()) {
        nrf24_receive(&rx_packet, sizeof(rx_packet));
        
        uint16_t channels[RC_NUM_ANALOG_CHANNELS];
        uint8_t switches;
        
        if (rc_is_emergency(&rx_packet)) {
            engage_rth();
        } else if (rc_decode_command(&rx_packet, channels, &switches)) {
            rc_link_mark_received(&link, rx_packet.header.sequence, HAL_GetTick());
            
            // application logic
            if (switches & (1 << SWITCH_ARM)) {
                set_motor_throttle(channels[STICK_THROTTLE]);
            }
        }
    }
    
    rc_link_update(&link, HAL_GetTick());
    
    // failsafe policy
    if (!rc_link_is_active(&link)) {
        apply_failsafe(failsafe_channels);
    }
    
    // send telemetry
    rc_telemetry_payload_t telem;
    telem.data_i32[0] = get_gps_latitude();
    telem.data_u16[0] = get_battery_mv();
    rc_encode_telemetry(&tx_packet, &telem, link.tx_seq++);
    nrf24_transmit(&tx_packet, rc_packet_size(&tx_packet));
}
```
## Support

- **Issues:** Report bugs or request features on GitHub
- **Datasheet:** See nRF24L01+ datasheet for radio configuration
- **Compatibility:** Tested on STM32F103, should work on any ARM Cortex-M
