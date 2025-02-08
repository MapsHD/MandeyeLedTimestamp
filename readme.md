** Note ** use this feature in Mandeye controller : https://github.com/JanuszBedkowski/mandeye_controller/pull/33
# UcCode


The firmware can be run on Wemos D1 Mini board with WS2012 RGB leds.
The LED signal is connected to D4 pin of the Wemos Board.

## Compilation

To compile and upload code to Wemos D1 Mini board you need PlatformIO.
[PlatformIO](https://docs.platformio.org/en/latest/what-is-platformio.html) is toolset for embedded systems. 

To install PlatformIO simply:

```shell
pip install -U platformio
cd UcCode
pio run --target upload --upload-port /dev/ttyUSB0
```

## Commands to send to Wemos

Fimware expects following data at its USB-serial port at baudrate 115200:
```json
{"brightness":1,"active":4278190335,"inactive":4294901760,"ledState":0}
```
Where:
 - brightness is number 0-255
 - active is a color for 1-bits
 - inactive is a color for 0-bits
 
**Note ** colors are unsigned integer where every 2 byte word represents colors (red, green, blue, alpha).
Color can be obtained as:
```cpp
uint32_t ConvertColorToUc(const std::array<uint8_t,3>& color){
    return uint32_t(0xff000000) | (color[0] << 16) | (color[1] << 8) | color[2];
}
```

# MandeyeTimeStampToLed App

This executable should be deployed to Mandeye device.
It listens to ZMQ status that is published everysecond by Mandeye Controller, interpolates it and sends command to Wemos every one-tenth of the seconds, updating displayed timestamp.

This code can be compiled:
```shell
mkdir -p TimestampToLed/build
cd TimestampToLed/build
cmake .. && make
```

It can be run as process, to do that simply create another file in


Service installation: Create a file `/usr/lib/systemd/system/mandeye_leds.service` with content. Note that you need to adjust your user's name:
```
[Unit]
Description=Mandeye_LedTimestamp
After=multi-user.target

[Service]
User=pi
StandardOutput=null
StandardError=null
ExecStartPre=/bin/sleep 20
ExecStart=/home/pi/MandeyeLedTimestamp/TimestampToLed/build/TimestampToLed 
Restart=always

[Install]
WantedBy=multi-user.target
```

Next reload daemons, enable and start the service`

```shell
sudo systemctl daemon-reload
sudo systemctl enable mandeye_leds.service
sudo systemctl start mandeye_leds.service
```
You can check status of the service with:
```shell
sudo systemctl status mandeye_leds.service
```
