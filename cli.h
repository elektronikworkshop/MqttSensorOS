#pragma once
#include "config.h"

#include <OneWire.h>
#include <DallasTemperature.h>
extern DallasTemperature sensors;
extern bool sensorTrigger;

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
extern Adafruit_BME680 bme;
extern bool bmeFound;

class Cli
  : public CliMqttClient<FlashSettingsFinal,
                           2, /* _NumCommandSets    */
                          48, /* _MaxCommands       */
                         128, /* _CommandBufferSize */
                           8> /* _MaxCommandSize    */
{
private:
  
public:
  Cli(Stream& stream,
      FlashSettingsFinal &flashSettings,
      NetworkManager &networkManager,
      char eolChar = '\n',
      const char* prompt = NULL)
    : CliMqttClient(stream, flashSettings, networkManager, eolChar, prompt)
  {
    /* WARNING: Due to the static nature of StreamCmd any overflow of the
     * command list will go unnoticed, since this object is initialized in
     * global scope.
     */

    addCommand("s.", &Cli::cmdHelp);
    addCommand("s.scan", &Cli::cmdSensorScan);
    addCommand("s.info", &Cli::cmdSensorInfo);
    addCommand("s.set", &Cli::cmdSensorSet);
    addCommand("s.trig", &Cli::cmdSensorTrigger);
  }
  
protected:

  virtual void cmdHelp()
  {
    const char* arg = current();

    if (strncmp(arg, "s.", 2) == 0) {
      stream() << helpSensors();
    } else if (strlen(arg) == 0) {
      stream()
        << helpGeneral()
        << "NETWORK\n"
        << helpNetwork()
        << "MQTT\n"
        << helpMqtt()
        << "SENSORS\n"
        << helpSensors()
        ;
    } else {
      CliMqttClient::cmdHelp();
    }
  }
  const char * helpSensors()
  {
    return
      "s.scan\n"
      "  scan for sensors\n"
      "s.info\n"
      "  display sensor configuration\n"
      "s.trig\n"
      "  trigger sensor readout (even if interval is not expired)\n"
      "s.set interval [seconds]\n"
      "s.set  ds address <idx> <address>\n"
      "          topic   <idx> <topic>\n"
      "          pin <pin number>\n"
      "s.set dht pin <pin number>\n"
      "          type {dht11|dht12|dht21|dht22|am2301}\n"
      "          ttopic <topic>\n"
      "          htopic <topic>\n"
      "s.set bme base <topic>\n"
      "          temp <name>\n"
      "          pressure <name>\n"
      "          humidity <name>\n"
      "          gas-res <name>\n"
      ;
  }
  
  void cmdSensorScan(void)
  {
    stream() << "Scanning for sensors ... ";
    
    auto N = sensors.getDeviceCount();
    
    stream() << N << " found" << (N ? ":" : ".") << "\n";
    
    for (uint8_t i = 0; i < N; i++) {
      uint8_t address[8];
      sensors.getAddress(address, i);
      stream() << "  " << i << ": ";
      printHex(address, 8) << "\n";
    }
  }

  void cmdSensorInfo(void)
  {
    auto &fs = flashSettings;
    stream()
      << "DS18D20:\n"
      << "  1-Wire pin: " << fs.oneWirePinNumber << "\n";
      for (uint8_t s = 0; s < MaxSensors; s++) {
      auto &e = fs.sensorEntries[s];
      stream()
      << "     address["<<s<<"]: " ; printHex(e.address, 8) << "\n"
      << "       topic["<<s<<"]: " << (strlen(e.topic) ? e.topic : "<none>") << "\n";
      }
    if (fs.dhtPinNumber != DhtPinOff) {
    stream()
      << "DHT:\n"
      << "         pin: " << fs.dhtPinNumber << "\n"
      << "        type: " << (fs.dhtType == DHT11  ? "DHT11"  :
                              fs.dhtType == DHT12  ? "DHT12"  :
                              fs.dhtType == DHT21  ? "DHT21"  :
                              fs.dhtType == DHT22  ? "DHT22"  :
                              fs.dhtType == AM2301 ? "AM2301" :
                                                     "<invalid>") << "\n"
      << "  temp topic: " << (strlen(fs.dhtMqttHumidityTopic) ? fs.dhtMqttHumidityTopic : "<not set>") << "\n"
      ;
    } else {
    stream()
      << "DHT: <off>\n";
    }
    if (bmeFound) {
    stream()
      << "BME680:\n"
      << "  base topic: " << (strlen(fs.bmeMqttBaseTopic        ) ? fs.bmeMqttBaseTopic         : "<not set>") << "\n" 
      << "   temp name: " << (strlen(fs.bmeMqttTempName         ) ? fs.bmeMqttTempName          : "<not set>") << "\n" 
      << "  press name: " << (strlen(fs.bmeMqttPressureName     ) ? fs.bmeMqttPressureName      : "<not set>") << "\n" 
      << "  humid name: " << (strlen(fs.bmeMqttHumidityName     ) ? fs.bmeMqttHumidityName      : "<not set>") << "\n"
      << " gas-rs name: " << (strlen(fs.bmeMqttGasResistanceName) ? fs.bmeMqttGasResistanceName : "<not set>") << "\n"
      ;
    } else {
    stream()
      << "BME680: <not attached>\n";
      ;
    }
    stream()
      << "----\n"
      << "    interval: " << fs.measurementIntervalSeconds << " seconds\n"
      ;
  }
  
  void cmdSensorSet(void)
  {
    size_t sensorType(0);
    enum {DS = 0, DHT, BME, INTERVAL};
    switch (getOpt(sensorType, "ds", "dht", "bme", "interval")) {
      case ArgOk:
        switch (sensorType)
        {
        case DS:
          cmdDsSet();
          break;
        case DHT:
          cmdDhtSet();
          break;
        case BME:
          cmdBmeSet();
          break;
        case INTERVAL:
          cmdSensorInterval();
          break;
        }
        break;
      case ArgNone:
        return;
      default: // invalid arg
        stream() << "invalid device - should be one of {ds|dht|bme}\n";
        return;
    }
  }

  void cmdDhtSet(void)
  {
    size_t operation(0);
    enum {PIN = 0, TYPE, TTOPIC, HTOPIC};
    switch (getOpt(operation, "pin", "type", "ttopic", "htopic")) {
      case ArgOk:
        break;
      case ArgNone:
        return;
      default: // invalid arg
        return;
    }

    switch (operation) {
      case PIN:
          setGpioPin(flashSettings.dhtPinNumber);
          return;
      case TYPE: {
        size_t dhttype(0);
        enum {T_DHT11 = 0, T_DHT12, T_DHT21, T_DHT22, T_AM2301};
        switch (getOpt(dhttype, "dht11", "dht12", "dht21", "dht22", "am2301")) {
          case ArgOk:
            break;
          case ArgNone:
            return;
          default: // invalid arg
            return;
        }
        flashSettings.dhtType = dhttype == T_DHT11 ? DHT11 :
                                dhttype == T_DHT12 ? DHT12 :
                                dhttype == T_DHT21 ? DHT21 :
                                dhttype == T_DHT22 ? DHT22 :
                                                    AM2301 ;
        break;
      }
      case TTOPIC:
      case HTOPIC:
      {
        auto *t = operation == HTOPIC ? flashSettings.dhtMqttHumidityTopic : flashSettings.dhtMqttTemperatureTopic;
        setFlashStringFromArg(t, MaxMqttTopicLen, "topic");
        break;
      }
    }
  }

  void cmdBmeSet(void)
  {
    size_t op(0);
    enum {BASE = 0, TEMP, PRESSURE, HUMIDITY, GASRES};
    if (getOpt(op, "base", "temp", "pressure", "humidity", "gas-res") != ArgOk) {
      stream() << "invalid field - muste be one of {base|temp|pressure|humidity|gas-res}\n";
      return;
    }
    auto &fs = flashSettings;
    switch (op)
    {
    case BASE:
      setFlashStringFromArg(fs.bmeMqttBaseTopic, MaxMqttTopicLen, "topic");
      break;
    case TEMP:
      setFlashStringFromArg(fs.bmeMqttTempName, MaxMqttNameLen, "name");
      break;
    case PRESSURE:
      setFlashStringFromArg(fs.bmeMqttPressureName, MaxMqttNameLen, "name");
      break;
    case HUMIDITY:
      setFlashStringFromArg(fs.bmeMqttHumidityName, MaxMqttNameLen, "name");
      break;
    case GASRES:
      setFlashStringFromArg(fs.bmeMqttGasResistanceName, MaxMqttNameLen, "name");
      break;
    }
  }

  void cmdDsSet(void)
  {
    size_t operation(0);
    enum {ADDR = 0, TOPIC, PIN};
    if (getOpt(operation, "address", "topic", "pin") != ArgOk) {
        stream() << "invalid or no configuration field - should be one of {address|topic|pin}\n";
        return;
    }

    if (operation == PIN) {
      setGpioPin(flashSettings.oneWirePinNumber);
      return;
    }

    unsigned int sensorIdx(0);
    if (getUInt(sensorIdx, 0, MaxSensors - 1) != ArgOk) {
        stream() << "invalid or no sensor index allowed range: 0 .. " << MaxSensors-1 << "\n";
        return;
    }

    if (operation == ADDR)
    {
      uint8_t address[8] = {0};
      for (uint8_t i = 0; i < 8; i++) {
        unsigned int a(0);
        switch (getUInt(a, 0, 255, /* base */ 16)) {
          case ArgOk:
            address[i] = a;
            break;
          default: // invalid arg
            stream() << "invalid or incomplete sensor address - required is: AA BB CC DD EE FF GG HH\n";
            return;
        }
      }
      memcpy(flashSettings.sensorEntries[sensorIdx].address, address, 8);
      flashSettings.update();
      stream() << "address [" << sensorIdx << "] set to ";
      printHex(address, 8) << "\n";
    }
    if (operation == TOPIC) {
      setFlashStringFromArg(flashSettings.sensorEntries[sensorIdx].topic, MaxMqttTopicLen, "topic");
    }
  }

  void cmdSensorInterval(void)
  {
    unsigned int seconds(0);
    switch (getUInt(seconds, 0, UINT_MAX)) {
      case ArgOk:
        break;
      case ArgNone:
        stream() << "Current measurement interval: " << flashSettings.measurementIntervalSeconds << " seconds\n";
        return;
      default: // invalid arg
        stream() << "invalid number\n";
        return;
    }
    flashSettings.measurementIntervalSeconds = seconds;
    flashSettings.update();
    stream() << "Measurement interval set to " << flashSettings.measurementIntervalSeconds << " seconds\n";    
  }

  void cmdSensorTrigger(void)
  {
    sensorTrigger = true;
  }

  bool setGpioPin(uint8_t &pinMemberFlash)
  {
    unsigned int idx(0);
    switch (getUInt(idx, 0, SENSOR_PIN_IDX_MAX)) {
      case ArgOk:
        {
          auto gpio = sensor_pin_idx2gpio(idx);
          if (gpio < 0) {
            stream() << "something strange happened. idx: " << idx << ", gpio: " << gpio << "\n";
            return false;
          }
          pinMemberFlash = sensor_pin_idx2gpio(idx);
          flashSettings.update();
          stream() << "pin set to idx " << idx << " (gpio " << gpio << ")\n";
        }
        break;
      case ArgNone:
        stream() << "current pin: " << sensor_pin_gpio2idx(pinMemberFlash) << " (gpio " << pinMemberFlash << ")\n";
        return false;
      default: // invalid arg
        stream() << "invalid pin index - allowed [0 .. " << SENSOR_PIN_IDX_MAX << "]\n";
        return false;
    }
    return true;
  }
};

