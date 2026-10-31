# UniFi device-presence RGB monitor

Standalone ESP32-S3 firmware that polls a local UniFi Network controller and
shows whether a configured device is currently associated with the network.
It is designed for the common ESP32-S3 Super Mini with a 4 MB flash / 2 MB
PSRAM configuration and an onboard WS2812-style RGB LED on GPIO 48.

## LED states

The RGB LED is intentionally dimmed for normal indoor use:

- off: booting
- amber: Wi-Fi disconnected or connecting
- purple: querying UniFi
- green: device present
- red: device away

The separate blue status LED near the USB connector on some Super Mini boards
is board hardware and is not controlled by this firmware.

## Requirements

- ESP32-S3 Super Mini compatible with the `esp32-s3-devkitc1-n4r2` PlatformIO
  board profile
- USB data cable
- PlatformIO Core or PlatformIO in VS Code
- A local UniFi Network controller reachable over HTTPS
- A UniFi API key with permission to read client/device state

## Installation

1. Install PlatformIO and clone this repository.
2. Copy the example credentials file:

   ```sh
   cp credentials.txt.example credentials.txt
   ```

3. Edit `credentials.txt` with the Wi-Fi, device, API, and controller values.
   The Wi-Fi line contains the SSID and password as two whitespace-separated
   values. The other lines contain one value each:

   ```text
   Wifi YOUR_WIFI_SSID YOUR_WIFI_PASSWORD
   Device AA:BB:CC:DD:EE:FF
   API YOUR_UNIFI_API_KEY
   Unifi 192.168.1.1
   ```

4. Connect the ESP32-S3 by USB and build/upload it:

   ```sh
   pio run -t upload
   ```

   PlatformIO auto-detects the USB serial/JTAG port. To view diagnostic
   messages, run:

   ```sh
   pio device monitor --baud 115200
   ```

The build-time script converts `credentials.txt` into the ignored
`include/secrets.h` file. Do not commit either file. The generated header is
not required in a clean checkout before building.

## UniFi API behavior

The firmware queries the default UniFi site at:

```text
/proxy/network/api/s/default/stat/sta/<device-mac>
```

An HTTP 200 response with a non-empty `data` array is treated as present; an
empty array or request failure is treated as away/error for the LED display.
The API key is sent in the `X-API-Key` header and is never printed. HTTPS
certificate verification is disabled because local controllers commonly use
self-signed certificates; use this only on a trusted local network.

## Configuration

- Poll interval: 30 seconds (`POLL_INTERVAL_MS` in `src/main.cpp`)
- UniFi request timeout: 5 seconds (`HTTP_TIMEOUT_MS`)
- RGB LED pin: GPIO 48 (`LED_PIN`)

If your board revision uses a different RGB LED pin, update `LED_PIN` before
building. The separate power/status LED cannot be controlled through this
setting.

## Security

Keep `credentials.txt`, `include/secrets.h`, and any API-key backups private.
The repository ignore rules exclude the real credential file and generated
header; `credentials.txt.example` contains dummy values only.
