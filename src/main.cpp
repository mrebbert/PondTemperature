#include <Arduino.h>
#include <ConnectionManager.h> 
#include <MqttClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DoubleResetDetect.h>

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings

// maximum number of seconds between resets that
// counts as a double reset 
#define DRD_TIMEOUT 2.0

// address to the block in the RTC user memory
// change it if it collides with another usage 
// of the address block
#define DRD_ADDRESS 0x00
DoubleResetDetect drd(DRD_TIMEOUT, DRD_ADDRESS);

#define DEBUG true
#define USE_SERIAL Serial

const int oneWireBus = 4;     

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

const char *configTopic               = "homeassistant/sensor/sensorPondT/config";
const char *attributesTopic           = "homeassistant/sensor/sensorPondT/attributes";
const char *availabilityTopic         = "homeassistant/sensor/sensorPondT/status";
const char *stateTopic                = "homeassistant/sensor/sensorPondT/state";

ConnectionManager connectionManager;
MqttClient        mqttClient;

char     hostname[32];

double reference_voltage = 3.3;
int bitrange = 1024;

double getBatteryVoltage(int adc) {
  return (adc * reference_voltage)/bitrange;
}

double getBatteryStatus(double voltage) {
  return (voltage*100/reference_voltage);
}

void setAvailability(boolean isAvailable) {
  
  const char *availability = "offline";
  if (isAvailable) {
    availability = "online";
  }
  
  if(!mqttClient.isConnected()) {
    if(!mqttClient.reconnect()) {
      if(DEBUG)
        USE_SERIAL.println("MQTT Connect failed.");
    } 
  }
  if(mqttClient.isConnected()) {
    mqttClient.publish(availabilityTopic, availability, false);
     if(DEBUG) {
      USE_SERIAL.printf("Published %s to availability topic", availability);
      USE_SERIAL.println();
    }
  }
}

void publishConfig() {
  if(!mqttClient.isConnected()) {
    if(!mqttClient.reconnect()) {
      if(DEBUG)
        USE_SERIAL.println("MQTT Connect failed.");
    } 
  }

  if(mqttClient.isConnected()) {
    
    JSONVar jconfig;
    jconfig["device_class"]           = "temperature";
    jconfig["name"]                   = "Pond Temperature";
    jconfig["object_id"]              = "pond_temperature";
    jconfig["state_topic"]            = stateTopic;
    jconfig["availability_topic"]     = availabilityTopic;
    jconfig["json_attributes_topic"]  = attributesTopic;
    jconfig["unit_of_measurement"]    = "°C";
    jconfig["value_template"]         = "{{ value_json.temperature}}";

    mqttClient.publish(configTopic, JSONVar::stringify(jconfig).c_str(), true);

    setAvailability(false);
    if(DEBUG) {
      USE_SERIAL.printf("Publishing: %s", JSONVar::stringify(jconfig).c_str());
      USE_SERIAL.println();
    }
  }
}

void publish(float temperature, double batteryVoltage, double batteryStatus) {

  if(!mqttClient.isConnected()) {
    if(!mqttClient.reconnect()) {
      if(DEBUG)
        USE_SERIAL.println("MQTT Connect failed.");
    } 
  }

  if(mqttClient.isConnected()) {

    JSONVar jpayload;
    jpayload["temperature"]     = temperature;
    mqttClient.publish(stateTopic, JSONVar::stringify(jpayload).c_str(), false);

    JSONVar jattributes;
    jattributes["battery_voltage"] = batteryVoltage;
    jattributes["battery_status"]  = batteryStatus;
    jattributes["RSSI"]            = WiFi.RSSI();
    mqttClient.publish(attributesTopic, JSONVar::stringify(jattributes).c_str(), false);

    setAvailability(true);
    if(DEBUG) {
      USE_SERIAL.printf("Published %.2f °", temperature);
      USE_SERIAL.println();
      USE_SERIAL.println(JSONVar::stringify(jpayload).c_str());
      USE_SERIAL.println(JSONVar::stringify(jattributes).c_str());
    }
  }
}

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  strcat(hostname, "MRT-PondTemperature-");
  strcat(hostname, String(ESP.getChipId()).c_str());

  USE_SERIAL.begin(115200);
  USE_SERIAL.setTimeout(2000);

  // Wait for serial to initialize.
  while(!USE_SERIAL) { }

  // Activate to reset all wifimanager settings
  // connectionManager.resetConfiguration();
  if (drd.detect() && ESP.getResetInfoPtr()->reason == 6) {
    USE_SERIAL.println("** Double reset boot **");
    connectionManager.resetConfiguration();
  }
  connectionManager.init();
 
  if(DEBUG) {
    USE_SERIAL.print("MQTT Server:    "); USE_SERIAL.println(connectionManager.config.mqtt_server);
    USE_SERIAL.print("MQTT Port:      "); USE_SERIAL.println(connectionManager.config.mqtt_port);
    USE_SERIAL.print("MQTT User:      "); USE_SERIAL.println(connectionManager.config.mqtt_user);
    // USE_SERIAL.print("MQTT Password:  "); USE_SERIAL.println(connectionManager.config.mqtt_password);
  }

  mqttClient.init(hostname, connectionManager.config.mqtt_server, connectionManager.config.mqtt_port, 
        connectionManager.config.mqtt_user, connectionManager.config.mqtt_password);

  delay(3000); // give some time to initialize...  
  
  publishConfig();
  sensors.begin();

  sensors.requestTemperatures(); 
  
  float  temperatureC   = sensors.getTempCByIndex(0);
  double batteryVoltage = getBatteryVoltage(analogRead(A0));
  double batteryStatus  = getBatteryStatus(batteryVoltage);
  
  if(temperatureC == -127.00) {
    setAvailability(false);
    USE_SERIAL.println("Failed to read from DS18B20 sensor");
  } else {
    publish (temperatureC, batteryVoltage, batteryStatus);
  }
  digitalWrite(LED_BUILTIN, LOW);

  if(DEBUG) {
    USE_SERIAL.printf("Temperature is: %.2f °C", temperatureC);
    USE_SERIAL.println();
    USE_SERIAL.printf("Battery Voltage is: %.2f V", batteryVoltage);
    USE_SERIAL.println();
    USE_SERIAL.printf("Battery Status is: %.2f %%", batteryStatus);
    USE_SERIAL.println();
  }

  delay(1000);
  if(DEBUG) {
    USE_SERIAL.println("Going to deep sleep for 5 minutes...");
  }
  setAvailability(false);
  ESP.deepSleep(30 * 60 * 1e6); 
  delay(5000);
}

void loop() {
  /*
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
  
    sensors.requestTemperatures(); 
    float temperatureC = sensors.getTempCByIndex(0);
    long batteryVoltage = getBatteryVoltage(analogRead(A0));


    if(temperatureC == -127.00) {
      setAvailability(false);
      USE_SERIAL.println("Failed to read from DS18B20 sensor");
    } else {
      publish (temperatureC, batteryVoltage);
    }
    digitalWrite(LED_BUILTIN, LOW);
    if(DEBUG) {
      USE_SERIAL.printf("Battery Voltage is: %lu mV", batteryVoltage);
      USE_SERIAL.println();
    }
  }
  */
}
