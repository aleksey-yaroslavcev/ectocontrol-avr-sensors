// based on https://ectostroy.ru/download/protocol_description_modbus_RS485_ectostroy_ru.pdf

#include <EEPROM.h>
#include "ModbusRtu.h"
#include <DHT.h>
#include <DHT_U.h>

const int dhtPinStart = 5;
const int ledPin = 3;
//const int butPin = 8;
const int rs485Pin = 2;
const word rs485Speed = 19200;

const byte sensType = 0x22; // 0x22 - temp, 0x23 - humidity
const int sensorsCount = 8;
DHT_Unified* dht[sensorsCount];

const byte defaultAddr = 0xF0;
byte curAddr = 0x00;
Modbus bus(curAddr, Serial, rs485Pin);


uint16_t modbusData[0x30];

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
    dht[i] = new DHT_Unified(i + dhtPinStart, DHT11);
    dht[i]->begin();
  }

  EEPROM.get(0, curAddr);
  bus.setID(curAddr);
  bus.start();

  modbusData[0x00] = 0x80; //FIXME: fill with avr's ID
  modbusData[0x01] = 0x00;
  modbusData[0x02] = curAddr;

  word tmp = (word(sensType) << 8) + sensorsCount;
  modbusData[0x03] = tmp;
  delay(1000);
  blinkLed(1, 200);
}

void checkAddr(){
  const byte newAddr = bus.getID();;
  if(newAddr != curAddr) {
    EEPROM.put(0, newAddr);
    curAddr =newAddr;
  }
}

void updateSensorsData() {
  sensors_event_t event;
  
  for(int i = 0; i < sensorsCount; ++i){
    dht[i]->temperature().getEvent(&event);
    modbusData[0x20 + i] = word(event.temperature * 10.0);
  }
}

void loop() {
  int8_t state = bus.poll(modbusData, 30);
  if(state > 4) {
    blinkLed(1, 20);
  }

  checkAddr();

  updateSensorsData();
}
