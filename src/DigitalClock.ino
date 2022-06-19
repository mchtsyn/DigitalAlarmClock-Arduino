#include <TimeLib.h>
#include <JC_Button.h>
#include <LiquidCrystal.h>
#include "DHT.h"

//LCD chars
byte emptyBell[] = {
  B00100,
  B01110,
  B01010,
  B01010,
  B10001,
  B11111,
  B00100,
  B00000
};

byte filledBell[] = {
  B00100,
  B01110,
  B01110,
  B01110,
  B11111,
  B11111,
  B00100,
  B00000
};

//Initialize LCD 
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Welcome Screen
char welcomeMessage[12]= {'A','L','A','R','M',' ','C','L','O','C','K',' '};
bool show = HIGH;
int timeCtr = 0;

//Time Management
int hh=19,mm=5,ss=48; //Initial Time (can be changed)
int hh12;
unsigned long currentMillis = 0;

//24H and 12H 
bool _24Hon = HIGH;
bool _12Hon = LOW;
bool AM;
bool PM;

//Clock Setup Vars
int clockMode = 0;
int clockModeGen = 0;
int alarmModeGen = 0;
int BTN1counter = 0;
int cursorX_2;
int changeHHorMM_2 = 0; 

//Blink Vars
unsigned long blinkMillisPrev;
bool lcdBlinkmin = LOW;
int cursorX;
int cursorY;
int changeHHorMM = 0; 

//Button Vars
int BUTTON1PIN = 9;
int BUTTON2PIN = 8;
int BUTTON3PIN = 7;
int BUTTON4PIN = 6;
Button BTN1(BUTTON1PIN,25,false,false); 
Button BTN2(BUTTON2PIN,25,false,false); 
Button BTN3(BUTTON3PIN,25,false,false); 
Button BTN4(BUTTON4PIN,25,false,false); 
int button1State=0;
int button2State=0;
int button3State=0;
int button4State=0;
const int LONG_PRESS_TIME = 3000;
int funcOutput = 0;

//Temperature
bool Fahr = LOW, Celc = HIGH;
#define DHTPIN A0
#define DHTTYPE DHT11
DHT dht(DHTPIN,DHTTYPE);
float temp;

//Alarm Vars
int BTN2counter = 0;
int hhAlarm = 0;
int mmAlarm = 0;
bool isSet = LOW;
bool isAlarmRinging = LOW;
bool notFirst = LOW;

//Buzzer
int BUZZERPIN = 13;

//Functions
void welcomeScreen();
void Temperature();
void printTime();
int buttonState(Button);
void setClock();
void blink();
void setAlarm();
void clearLCDLine();
void checkAlarm();
void dispAlarm();

