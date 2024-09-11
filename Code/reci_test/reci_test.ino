#include "SPI.h"       // библиотека для протокола SPI
#include "nRF24L01.h"  // библиотека для nRF24L01+
#include "RF24.h"      // библиотека для радио модуля

#define channel_number 6  //set the number of channels
#define sigPin 2  //set PPM signal output pin on the arduino
#define PPM_FrLen 27000  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 400  //set the pulse length
//////////////////////////////////////////////////////////////////

int ppm[channel_number];

int throttle;
int yaw;
int roll;
int pitch;

const uint64_t pipe = 0xF0F1F2F3F4LL;  // идентификатор передачи
RF24 radio(9, 10);                     // Для MEGA2560 замените на RF24 radio(9,53);

void setup() {
  Serial.begin(9600);   // включаем последовательный порт
  radio.begin();        // включаем радио модуль
  radio.setChannel(0);  // выбираем канал (от 0 до 127)
  delay(2);

  radio.setChannel(0);             // устанавливаем канал (0-127)
  radio.setDataRate(RF24_1MBPS);   // скорость передачи данных
  radio.setPALevel(RF24_PA_HIGH);  // мощность передатчика

  radio.openReadingPipe(1, pipe);  // открываем первую трубу
  radio.startListening();          // начинаем слушать трубу


  ppm[0] = 1000;//map(throttle, 0, 255, 1000, 2000);
  ppm[1] = 1500;//map(yaw, 0, 255, 1000, 2000);
  ppm[2] = 1500;//map(roll, 0, 255, 1000, 2000);
  ppm[3] = 1500;//map(pitch, 0, 255, 1000, 2000);

  pinMode(sigPin, OUTPUT);
  digitalWrite(sigPin, 0);  //set the PPM signal pin to the default state (off)
  
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  
  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
}

void loop() {
  float data[4];

  if (radio.available())  // проверяем буфер обмена
  {
    radio.read(&data, sizeof(data));  // читаем данные

    data[0] = throttle;
    data[1] = yaw;
    data[2] = roll;
    data[3] = pitch;

    throttle = map(throttle, 0, 255, 1000, 2000);
    yaw = map(yaw, 0, 255, 1000, 2000);
    roll = map(roll, 0, 255, 1000, 2000);
    pitch = map(pitch, 0, 255, 1000, 2000);

    Serial.print(data[0]);
    Serial.print("     ");
    Serial.print(data[1]);
    Serial.print("     ");
    Serial.print(data[2]);
    Serial.print("     ");
    Serial.print(data[3]);
    Serial.println();
  }

  ppm[0] = throttle;
  ppm[1] = yaw;
  ppm[2] = roll;
  ppm[3] = pitch;

}


ISR(TIMER1_COMPA_vect){  //leave this alone
  static boolean state = true;
  
  TCNT1 = 0;
  
  if (state) {  //start pulse
  PORTD = PORTD & ~B00000100; // turn pin 2 off. Could also use: digitalWrite(sigPin,0)
    OCR1A = PPM_PulseLen * 2;
    state = false;
  } else{  //end pulse and calculate when to start the next pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;
  
    PORTD = PORTD | B00000100; // turn pin 2 on. Could also use: digitalWrite(sigPin,1)
    state = true;

    if(cur_chan_numb >= channel_number){
      cur_chan_numb = 0;
      calc_rest = calc_rest + PPM_PulseLen;// 
      OCR1A = (PPM_FrLen - calc_rest) * 2;
      calc_rest = 0;
    }
    else{
      OCR1A = (ppm[cur_chan_numb] - PPM_PulseLen) * 2;
      calc_rest = calc_rest + ppm[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}