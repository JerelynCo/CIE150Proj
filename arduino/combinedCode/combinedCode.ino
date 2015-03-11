/*******************************************
Wire Connections:
  RTC shield connections 
    SDA to a4; SCL to a5; GND to gnd; VCC to 5V
  Moisture Sensor
    Blue to A0; Black to gnd, Red to 5V/3.3V
  Temperature&Humidity Sensor
    Green to D4; Black to gnd; Red to 3.3V
  
Notes
  To reconfigure RTC:
    RTC.adjust(DateTime(__DATE__, __TIME__));
  Sensor values description in Moisture Sensor
    0  ~300     dry soil
    300~700     humid soil
    700~950     in water
    
Added and used libraries:
  RTClib
  DHTlib
  LiquidCrystal
********************************************/

#include <LiquidCrystal.h>
#include <Wire.h>
#include <dht11.h>
#include "RTClib.h"

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
RTC_DS1307 RTC;
dht11 DHT;
#define DHT11_PIN 4

//LCD
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

//Time variables
String Month;
String Date;
String Year;
String Hour;
String Minute;
String Second;

//flags
boolean setTime = true;
boolean setSwitch = true;
boolean fillArray = false;
boolean proceed = true;
boolean seen = false;
boolean matched = false;

//Sensor values
int wantedMoisture = 300;

//Sensor/relay pins
int relayPin = 13;
int moistureSensorPin = A3;

//for checkSwitch function
int switchTimerPin = 2;
int switchMoisturePin = 3;
char mode;   // for incoming serial data and switch value

//Watering times var
int nTimes;
const int nTimesMax = 5; //number of times to water the plant in a day
int waterTime[nTimesMax][2] = {0}; //time to water; format: {h,m}
String arr[2] = {"hour", "min"};
int h=1;
int m, i, j=0;


int readButtons(){
 adc_key_in = analogRead(0);     
 if (adc_key_in > 1000) return btnNONE; 
 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 250)  return btnUP; 
 if (adc_key_in < 450)  return btnDOWN; 
 if (adc_key_in < 650)  return btnLEFT; 
 if (adc_key_in < 850)  return btnSELECT;  
 return btnNONE;
}

void checkSerial(){
  if(Serial.available()>0){
    mode = Serial.read();
  }
  else{
    mode = 't';
  }
}

void lcdClearLine(int n){
  lcd.setCursor(0,n);
  lcd.print("                ");
}
  
void checkSwitch(){
  lcdClearLine(1);
  lcd.setCursor(0,1);
  lcd.print("Up:Mstr|Dwn:Tmr");
  
  if(readButtons() == btnDOWN){
    lcdClearLine(0);
    lcd.setCursor(0,0);
    lcd.print("Timer Mode");
    mode = 't';
    setSwitch = false;
  }
  else if(readButtons() == btnUP){
    lcdClearLine(0);
    lcd.setCursor(0,0);
    lcd.print("Moisture Mode");
    mode = 'm';
    setSwitch = false;
  }

  else{
    Serial.println("Choose mode");
    Serial.println("-------------------------------");
    mode = 'n';
  }
}

void timerCtrl(){
  DateTime now = RTC.now(); 

  Year = String(now.year());
  Month = String(now.month());
  Date = String(now.day());
  
  Hour = String(now.hour());
  Minute = String(now.minute());
  Second = String(now.second());
  
  Serial.println("Date: "+Month+'/'+Date+'/'+Year+"; Time: "+Hour+':'+Minute+':'+Second);   
  for(int i = 0; i<nTimes; i++){
    if((waterTime[i][0]==now.hour())&&(waterTime[i][1]==now.minute()) && (0 == now.second())){
      Serial.print("Water Time " + String(i+1) + ":"+String(waterTime[i][0])+":"+String(waterTime[i][1]));
      Serial.println(" [Time matched]");
      matched = true;
      turnRelayON(10000);
    }
    else{
      Serial.print("Water Time " + String(i+1) + ":"+String(waterTime[i][0])+":"+String(waterTime[i][1]));
      Serial.println(" [Time not match]");

    }
  }
}

void moistCtrl(){
  Serial.println("Moisture:" +analogRead(moistureSensorPin));
  lcdClearLine(1);
  lcd.setCursor(0,1);
  lcd.print("Level:" +String(analogRead(moistureSensorPin)));

  if(analogRead(moistureSensorPin) < wantedMoisture){
    Serial.println("More water needed");
    lcd.print(" Water");
    turnRelayON(10000);
  }
  else{
    Serial.println("Idle");
    lcd.print(" Idle");
  }
}