void setup()
{
  pinMode(BUTTON1PIN,INPUT);
  pinMode(BUTTON2PIN,INPUT);
  pinMode(BUTTON3PIN,INPUT);
  pinMode(BUTTON4PIN,INPUT);
  pinMode(BUZZERPIN,OUTPUT);

  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setCursor(4,0);
  lcd.print("DIGITAL");
  lcd.createChar(0, emptyBell);
  lcd.createChar(1, filledBell);

  dht.begin();

  //TIMER1
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 3036;
  TCCR1B |= (1<<CS12); //pre 256
  TIMSK1 |= (1<<TOIE1); 
}
ISR(TIMER1_OVF_vect)
{
  if(clockModeGen == 0)
  {
    ss++; //increase ss every second (overflow detected)
    TCNT1 = 3036;
  }
}
/*
  clockMode == 0 No buttons pressed, No alarms set
  clockMode == 1 Set Clock mode Minute
  clockMode == 2 Set Clock mode Hour
  clockModeGen Set Clock mode General
  alarmModeGen Set Alarm mode General
  clockMode == 3 Set Alarm mode Minute
  clockMode == 4 Set Alarm mode Hour
*/
void loop()
{
  currentMillis = millis();
  if(show) welcomeScreen();
  else
  { // --- MAIN --- //
    BTN1.read();    //Check Button States
    BTN2.read();
    BTN3.read();
    BTN4.read();
    button1State = buttonState(BTN1);
    button2State = buttonState(BTN2);
    button3State = buttonState(BTN3);
    button4State = buttonState(BTN4);

    if(button1State == 1 && clockMode == 0) //24H and 12H
    {
      clearLCDLine();
      if(_24Hon == HIGH)
      {
        _24Hon = LOW;
        _12Hon = HIGH;
      }
      else
      {
        _24Hon = HIGH;
        _12Hon = LOW;
      }
    }
    if(button1State == 2) //Enter clock set up mode
    {
      BTN1counter = 0;
      clockMode = 1;
      clockModeGen = 1;
    } 
    
    if(button2State == 2) //Enter alarm set up mode
    {
      isSet = LOW;
      BTN2counter = 0;
      hhAlarm = hh; //Initially, show current time on the alarm. 
      mmAlarm = mm;
      dispAlarm();
      clockMode = 3;
      alarmModeGen = 1;
    }
    if(clockMode == 0 || clockMode == 3 || clockMode == 4) //Clock counts up when -> idle, alarm set up
    {
      printTime();
      lcd.setCursor(0,1); //
      if(!isSet) lcd.write(byte(0)); // Print empty bell to indicate that no alarm is set 
      if(isSet) dispAlarm();
    }
    if(alarmModeGen == 1) //Set Alarm
    {
      setAlarm();
      blink();
    }
    if(clockModeGen == 1) //Set Clock
    {
      setClock();
      blink();
    }
    if(isSet) //Print filledBell to indicate that alarm is set
    {
      lcd.setCursor(0,1);
      lcd.write(byte(1));
    }
    if(BTN2counter == 0 && button2State == 1) // Alarm ON/OFF
    {
      if(isSet && notFirst)
      {
        isSet = LOW;
        isAlarmRinging = LOW;
        digitalWrite(BUZZERPIN,LOW);
        lcd.setCursor(0,1);
        lcd.write(byte(0));
        lcd.print("     ");
      }
      else{
        isSet = HIGH;
        dispAlarm();
      }
      notFirst = HIGH;
    }
    
    checkAlarm(); // Is alarm==clock
    Temperature();
  }
}
void dispAlarm()
{
  lcd.setCursor(1,1);
  if(hhAlarm<10 && mmAlarm<10)
  {
    lcd.print(0);
    lcd.print(hhAlarm);
    lcd.print(":");
    lcd.print(0);
    lcd.print(mmAlarm);
  }
  else if(hhAlarm>10 && mmAlarm<10)
  {
    lcd.print(hhAlarm);
    lcd.print(":");
    lcd.print(0);
    lcd.print(mmAlarm);
  }
  else if(hhAlarm>10 && mmAlarm>10)
  {
    lcd.print(hhAlarm);
    lcd.print(":");
    lcd.print(mmAlarm);
  }
  else if(hhAlarm<10 && mmAlarm>10)
  {
    lcd.print(0);
    lcd.print(hhAlarm);
    lcd.print(":");
    lcd.print(mmAlarm);
  }
}
void clearLCDLine()
{               
        lcd.setCursor(0,0);
        for(int n = 0; n < 16; n++)
        {
          lcd.print(" ");
        }
}

void welcomeScreen()
{
    lcd.setCursor(1,1);

    for (size_t i = 0; i < 12; i++)
    {
      lcd.print(welcomeMessage[i]);
    }
    delay(200);
    timeCtr++;
    char changeLast = welcomeMessage[0];
    for (size_t i = 0; i < 12; i++)
    {
      welcomeMessage[i] = welcomeMessage[i+1];
    }
    welcomeMessage[11] = changeLast;
    
    if(timeCtr>=12) 
    {
      show = LOW;
      lcd.clear();
    } 
}

