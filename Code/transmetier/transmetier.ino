/*A basic 6 channel transmitter using the nRF24L01 module.*/
/* Like, share and subscribe, ELECTRONOOBS */
/* http://www.youtube/c/electronoobs */

/* First we include the libraries. Download it from 
   my webpage if you donw have the NRF24 library */


//This are the bytes for the EN logo


#include <SPI.h>
#include <nRF24L01.h>             //Downlaod it here: https://www.electronoobs.com/eng_arduino_NRF24.php
#include <RF24.h>              
#include <Wire.h>
#include <GyverOLED.h>
#include <EEPROM.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//OLED setup
GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> display;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




/*Create a unique pipe out. The receiver has to 
  wear the same unique code*/
  
const uint64_t pipeOut = 0xE8E8F0F0E1LL; //IMPORTANT: The same as in the receiver!!!

RF24 radio(9, 10); // select  CSN  pin

// The sizeof this struct should not exceed 32 bytes
// This gives us up to 32 8 bits channals
struct MyData {
  byte throttle;
  byte yaw;
  byte pitch;
  byte roll;
  byte AUX1;
  byte AUX2;
};

MyData data;



//Inputs outputs
int battery_in = A7;                  //pin for analog in from the battery divider
int buttons_analog_in = A6;           //Analog in from all the push buttons
int toggle_1 = 3; 
int toggle_2 = 2;
int throttle_in = A3;
int yaw_in = A2;
int pitch_in = A0;
int roll_in = A1;
int mode_in = 4;
int buzzer = 5;

//Variables
float battery_level = 0;
int throttle_fine = 0;
int yaw_fine = 0;
int pitch_fine = 0;
int roll_fine = 0;
int button_read = 0;

int throttle_to_send = 0;
int yaw_to_send = 0;
int pitch_to_send = 0;
int roll_to_send = 0; 

bool throttle_inverted = false; 
bool yaw_inverted = true; 
bool pitch_inverted = true; 
bool roll_inverted = false; 

bool yaw_decrease = false;
bool throttle_decrease = false;
bool pitch_decrease = false;
bool roll_decrease = false;

bool yaw_increase = false;
bool throttle_increase = false;
bool pitch_increase = false;
bool roll_increase = false;

bool mode = false;
bool mode_button_pressed = false;
bool sound = true;
int counter = 0;
int invert_counter = 0;
bool sound_changed = false;

void resetData() 
{
  //This are the start values of each channal
  // Throttle is 0 in order to stop the motors
  //127 is the middle value of the 10ADC.
    
  data.throttle = 127;
  data.yaw = 127;
  data.pitch = 127;
  data.roll = 127;
  data.AUX1 = 0;
  data.AUX2 = 0;
}

void setup()
{
  if( EEPROM.read(1) != 55)
  {
    EEPROM.write(2, 127);
    EEPROM.write(3, 127);
    EEPROM.write(4, 127);
    EEPROM.write(5, 127); 
    EEPROM.write(6, 0);
    EEPROM.write(7, 1);
    EEPROM.write(8, 1);
    EEPROM.write(9, 0);    
    EEPROM.write(1, 55);
  }

  throttle_fine = EEPROM.read(2);
  yaw_fine = EEPROM.read(3);
  pitch_fine = EEPROM.read(4);
  roll_fine = EEPROM.read(5);
  throttle_inverted = EEPROM.read(6);
  yaw_inverted = EEPROM.read(7);
  pitch_inverted = EEPROM.read(8);
  roll_inverted = EEPROM.read(9);
  
  pinMode(buttons_analog_in, INPUT);
  pinMode(mode_in, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer,LOW);


  delay(100);
  display.init();  
  display.clear();
  display.setScale(1);
  
  digitalWrite(buzzer,HIGH);
  delay(40);
  digitalWrite(buzzer,LOW);  
  delay(40);
  digitalWrite(buzzer,HIGH);
  delay(40);
  digitalWrite(buzzer,LOW);  


  delay(2000);
  
  //Start everything up
  radio.begin();
  radio.setAutoAck(false);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(pipeOut);
  resetData();
}

/**************************************************/




int map_normal(int val, int lower, int middle, int upper, bool reverse)
{
  val = constrain(val, lower, upper);
  if ( val < middle )
    val = map(val, lower, middle, 0, 128);
  else
    val = map(val, middle, upper, 128, 255);
  return ( reverse ? 255 - val : val );
}





// Returns a corrected value for a joystick position that takes into account
// the values of the outer extents and the middle of the joystick range.
int map_exponential(int val, bool reverse)
{
  val = constrain(val, 0, 1023);
  float cube = ((pow((val - 512),3)/520200) + 258.012) / 2; 
  return ( reverse ? 255 - cube : cube );
}










