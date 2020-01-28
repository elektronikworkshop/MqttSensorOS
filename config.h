#pragma once

#include <Arduino.h>

const unsigned int VersionMajor = 1;
const unsigned int VersionMinor = 0;
const unsigned int VersionRevision = 0;

const unsigned int MaxTelnetClients = 1;

#define DefaultHostName "mqtt-temp-sensor"

const uint8_t DefaultOneWirePinNumber  = D4;
const uint16_t DefaultMeasurementIntervalSeconds = 60 * 30;


// to be adjusted for non NodeMcu/WeMos MCUs
inline int
sensor_pin_gpio2idx(unsigned int gpio)
{
  switch (gpio) {
    case D0: return 0;
    case D1: return 1;
    case D2: return 2;
    case D3: return 3;
    case D4: return 4;
    case D5: return 5;
    case D6: return 6;
    case D7: return 7;
    case D8: return 8;
    default: return -1;
  }
}

const unsigned int SENSOR_PIN_IDX_MAX = 8;

inline int
sensor_pin_idx2gpio(unsigned int idx)
{
  // TODO: different LUTs for different architectures!
  const unsigned int lut[SENSOR_PIN_IDX_MAX + 1] = {D0, D1, D2, D3, D4, D5, D6, D7, D8};
  
  if (idx > SENSOR_PIN_IDX_MAX) {
    return -1; 
  }
  return lut[idx];
}


#include <DHT.h>

const uint8_t DhtPinOff = 0xFF;
const uint8_t DefaultDhtPinNumber = DhtPinOff;
const uint8_t DefaultDhtType = DHT11;


#include <EwUtil.h>
using namespace ew;

#define WelcomeMessage(what)                          \
  "Welcome to the MQTT Temperature Sensor " what " interface!\n" \
  "Copyright (c) 2016-2020 Elektronik Workshop\n"          \
  "Type \"help\" for available commands\n"

#define PrintVersion(stream) prtFmt(stream, "%02u:%02u:%02u", VersionMajor, VersionMinor, VersionRevision) << " -- " << __DATE__ << " " << __TIME__ << "\n"

#include <LogProxy.h>

/* we could back up the log buffers to flash, but that's probably overkill */

extern LogProxy<MaxTelnetClients> Log;
extern LogProxy<MaxTelnetClients> Debug;


#include <MqttClient.h>

const unsigned int MaxMqttTopicLen = 63; // excluding zero termination
const unsigned int MaxMqttNameLen = 24;
const unsigned int MaxSensors = 4;

struct TempSensorEntry
{
  /* OneWire address */
  uint8_t address[8];
  char topic[MaxMqttTopicLen+1];
};

struct FlashData
  : public FlashDataMqttClient
{  
  uint8_t oneWirePinNumber;
  uint16_t measurementIntervalSeconds;
  TempSensorEntry sensorEntries[MaxSensors];

  uint8_t dhtPinNumber;
  uint8_t dhtType;
  char dhtMqttHumidityTopic[MaxMqttTopicLen+1];
  char dhtMqttTemperatureTopic[MaxMqttTopicLen+1];

  char bmeMqttBaseTopic[MaxMqttTopicLen+1];
  char bmeMqttTempName[MaxMqttNameLen+1];
  char bmeMqttPressureName[MaxMqttNameLen+1];
  char bmeMqttHumidityName[MaxMqttNameLen+1];
  char bmeMqttGasResistanceName[MaxMqttNameLen+1];

  FlashData()
    : oneWirePinNumber(DefaultOneWirePinNumber)
    , measurementIntervalSeconds(DefaultMeasurementIntervalSeconds)
    , sensorEntries{{{0}, {0}}, {{0}, {0}}, {{0}, {0}}, {{0}, {0}}}
    , dhtPinNumber(DefaultDhtPinNumber)
    , dhtType(DefaultDhtType)
    , dhtMqttHumidityTopic{0}
    , dhtMqttTemperatureTopic{0}

    , bmeMqttBaseTopic{0}
    , bmeMqttTempName{"temperature"}
    , bmeMqttPressureName{"pressure"}
    , bmeMqttHumidityName{"humidity"}
    , bmeMqttGasResistanceName{"gas-resistance"}
  { }
};

const uint16_t FlashSettingsPartitionSize = 2 * 1024;
typedef FlashSettings<FlashData, FlashSettingsPartitionSize> FlashSettingsFinal;
extern FlashSettingsFinal flashSettings;

extern NetworkManager network;
