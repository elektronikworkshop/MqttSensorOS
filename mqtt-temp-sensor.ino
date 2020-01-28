/*
 * https://www.instructables.com/id/Remote-Temperature-Monitoring-Using-MQTT-and-ESP82/
 * https://github.com/PaulStoffregen/OneWire
 * https://github.com/milesburton/Arduino-Temperature-Control-Library
 * https://github.com/knolleary/pubsubclient
 */
/** Required libraries:
 *  
 * Github:
 *   FlashSettings
 *   StreamCmd
 *   TelnetServer
 *
 * Library manager:
 *   OneWire
 *   NTP Client
 *   ThingSpeak
 * 
 */

#include "cli.h"

LogProxy<MaxTelnetClients> Log;
LogProxy<MaxTelnetClients> Debug;

FlashSettingsFinal flashSettings;
NetworkManager network(Log, flashSettings, telnetClis, MaxTelnetClients);

OneWire oneWire(DefaultOneWirePinNumber);
DallasTemperature sensors(&oneWire);
bool sensorTrigger(false);

Adafruit_BME680 bme;
bool bmeFound = false;

void setup()
{
  Serial.begin(115200);
  Serial.println("");
  PrintVersion(Serial);

  flashSettings.begin();
  
  Debug.enable(flashSettings.debug);
  Debug << "number of registered commands: " << uartCli.getNumCommandsRegistered(0) << "\n";

  network.begin();

  /* Note: Setting the pin in CLI requires calling this again... */
  oneWire.begin(flashSettings.oneWirePinNumber);
  sensors.begin();

  Serial << WelcomeMessage("serial");

  if (!bme.begin()) {
    Serial << "No BME680 sensor found\n";
  } else {
    bmeFound = true;
    // Set up oversampling and filter initialization
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320*C for 150 ms
  }
}

void readDs(void)
{
  sensors.setResolution(12);
  sensors.requestTemperatures(); // Send the command to get temperatures

  auto &mqttClient = network.getMqttClient();

  auto N = sensors.getDeviceCount();
  for (size_t didx = 0; didx < N; didx++) {
    bool found(false);
    uint8_t address[8];
    sensors.getAddress(address, didx);
    
    for (size_t sidx = 0; sidx < MaxSensors; sidx++) {
      auto &entry = flashSettings.sensorEntries[sidx];
      if (memcmp(address, entry.address, 8) == 0) {
        if (strlen(entry.topic)) {
          found = true;
          float t = sensors.getTempC(address);
          Log << "Temperature [" << sidx << ", " ; Cli::printHex(Log, address, 8) << "]: " << t << "\n";

          if (mqttClient.connected()) {
            mqttClient.publish(entry.topic, String(t).c_str(), true);
          }
        }
        continue;
      }
    }

    if (not found) {
      // sensor address not configured
      Log << "sensor address " ; Cli::printHex(Log, address, 8) << " not configured\n";
    }
  }
}

void readDht(void)
{
  if (flashSettings.dhtPinNumber == DhtPinOff) {
    return;
  }

  /* Note: This way the DHT library will not realise if we read too fast. 
   * Between readouts we should wait at least 2 seconds. The measurement
   * interval should be configured like that.
   */
  DHT dht = DHT(flashSettings.dhtPinNumber,
                flashSettings.dhtType);
  dht.begin();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Log << "failed to read from DHT sensor - is pin configured correctly?\n";
    return;
  }

  Log << "DHT: " << h << "% " << t << "C\n";

  auto &mqttClient = network.getMqttClient();
  if (not mqttClient.connected()) {
    return;
  }

  if (strlen(flashSettings.dhtMqttHumidityTopic)) {
    mqttClient.publish(flashSettings.dhtMqttHumidityTopic, String(h).c_str(), true);
  }
  if (strlen(flashSettings.dhtMqttTemperatureTopic)) {
    mqttClient.publish(flashSettings.dhtMqttTemperatureTopic, String(t).c_str(), true);
  }
}

void readBme(void)
{
  if (not bmeFound) {
    return;
  }
  if (not bme.performReading()) {
    return;
  }

  auto t = bme.temperature;             // C
  auto p = bme.pressure / 100.0;        // hPa
  auto h = bme.humidity;                // %
  auto r = bme.gas_resistance / 1000.0; // kOhms

  Log << "BME: " << t << "C " << h << "% " << p << "hPa " << r << "kOhms\n";

  auto &fs = flashSettings;
  auto &mqttClient = network.getMqttClient();
  if (not mqttClient.connected() or
      not strlen(flashSettings.bmeMqttBaseTopic))
  {
    return;
  }

  String base(fs.bmeMqttBaseTopic);

  if (strlen(fs.bmeMqttTempName)) {
    String topic = base + "/" + fs.bmeMqttTempName;
    mqttClient.publish(topic.c_str(), String(t).c_str(), true);
  }
  if (strlen(fs.bmeMqttPressureName)) {
    String topic = base + "/" + fs.bmeMqttPressureName;
    mqttClient.publish(topic.c_str(), String(p).c_str(), true);
  }
  if (strlen(fs.bmeMqttHumidityName)) {
    String topic = base + "/" + fs.bmeMqttHumidityName;
    mqttClient.publish(topic.c_str(), String(h).c_str(), true);
  }
  if (strlen(fs.bmeMqttGasResistanceName)) {
    String topic = base + "/" + fs.bmeMqttGasResistanceName;
    mqttClient.publish(topic.c_str(), String(r).c_str(), true);
  }
}

void loop()
{
  uartCli.run();
  network.run();

  unsigned long delta = flashSettings.measurementIntervalSeconds * 1000UL;
  unsigned long now = millis();
  static unsigned long prev = 0;
  if (now - prev >= delta or sensorTrigger) {
    sensorTrigger = false;
    prev = now;

    readDs();
    readDht();
    readBme();
  }
}