void loop()
{
  //battery read
  battery_level = analogRead(battery_in) / 67.331; //////Voltage divider is 10k and 20K so 1/3


  //Buttons read
  button_read = analogRead(buttons_analog_in);
  //Reset buttons
  if(button_read > 820)
  {
    yaw_decrease = true;
    throttle_decrease = true;
    pitch_decrease = true;
    roll_decrease = true;
    yaw_increase = true;
    throttle_increase = true;
    pitch_increase = true;
    roll_increase = true;
  }
  //////////////////////////////////////////////////////////////////////////////////////////
  
  //YAW buttons
  if(button_read < 436 && button_read > 416 && !yaw_decrease)
  {
    yaw_fine = yaw_fine + 1;
    yaw_decrease = true;
    EEPROM.write(3, yaw_fine);
    if(sound)
    {
      digitalWrite(buzzer,HIGH);
      delay(50);
      digitalWrite(buzzer,LOW); 
    }
  }
  if(button_read < 205 && button_read > 185 && !yaw_increase)
  {
    yaw_fine = yaw_fine - 1;
    yaw_increase = true;
    EEPROM.write(3, yaw_fine);
    if(sound)
    {
      digitalWrite(buzzer,HIGH);
      delay(50);
      digitalWrite(buzzer,LOW); 
    }
  }
  //////////////////////////////////////////////////////////////////////////////////////////
  
  //THROTTLE buttons
  if(button_read < 340 && button_read > 320 && !throttle_decrease)
  {
    throttle_fine = throttle_fine + 1;
    throttle_decrease = true;
    EEPROM.write(2, throttle_fine);
    if(sound)
    {
      digitalWrite(buzzer,HIGH);
      delay(50);
      digitalWrite(buzzer,LOW); 
    }
  }
  if(button_read < 505 && button_read > 485 && !throttle_increase)
  {
    throttle_fine = throttle_fine - 1;
    throttle_increase = true;
    EEPROM.write(2, throttle_fine);
    if(sound)
    {
      digitalWrite(buzzer,HIGH);
      delay(50);
      digitalWrite(buzzer,LOW); 
    }
  }
  //////////////////////////////////////////////////////////////////////////////////////////

  //PITCH buttons
  if(button_read < 664 && button_read > 644 && !pitch_decrease)
  {
    pitch_fine = pitch_fine + 1;
    pitch_decrease = true;
    EEPROM.write(4, pitch_fine);
    if(sound)
    {
      digitalWrite(buzzer,HIGH);
      delay(50);
      digitalWrite(buzzer,LOW); 
    }
  }
  if(button_read < 600 && button_read > 580 && !pitch_increase)
  {
    pitch_fine = pitch_fine - 1;
    pitch_increase = true;
    EEPROM.write(4, pitch_fine);
    if(sound)
    {
      digitalWrite(buzzer,HIGH);
      delay(50);
      digitalWrite(buzzer,LOW); 
    }
  }
  //////////////////////////////////////////////////////////////////////////////////////////

  //ROLL buttons
  if(button_read < 635 && button_read > 615 && !roll_decrease)
  {
    roll_fine = roll_fine + 1;
    roll_decrease = true;
    EEPROM.write(5, roll_fine);
    if(sound)
    {
      digitalWrite(buzzer,HIGH);
      delay(50);
      digitalWrite(buzzer,LOW); 
    }
  }
  if(button_read < 558 && button_read > 538 && !roll_increase)
  {
    roll_fine = roll_fine - 1;
    roll_increase = true;
    EEPROM.write(5, roll_fine);
    if(sound)
    {
      digitalWrite(buzzer,HIGH);
      delay(50);
      digitalWrite(buzzer,LOW); 
    }
  }


  //Mode select button
  if(!digitalRead(mode_in) && !mode_button_pressed)
  {
    mode = !mode;
    mode_button_pressed = true;
    if(sound)
    {
      digitalWrite(buzzer,HIGH);
      delay(50);
      digitalWrite(buzzer,LOW); 
    }
  }
  
  if(!digitalRead(mode_in) && !sound_changed)
  {
    if(counter > 20)
    {
      sound = !sound;
      counter = 0;
      sound_changed = true;
      if(sound)
      {
        digitalWrite(buzzer,HIGH);
        delay(50);
        digitalWrite(buzzer,LOW); 
      }
    }
    counter = counter + 1;
  }



  //Invert channels
  //THROTTLE INVERT
  if(button_read < 340 && button_read > 320)
  {
    if(invert_counter > 30)
    {
      throttle_inverted = !throttle_inverted;
      invert_counter = 0;
      EEPROM.write(6, throttle_inverted);
      display.clear();            //Clear the display  
      display.setCursorXY(13,30);            //Select where to print 124 x 64
      display.print("Throttle inverted");  
      if(sound)
      {
        digitalWrite(buzzer,HIGH);
        delay(50);
        digitalWrite(buzzer,LOW); 
      }
      delay(1500);
    }
    invert_counter = invert_counter + 1;
  }

  //YAW INVERT
  if(button_read < 436 && button_read > 416)
  {
    if(invert_counter > 30)
    {
      yaw_inverted = !yaw_inverted;
      invert_counter = 0;
      EEPROM.write(7, yaw_inverted);
      display.clear();            //Clear the display  
      display.setCursorXY(15,30);            //Select where to print 124 x 64
      display.print("  Yaw inverted");   
      if(sound)
      {
        digitalWrite(buzzer,HIGH);
        delay(50);
        digitalWrite(buzzer,LOW); 
      }
      delay(1500);
    }
    invert_counter = invert_counter + 1;
  }

  //PITCH INVERT
  if(button_read < 664 && button_read > 644)
  {
    if(invert_counter > 30)
    {
      pitch_inverted = !pitch_inverted;
      invert_counter = 0;
      EEPROM.write(8, pitch_inverted);
      display.clear();            //Clear the display  
      display.setCursorXY(13,30);            //Select where to print 124 x 64
      display.print("  Pitch inverted");
      if(sound)
      {
        digitalWrite(buzzer,HIGH);
        delay(50);
        digitalWrite(buzzer,LOW); 
      }
      delay(1500);
    }
    invert_counter = invert_counter + 1;
  }

  //ROLL INVERT
  if(button_read < 635 && button_read > 615)
  {
    if(invert_counter > 30)
    {
      roll_inverted = !roll_inverted;
      invert_counter = 0;
      EEPROM.write(9, roll_inverted);
      display.clear();            //Clear the display  
      display.setCursorXY(15,30);            //Select where to print 124 x 64
      display.print("  Roll inverted");        
      if(sound)
      {
        digitalWrite(buzzer,HIGH);
        delay(50);
        digitalWrite(buzzer,LOW); 
      }
      delay(1500);
    }
    invert_counter = invert_counter + 1;
  }

  


  
  if(digitalRead(mode_in) && mode_button_pressed)
  {
    mode_button_pressed= false;    
    sound_changed = false;
    counter = 0;
    invert_counter = 0;
  }
  
  
  //Mode select  
  if(mode)
  {
  throttle_to_send = map_normal(analogRead(throttle_in), 0, 512, 1023, throttle_inverted);
  yaw_to_send = map_normal(analogRead(yaw_in), 0, 512, 1023,           yaw_inverted);
  pitch_to_send = map_normal(analogRead(pitch_in), 0, 512, 1023,       pitch_inverted);
  roll_to_send = map_normal(analogRead(roll_in), 0, 512, 1023,         roll_inverted);  
  }
  
  if(!mode)
  {
  throttle_to_send = map_exponential(analogRead(throttle_in), throttle_inverted);
  yaw_to_send = map_exponential(analogRead(yaw_in),           yaw_inverted);
  pitch_to_send = map_exponential(analogRead(pitch_in),       pitch_inverted);
  roll_to_send = map_exponential(analogRead(roll_in),         roll_inverted);  
  }

  


  throttle_to_send = throttle_to_send  + throttle_fine - 127;
  yaw_to_send = yaw_to_send  + yaw_fine - 127;
  pitch_to_send = pitch_to_send  + pitch_fine - 127;
  roll_to_send = roll_to_send  + roll_fine - 127;  
  
  
  data.throttle = constrain(throttle_to_send,0,255);
  data.yaw      = constrain(yaw_to_send,0,255);
  data.pitch    = constrain(pitch_to_send,0,255);
  data.roll     = constrain(roll_to_send,0,255);
  data.AUX1     = digitalRead(toggle_1);
  data.AUX2     = digitalRead(toggle_2);

  radio.write(&data, sizeof(MyData));


  display.clear();            //Clear the display  
  if(sound)
  {
    display.setCursorXY(0,0);            //Select where to print 124 x 64
    display.print("Sound ON");      
  }
  if(!sound)
  {
    display.setCursorXY(0,0);            //Select where to print 124 x 64
    display.print("Sound OFF");      
  }
  display.setCursorXY(90,0);            //Select where to print 124 x 64
  display.print(battery_level,1);      
  display.print("V");     
  display.setCursorXY(0,16);            //Select where to print 124 x 64
  display.print("T: ");             
  display.print(throttle_to_send);     
  display.print("      P: ");  
  display.print(pitch_to_send);    
  display.setCursorXY(0,29);
  display.print("Y: ");             
  display.print(yaw_to_send);     
  display.print("      R: ");  
  display.print(roll_to_send);  
  // 
  display.setCursorXY(0,42);
  display.print("SW1: ");             
  display.print(digitalRead(toggle_2));     
  display.print("      SW2: ");  
  display.print(digitalRead(toggle_1));  
  if(mode)
  {
    display.setCursorXY(0,56);
    display.print("Mode: ");      
    display.print("Exponential"); 
  }
  if(!mode)
  {
    display.setCursorXY(0,56);
    display.print("Mode: ");      
    display.print("Linear"); 
  }
  //  


  
}