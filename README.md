# ESP-Temperature Sensor for Ponds or Pools using Home Assistant and MQTT Integration

## TL;DR
A sketch for an ESP 8266 based micro controller and an DS18B20 temperature sensor to measure the temperature of a pond or pool using Home Assistant and MQTT.

![](images/schema_pond_temperature.png)

## The Software
### Sketch

### MQTT (Discovery)

#### Topics and Messages

##### Configuration (for MQTT Discovery)
"homeassistant/sensor/sensorPondT/config"
```` json
{
 "device_class":"temperature",
 "name":"Pond Temperature",
 "object_id":"pond_temperature",
 "state_topic":"homeassistant/sensor/sensorPondT/state",
 "availability_topic":"homeassistant/sensor/sensorPondT/status",
 "json_attributes_topic":"homeassistant/sensor/sensorPondT/attributes",
 "unit_of_measurement":"°C",
 "value_template":"{{ value_json.temperature}}"
}
````
##### State - Temperature
"homeassistant/sensor/sensorPondT/state"
````json
{"temperature":14.9375}
````
##### Attributes
"homeassistant/sensor/sensorPondT/attributes"
````json
{
 "battery_voltage":2.9455078125,
 "battery_status":89.2578125,
 "RSSI":-81
}
````
##### Status
"homeassistant/sensor/sensorPondT/status"
````
online | offline 
````
## The Hardware

## The Wiring
![](images/pondTemperature_Steckplatine.png)
![](images/pondTemperature_Schaltplan.png)
### Configure the Buck Converter

## The Configuration

## The Home Assistant Integration 
### Sensor
![](images/ha_sensor_pond_temperature.png)

![](images/entity_view_pond_temperature.png)

### View (Example)
![](images/ha_preview_pond_temperature.png)

```` yaml
- type: gauge
  entity: sensor.pond_temperature
  name: Pond Temperature
  unit: "°C"
  needle: true
  min: -5
  max: 35
  severity:
    red: 0
    yellow: 10
    green: 20
````



## The Case
![](images/case_pond_temperature.jpeg)