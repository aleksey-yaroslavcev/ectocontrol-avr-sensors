// based on https://ectostroy.ru/download/protocol_description_modbus_RS485_ectostroy_ru.pdf

#include <EEPROM.h>
#include <ModbusRtu.h>
#include <DHT.h>
#include <DHT_U.h>

const int numHR = 4;  // 4 general registers
const int numIR = 10; // 10 sensors data

const byte defaultAddr = 0xF0;
byte curAddr = 0x00;
const word rs485Speed = 19200;

const byte sensType = 0x22; // 0x22 - temp, 0x23 - humidity

DHT_Unified dht(2, DHT11);
Modbus bus(curAddr,Serial,1);
uint16_t modbusData[numHR + numIR];

void(* resetAvr) (void) = 0;

void setup() {
  Serial.begin(9600);
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));  

  EEPROM.get(0, curAddr);
  if (curAddr == 0) {
    curAddr = defaultAddr;
  }
  EEPROM.put(0, curAddr);

  //bus = Modbus(curAddr,Serial,1);

  modbusData[0x00] = 0x80; //FIXME: fill with avr's ID
  modbusData[0x01] = 0x00;
  modbusData[0x02] = curAddr;

  word tmp = (word(sensType) << 8) + 1;
  modbusData[0x03] = tmp;
}

void checkAddr(){
  const byte newAddr = modbusData[0x02] & 0xFF;
  if(newAddr != curAddr) {
    EEPROM.put(0, newAddr);
    resetAvr();
  }
}

void updateSensorsData() {
  // TODO: realize
  sensors_event_t event;
  dht.temperature().getEvent(&event);
   if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
  }
  modbusData[0x03] = word(event.temperature * 10);
}

void loop() {
  delay(2000);
  int8_t state = bus.poll(modbusData, numHR + numIR);
  if (state > 4) {
    Serial.print(F("MBError"));
  }
  checkAddr();

  updateSensorsData();
}