void turnRelayON(int msWaterDuration){
  digitalWrite(relayPin,HIGH);//turn on relay 
  delay(msWaterDuration);
  digitalWrite(relayPin, LOW);
  matched = false;
}

void displayTempAndHum(){
  int chk = DHT.read(DHT11_PIN);
  Serial.print("Temperature: ");
  Serial.print(DHT.temperature,1);
  Serial.print(" deg celcius ; Humidity: ");
  Serial.print(DHT.humidity,1);
  Serial.println("%");
  
  lcdClearLine(0);
  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.setCursor(2,0);
  lcd.print(DHT.temperature,1);
  lcd.setCursor(4,0);
  lcd.print("c H:");
  lcd.setCursor(8,0);
  lcd.print(DHT.humidity,1);
  lcd.setCursor(10,0);
  lcd.print("%"); 
}

void changeWateringTimes(){
  if(!fillArray){
    lcdClearLine(1);
    lcd.setCursor(0,1);
    lcd.print("nTimes:");
    
    if(readButtons()==btnDOWN && nTimes < 5) nTimes++;
    else if(readButtons()==btnUP && nTimes > 1) nTimes--;
    lcd.setCursor(7,1);
    lcd.print(nTimes);
  
    if(readButtons()==btnRIGHT) fillArray = true;
  }
  if(fillArray){
    lcdClearLine(1);
    if(i < nTimes){      
      lcd.setCursor(0,1);
      lcd.print("Enter " + arr[j] + " " + String(i+1)+":");
      if(j==0){
        if(readButtons()==btnDOWN && h < 24) h++;
        else if(readButtons()==btnUP && h > 1) h--;
        lcd.print(h);
      }

      else if(j==1){
        if(readButtons()==btnDOWN && m < 60) m+=15;
        else if(readButtons()==btnUP && m > 0) m-=15;
        lcd.print(m);
      }
      if(readButtons()==btnRIGHT){
        if(j==0) j++;
        else if(j==1){
          waterTime[i][0]=h;
          waterTime[i][1]=m;
          j=0;
          i++;
          h=1;
          m=0;
        }
      }
      else if(readButtons()==btnLEFT){
        if(i==0) j=0;
        else if(i>0){
          i--;
          j=0;
        }
      }      
    }
    else{
      if(!seen){
        lcdClearLine(0);
        lcd.setCursor(0,0);
        lcd.print("Time/s recorded");
        lcdClearLine(1);
        lcd.setCursor(0,1);
        for(int i = 0; i < nTimes; i++){
          lcd.print(" [Time " + String(i+1) + "] " +String(waterTime[i][0]) +":" +String(waterTime[i][0]) +" |");
        }
        if(readButtons()==btnRIGHT){
          seen = true;
        }
      }
      else{
        lcdClearLine(0);
        lcd.setCursor(0,0);
        lcd.print("Time: "+Hour+':'+Minute+':'+Second);
        lcdClearLine(1);
        lcd.setCursor(0,1);
        if(matched) lcd.print("Matched");
        if(!matched) lcd.print("Not matched");
      }
    }
  }
}

void setup(){
  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  lcd.begin(16,2);
  
  if (!RTC.isrunning()) {Serial.println("RTC is NOT running!");}
  //RTC.adjust(DateTime(__DATE__, __TIME__));
  pinMode(relayPin,OUTPUT);
  pinMode(switchTimerPin,INPUT);
  pinMode(switchMoisturePin,INPUT);
  pinMode(13,OUTPUT);
  
  digitalWrite(relayPin,LOW);
  digitalWrite(switchMoisturePin,LOW);
  digitalWrite(switchTimerPin,LOW);
  digitalWrite(13,LOW); 
}
 
void loop(){

  /*STRICTLY choose ONE between checkSerial and checkSwitch
    checkSerial = input through the Serial Monitor/Processing
    checkSwitch = input through the arduino for stand-alone processes
  */
  
  if(setSwitch){
    displayTempAndHum();
    checkSwitch(); 
    
  }
  //press select button to choose a mode again.
  if(readButtons() == btnSELECT){
    setSwitch = true;
  }
  
  //checkSerial();
  /*if(Serial.available()){
    mode = Serial.read();
    //Serial.end();
    holder = mode;
  }
  else{
    mode = holder;
  }*/
  
  if(mode == 't'){
    Serial.println("Timer Selected");
    changeWateringTimes();
    timerCtrl();
    Serial.println("-------------------------------\t");
  }
  else if(mode == 'm'){
    Serial.println("Moisture Selected");
    moistCtrl();
    Serial.println("-------------------------------\t");
    
  }
   
  delay(200); 
}