void setAlarm()
{
  if(button2State == 1) BTN2counter++;

  lcd.setCursor(0,1);
  lcd.write(byte(0)); //emptyBell
  if(clockMode == 3 && button3State == 1) mmAlarm++;
  if(clockMode == 4 && button3State == 1) hhAlarm++;
  if(mmAlarm>=60) mmAlarm = 0;
  if(hhAlarm>=24) hhAlarm = 0;
  if(BTN2counter == 0) //Alarm Minute Set
  {
    clockMode = 3;
  }
  else if(BTN2counter == 1)
  {
    clockMode = 4;
    lcd.setCursor(4,1); //prevent blank
    if(mmAlarm<10)
    {
      lcd.print("0");
      lcd.print(mmAlarm);
    }
    else if(mmAlarm >= 10) lcd.print(mmAlarm);
  }
  else if(BTN2counter == 2)
  {
    BTN2counter = 0;
    isSet = HIGH;
    clockMode = 0;
    alarmModeGen = 0;
    lcd.setCursor(0,1);
    lcd.write(byte(1));
    if(hhAlarm<10)
    {
      lcd.print("0");
      lcd.print(hhAlarm);
    }
    else if(hhAlarm >= 10) lcd.print(hhAlarm);
  }
}

void Temperature()
{
  temp = dht.readTemperature();
  String celcORfahr ="C";
  if(button3State == 1 && Celc && clockMode == 0)
  {
    Fahr = HIGH;
    Celc = LOW;
  }
  else if(button3State && Fahr && clockMode == 0)
  {
    Fahr = LOW;
    Celc = HIGH;
  }

  if(Fahr)
  {
    temp = temp*1.8+32; //Celcius to Fahrenheit conversion 
    celcORfahr = "F";
  }
  else if(Celc)
  {
    celcORfahr = "C";
  } 
  String tempStr = String(temp) + (char)223 + celcORfahr; //(char)223 = degree symbol
  lcd.setCursor(9,1);
  lcd.print(tempStr);
}

void printTime()
{
  //Time management
    if(ss>=60)
    {
      ss=0;
      mm++;
    }
    if(mm>=60)
    {
      mm=0;
      hh++;
    }

    if(_24Hon)
    {
      lcd.setCursor(4,0); 
      lcd.print((hh/10)%10);
      lcd.print(hh % 10); 
      lcd.print(":");
      lcd.print((mm/10)%10);
      lcd.print(mm % 10);
      lcd.print(":");
      lcd.print((ss/10)%10);
      lcd.print(ss % 10);
      lcd.print(" ");
    }  
    else if(_12Hon)
    {
      if(hh<12)
      {
        AM = HIGH;
        PM = LOW;
        hh12 = hh;
      } 
      else if(hh>=12)
      {
        PM = HIGH;
        AM = LOW;
        hh12 = hh - 12;
      }
      if(clockMode == 2 && PM)
      {
        hh12 = hh - 12;
      }
      lcd.setCursor(3,0); 
      lcd.print((hh12/10)%10);
      lcd.print(hh12 % 10); 
      lcd.print(":");
      lcd.print((mm/10)%10);
      lcd.print(mm % 10);
      lcd.print(":");
      lcd.print((ss/10)%10);
      lcd.print(ss % 10);
      if(AM) lcd.print("AM");
      else if(PM) lcd.print("PM");
    }
}

int buttonState(Button btn)
{
    if(!btn.pressedFor(LONG_PRESS_TIME) && btn.wasPressed())
      funcOutput = 1;
    else if(btn.pressedFor(LONG_PRESS_TIME))
    {
      funcOutput = 2;
      return funcOutput;
    }
    if(btn.wasReleased())
    {
      return funcOutput;
    }
    else return 0; 
}

