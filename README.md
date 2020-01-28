# MqttSensorOS
An Arduino MQTT Sensor OS

This operating system allows you to set up a wireless temperature/humidity/pressure sensor node without having to edit a line of code. Everything can be configured and is stored in the internal flash.

The system currently supports the following sensors

* DS18B20 (multiple)
* BME680
* DHT11, DHT12
* DHT21, DHT22
* AM2301

The system can easily be adapted to support more/other sensors and peripherals.


## Features
* **Serial command line interface**
  To bootstrap your system
* **Telnet server**
  You can connect to your sensor board via telnet and tweak the settings. As soon as you have configured the WiFi SSID and the WiFi password via the serial monitor you are ready to go. The serial and telnet CLIs are the same (except for the quit command which terminates the telnet session)
* **WiFi roaming**

You want usually run this system on ESP8366 or ESP32 based hardware.

## Building the system
### Libraries
The following libraries can be installed from the *Library Manager*

* Adafruit Unified Sensor
* Adafruit BME680 Library
* DallasTemperature
* Adafruit DHT sensor library
* OneWire
* PubSubClient

The following libraries must be downloaded and installed manually from the *elektronikworkshop* GitHub page:

* EwUtil
* FlashSettings
* LogProxy
* MqttClient
* StreamCmd
* TelnetServer

Depending on your target platform you have to install the board packages for ESP32/ESP8266. Other platforms probably require some additions in the libraries - just let us know... 

## Setting up the System
### Network and MQTT-backend
After uploading connect with the serial monitor or similar. The set up your WiFi connection:
```terminal
n.ssid <your SSID>
n.pass <your WiFi password>
```
Now you're ready to connect via telnet:
```terminal
you@computer:~ > telnet mqtt-temp-sensor
```
The default telnet password is ```h4ckm3``` you should change this as soon as you're connected using the following command:
```terminal
mqtt-temp-sensor> n.telnet pass <new password>
```

To set up the MQTT backend run the following commands:
```terminal
mqtt-temp-sensor> m.server <yourmqttserver>
mqtt-temp-sensor> m.user <your mqtt user name>
mqtt-temp-sensor> m.pass <your mqtt user password>
mqtt-temp-sensor> m.client <name of this mqtt client>
```
Type
```terminal
mqtt-temp-sensor> n.
```
for help on the networking commands and
```terminal
mqtt-temp-sensor> m.
```
for the MQTT commands.

### Sensors


Currently the system is running in our labs on a variety of boards
* NodeMcu 1.0 (Amica)
* WeMos D1 mini pro
* WeMos D1 mini
* DOIT ESP32 DEVKIT V1

## Development
### TODO
* Support for non SI units (Fahrenheit etc.)
* Deep sleep via [Timer on 8266](https://randomnerdtutorials.com/esp8266-deep-sleep-with-arduino-ide/) or [ULP on ESP32](https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/) platforms

## A typical Command Line Session
```terminal
uli@tire-bouchon:~/Projects/chanet/software/mqtt-temp-sensor$ telnet mqtt-temp-0
Trying 192.168.1.46...
Connected to mqtt-temp-0.fritz.box.
Escape character is '^]'.
Welcome to the MQTT Temperature Sensor telnet interface!
Copyright (c) 2016-2020 Elektronik Workshop
Type "help" for available commands
Please enter password for "mqtt-temp-0": ******
mqtt-temp-0> s.info
DS18D20:
  1-Wire pin: 2
  Entry [0]
     address: 00 00 00 00 00 00 00 00
       topic: <none>
  Entry [1]
     address: 00 00 00 00 00 00 00 00
       topic: <none>
  Entry [2]
     address: 00 00 00 00 00 00 00 00
       topic: <none>
  Entry [3]
     address: 00 00 00 00 00 00 00 00
       topic: <none>
DHT: <off>
BME680:
  base topic: indoor/ba1/eg/stueva
   temp name: temperature
  press name: pressure
  humid name: humidity
 gas-rs name: gas-resistance
----
    interval: 60 seconds
mqtt-temp-0> s.
s.scan
  scan for sensors
s.info
  display sensor configuration
s.trig
  trigger sensor readout (even if interval is not expired)
s.set interval [seconds]
s.set <device> <device options>
s.set  ds address <idx> <address>
          topic   <idx> <topic>
          pin <pin number>
s.set dht pin <pin number>
          type {dht11|dht22}
          ttopic <topic>
          htopic <topic>
s.set bme base <topic>
          temp <name>
          pressure <name>
          humidity <name>
          gas-res <name>
mqtt-temp-0>  n.
n.rssi
  display the current connected network strength (RSSI)
n.list
  list the visible networks
n.ssid [ssid]
  with argument: set wifi SSID
  without: show current wifi SSID
n.pass [password]
  with argument: set wifi password
  without: show current wifi password
n.host [hostname]
  shows the current host name when invoked without argument
  sets a new host name when called with argument
n.connect
  connect to configured wifi network
n.telnet [params]
  without [params] this prints the telnet configuration
  with [params] the telnet server can be configured as follows
    on
      enables the telnet server
    off
      disables the telnet server
    pass <pass>
      sets the telnet login password to <password>
n.info
 print network setup info
mqtt-temp-0> n.info
MAC:              dc:4f:22:65:d3:2d
SSID:             Archas-199-Internal
host name:        mqtt-temp-0
MQTT server:      hp-ml330-g6.fritz.box
MQTT user:        archas
MQTT client name: mqtt-temp-00
WiFi:             connected
signal strength:  -52 dB
IP:               192.168.1.46
MQTT server connected
mqtt-temp-0> s.trig
BME: 18.02C 31.57% 863.84hPa 55.25kOhms
mqtt-temp-0>
```