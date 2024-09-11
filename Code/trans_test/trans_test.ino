#include "SPI.h"       // библиотека для протокола SPI
#include "nRF24L01.h"  // библиотека для nRF24L01+
#include "RF24.h"      // библиотека для радио модуля
#include <GyverOLED.h>
GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;

#define throttleA A0  // ось X джойстика
#define yawA A1       // ось Y джойстика
#define rollA A3      // ось X джойстика
#define pitchA A2     // ось Y джойстика

int throttle;
int yaw;
int roll;
int pitch;
int trimmer; 
//int rollmin = 0; 
//int rollmax = 0; 
//int rollsum;
uint32_t timer = 0;



const uint64_t pipeOut = 0xE8E8F0F0E1LL;  // идентификатор передачи
RF24 radio(9, 10);                        // Для MEGA2560 замените на RF24 radio(9,53);

struct MyData {
  byte throttle;
  byte yaw;
  byte pitch;
  byte roll;
  byte AUX1;
  byte AUX2;
};

MyData data;

void resetData() {
  //This are the start values of each channal
  // Throttle is 0 in order to stop the motors
  //127 is the middle value of the 10ADC.

  data.throttle = 0;
  data.yaw = 127;
  data.pitch = 127;
  data.roll = 127;
  data.AUX1 = 0;
  data.AUX2 = 0;
}

void setup() {
  Serial.begin(9600);

  pinMode(throttleA, INPUT);
  pinMode(yawA, INPUT);
  pinMode(rollA, INPUT);
  pinMode(pitchA, INPUT);
  pinMode(3, INPUT);
  pinMode(2, INPUT);
  pinMode(A6, INPUT);

  radio.begin();
  radio.setAutoAck(false);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(pipeOut);
  resetData();

  oled.init();        // инициализация
  oled.clear();       // очистка
  oled.setScale(2);   // масштаб текста (1..4)
  oled.home();        // курсор в 0,0
}

void loop() {
  if (millis() - timer >= 10) {  // таймер на millis()
    timer = millis();
    throttle = analogRead(throttleA);
    yaw = analogRead(yawA);
    roll = analogRead(rollA);
    pitch = analogRead(pitchA);


    throttle = map(throttle, 0, 1023, 255, 0);  // преобразуем значение X в другой диапазон
    yaw = map(yaw, 0, 1023, 0, 255);            // преобразуем значение Y в другой диапазон
    roll = map(roll, 0, 1023, 0, 255);          // преобразуем значение X в другой диапазон
    pitch = map(pitch, 0, 1023, 255, 0);        // преобразуем значение Y в другой диапазон

    //if (522 < trimmer && trimmer < 542){
    //  rollmin = rollmin + 1;
    //}
    //if (607 < trimmer && trimmer < 627){
    //  rollmax = rollmax + 1;
    //}

    //rollsum = roll - rollmin + rollmax;

    data.throttle = throttle;
    data.yaw = yaw;
    data.pitch = pitch;
    data.roll = roll;
    data.AUX1 = digitalRead(3);
    data.AUX2 = digitalRead(2);

    radio.write(&data, sizeof(MyData));
    
    oled.setCursor(0, 0);
    oled.print("T:");
    oled.print(throttle);
    oled.setCursor(0, 3);
    oled.print("Y:");
    oled.print(yaw);
    oled.setCursor(70, 0);
    oled.print("P:");
    oled.print(pitch);
    oled.setCursor(70, 3);
    oled.print("R:");
    oled.print(roll);
    oled.setCursor(0, 6);
    oled.print("SW1:");
    oled.print(digitalRead(2));
    oled.setCursor(70, 6);
    oled.print("SW2:");
    oled.print(digitalRead(3));
  }

}