void setClock()
{
    if(clockMode == 2) //Set Clock Minute shows blank bug
    {
      if(_12Hon)
      {
        lcd.setCursor(6,0);
        if(mm<10){
          lcd.print("0");
          lcd.print(mm);
        }
        else{
          lcd.print(mm);
        }
      }
      else if(_24Hon)
      {
        lcd.setCursor(7,0);
        if(mm<10){
          lcd.print("0");
          lcd.print(mm);
        }
        else{
          lcd.print(mm);
        }
      }
    }
    if(button1State == 1) BTN1counter++; //counting up the button1 presses for changing set up mode
    
    if(BTN1counter == 1 && clockModeGen == 1) clockMode = 2; //in clock set up mode, if button1 is pressed first time 

    if(button3State == 1 && clockMode == 1) //SET MINUTE
    { 
        mm++;
        if(mm>=60) mm=0;
    }

    if(button3State == 1 && clockMode == 2) //SET HOUR
    {
      if(_12Hon && hh12<12)
      {
        hh12++;
        hh = hh12; 
      } 
      else if(_24Hon) hh++;
      if(hh>=24 && _24Hon) hh=0;
      if(hh12>=12 && PM && clockModeGen == 1)
      {
        hh12 = 0;
        hh = hh12 + 12;
      } 
    }

    if(BTN1counter == 2 && clockMode == 2) // Exit clock set up
    {
      if(PM && _12Hon) 
      {
        hh = hh12 +12;
      }
      clockModeGen = 0;
      clockMode = 0;
    }
}

void blink()
{
  //CLOCK 
  if(_24Hon && clockMode == 1){
    cursorX = 7;
    cursorY = 0;
    changeHHorMM = mm;
  } 
  if(_24Hon && clockMode == 2){
    cursorX = 4;
    cursorY = 0;
    changeHHorMM = hh;
  } 
  if(_12Hon && clockMode == 1)
  {
    cursorX = 6;
    cursorY = 0;
    changeHHorMM = mm;
  }
  if(_12Hon && clockMode == 2)
  {
    cursorX = 3;
    cursorY = 0;
    changeHHorMM = hh12;
  } 
  //ALARM
  if(clockMode == 3) //Min
  {
    cursorX = 4;
    cursorY = 1;
    changeHHorMM = mmAlarm;
  }
  if(clockMode == 4) //Hour
  {
    cursorX = 1;
    cursorY = 1;
    changeHHorMM = hhAlarm;
  }
  if(isAlarmRinging)
  {
    cursorX = 1;
    cursorY = 1;
  }
    //HIGH to LOW , LOW to HIGH
    if((unsigned long)(currentMillis - blinkMillisPrev) >= 50)
    {
        lcd.setCursor(cursorX,cursorY);
        if(!isAlarmRinging)
        {
          if(lcdBlinkmin == LOW)
          {
            lcd.print(" ");
            lcd.print(" ");
          }
          else if(lcdBlinkmin == HIGH)
          {
            if(changeHHorMM<10)
            {
              lcd.print(0);  
              lcd.print(changeHHorMM);
            }
            else if(changeHHorMM>=10){
              lcd.print(changeHHorMM);
            }
          }
        }
        if(isAlarmRinging) //Blink for alarm
        {
          if(lcdBlinkmin == LOW)
          {
            lcd.setCursor(1,1);
            lcd.print("     ");
          }
          else if(lcdBlinkmin == HIGH)
          {
            lcd.setCursor(1,1);
            lcd.print("     ");
          }
        }

        if(lcdBlinkmin==LOW) lcdBlinkmin = HIGH;
        else lcdBlinkmin = LOW; 
        blinkMillisPrev = currentMillis;
    }
}

void checkAlarm()
{
  if(isSet)
  {
    if(mmAlarm == mm && hhAlarm == hh)
    {
      isAlarmRinging = HIGH;
      blink();
      digitalWrite(BUZZERPIN,HIGH);
      if(button4State == 1) //Delay alarm 5 min
      {
        mmAlarm+=5;
        isAlarmRinging = LOW;
        digitalWrite(BUZZERPIN,LOW);
        dispAlarm();
      }
    }
  }
}