Cli uartCli(Serial, flashSettings, network);


#include <TelnetServer.h>

class TelnetCli
  : public TelnetClient
  , public Cli
{
public:
  enum CommandSet
  {
    Authenticated = 0,
    NotAuthenticated,
  };
  TelnetCli()
    : Cli(getStream() /* returns TelnetStreamProxy */, flashSettings, network, '\r', flashSettings.hostName)
  {
    switchCommandSet(Authenticated);
    
    /* Add quit command to command set 0 */
    addCommand("quit",   &TelnetCli::cmdQuit);

    switchCommandSet(NotAuthenticated);

    setDefaultHandler(&TelnetCli::auth);
  }

private:
  void auth(const char* password)
  {
    if (strcmp(flashSettings.telnetPass, password) == 0) {
      switchCommandSet(0);
      return;
    }
    getStream() << "authentication failed -- wrong password\n";
    reset();
  }
  virtual void cmdHelp()
  {
    Cli::cmdHelp();
    getStream()
      << "quit\n"
      << "  quit telnet session and close connection\n";
  }
  void cmdQuit()
  {
    reset();
  }
  void begin(const WiFiClient& client)
  {
    if (isConnected()) {
      return;
    }

    TelnetClient::begin(client);

    /* Log before we add client to the logger proxies */
    Debug << "telnet client connection (" << getClient().remoteIP().toString() << ")\n";

    auto prterr = [](const char* who)
    {
      Log << "failed to add telnet stream proxy to " << who << " logger proxy\n";
    };
    if (not Log.addClient(getStream())) {
      prterr("default");
    }
    if (not Debug.addClient(getStream())) {
      prterr("debug");
    }
    
    /* make sure the new client must authenticate first */
    switchCommandSet(NotAuthenticated);

    getStream()
      << WelcomeMessage("telnet")
      << "Please enter password for \"" << flashSettings.hostName << "\": ";
      ;
  }
  /* Attention: possible conflict with StreamCmd's reset() member */
  virtual void reset()
  {
    if (not isConnected()) {
      return;
    }

    getStream() << "bye\n";
    
    auto prterr = [](const char* who)
    {
      Log << "failed to remove telnet stream proxy from " << who << " logger proxy\n";
    };
    if (not Log.removeClient(getStream())) {
      prterr("default");
    }
    if (not Debug.removeClient(getStream())) {
      prterr("debug");
    }

    if (getClient()) {
      Debug << "telnet connection closed (" << getClient().remoteIP().toString() << ")\n";
    } else {
      Debug<< "telnet connection closed (unknown IP)\n";
    }

    TelnetClient::reset();
  }
  virtual void processStreamData()
  {
    Cli::run();
  }
};

TelnetCli telnetClis[MaxTelnetClients];
