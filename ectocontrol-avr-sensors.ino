// based on https://ectostroy.ru/download/protocol_description_modbus_RS485_ectostroy_ru.pdf

#include <EEPROM.h>
#include "ModbusRtu.h"

#include <DHT.h>
#include <DHT_U.h>
#include <ArduinoUniqueID.h>

#include <DS18B20.h>


const int dhtPinStart = 5;
const int ledPin = 3;
//const int butPin = 8;
const int rs485Pin = 2;
const word rs485Speed = 19200;

enum SensorType {
  ST_TEMP = 0x22,
  ST_HUM = 0x23
};

const byte sensType = ST_TEMP;
const int sensorsCount = 8;
DHT_Unified* dht[sensorsCount];
DS18B20* ds[sensorsCount];

const byte defaultAddr = 0xF0;
byte curAddr = 0x00;
Modbus bus(curAddr, Serial, rs485Pin);

const uint8_t modbusDataSize = 0x30;
const uint8_t sensorsDataOffset = 0x20;
uint16_t modbusData[modbusDataSize];

void blinkLed(int cnt = 1, int delayms = 50) {
  for(int i = 0; i < cnt; ++i) {
    digitalWrite (ledPin, HIGH);
    delay(delayms);
    digitalWrite (ledPin, LOW);
    delay(delayms);
  }
}

void setup() {
  Serial.begin(rs485Speed);

  pinMode(ledPin, OUTPUT);
  digitalWrite (ledPin, LOW);

  blinkLed(5, 200);
  //EEPROM.put(0, defaultAddr);

  /*pinMode(butPin, INPUT);
  if(digitalRead(butPin) == HIGH) {
  blinkLed(5, 200);
    EEPROM.put(0, defaultAddr);
  } else {
    blinkLed(3, 200);
  }*/

  for(int i = 0; i < sensorsCount; ++i){
    pinMode(i + dhtPinStart, INPUT);
    if(sensType == 0x23) {
      dht[i] = new DHT_Unified(i + dhtPinStart, DHT11);
      dht[i]->begin();
    } else {
      ds[i] = new DS18B20(i + dhtPinStart);
      ds[i]->selectNext();
    }
  }

  EEPROM.get(0, curAddr);
  bus.setID(curAddr);
  bus.start();

  modbusData[0x00] = 0x80; //FIXME: fill with avr's ID
  modbusData[0x01] = UniqueID[UniqueIDsize - 1] << 8 + UniqueID[UniqueIDsize - 2];
  modbusData[0x02] = curAddr;

  word tmp = (word(sensType) << 8) + sensorsCount;
  modbusData[0x03] = tmp;
  delay(1000);
  updateSensorsData();
  blinkLed(1, 200);
}

void checkAddr(){
  const byte newAddr = bus.getID();
  if(newAddr != curAddr) {
    EEPROM.put(0, newAddr);
    curAddr = newAddr;
    modbusData[0x02] = curAddr;
  }
}

void updateSensorsData() {
  sensors_event_t event;
  
  for(int i = 0; i < sensorsCount; ++i){
    if (sensType == ST_TEMP) {
      modbusData[sensorsDataOffset + i] = word(ds[i]->getTempC() * 10.0);
    } else {
      dht[i]->humidity().getEvent(&event);
      modbusData[sensorsDataOffset + i] = word(event.relative_humidity * 10.0);
    }
  }
}

void loop() {
  int8_t state = bus.poll(modbusData, modbusDataSize);
  if(state > 4) {
    blinkLed(1, 10);
  }

  checkAddr();

  updateSensorsData();
}
