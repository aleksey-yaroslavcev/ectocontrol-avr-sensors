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
Modbus bus(curAddr,Serial,4);
uint16_t modbusData[numHR + numIR];

void setup() {
  Serial.begin(19200);
  
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);

  EEPROM.get(0, curAddr);
  if (curAddr == 0) {
    curAddr = defaultAddr;
  }
  EEPROM.put(0, curAddr);
  bus.setID(curAddr);
  bus.start();

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
    bus.setID(newAddr);
  }
}

void updateSensorsData() {
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  modbusData[0x04] = word(event.temperature * 10.0);
}

void loop() {
  int8_t state = bus.poll(modbusData, numHR + numIR);

  checkAddr();

  updateSensorsData();
}
