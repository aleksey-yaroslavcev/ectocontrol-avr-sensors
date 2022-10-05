// based on https://ectostroy.ru/download/protocol_description_modbus_RS485_ectostroy_ru.pdf

#include <EEPROM.h>
#include <ModbusRtu.h>
#include <DHT.h>
#include <DHT_U.h>

const byte sensType = 0x22; // 0x22 - temp, 0x23 - humidity
const int sensorsCount = 5;
DHT_Unified* dht[sensorsCount];

const int ledPin = 7;

const int butPin = 6;

const byte defaultAddr = 0xF0;
byte curAddr = 0x00;
const word rs485Speed = 19200;
const int rs485pin = 2;
Modbus bus(curAddr, Serial, rs485pin);
const int numHR = 4;  // 4 general registers
const int numIR = 10; // 10 sensors data
uint16_t modbusData[numHR + numIR];

void blinkLed(int cnt = 1) {
  for(int i = 0; i < cnt; ++i) {
    digitalWrite (ledPin, HIGH);
    delay(50);
    digitalWrite (ledPin, LOW);
    delay(50);
  }
}

void setup() {
  Serial.begin(rs485Speed);

  pinMode(ledPin, OUTPUT);
  digitalWrite (ledPin, LOW);

  if(digitalRead(butPin)) {
    blinkLed(5);
    EEPROM.put(0, defaultAddr);
  }

  for(int i = 0; i < sensorsCount; ++i){
    dht[i] = new DHT_Unified(i + 8, DHT11);
    dht[i]->begin();
  }

  EEPROM.get(0, curAddr);
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
  
  for(int i = 0; i < sensorsCount; ++i){
    dht[i]->temperature().getEvent(&event);
    modbusData[0x04 + i] = word(event.temperature * 10.0);
  }
}

void loop() {
  int8_t state = bus.poll(modbusData, numHR + numIR);
  if(state > 4) {
    blinkLed();
  }

  checkAddr();

  updateSensorsData();
}
