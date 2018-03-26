#include <dht.h>
#include <Time.h>
#include <TimeAlarms.h>

boolean ledToggle = false;
const int ledPin2 = 2;
const int ledPin3 = 3;
const int ledPin4 = 4;
const int ledPin5 = 5;
const int ledPin6 = 6;
const int lightSensorPin = A0;
int lightValue;

int relayPin = 10;
boolean relayToggle = false;

const int tmpSensorPin = A1;
int tempValue;

const int termoResistorPin = A2;
double termoValue;

dht DHT;

#define DHT22_PIN 8

void setup() {
  Serial.begin(9600);
  pinMode(ledPin2, OUTPUT);
  pinMode(relayPin, OUTPUT);

  setTime(1,23,00,22,3,18); // set time to Saturday 8:29:00am Jan 1 2011

  Alarm.alarmRepeat(1,24,0, toggleNeon);  // 8:30am every day
  Alarm.alarmRepeat(1,25,0, toggleNeon);  // 5:45pm every day 
  
  Alarm.timerRepeat(10, readTemp); 
  Alarm.timerRepeat(15, readDht); 
  Alarm.timerRepeat(5, toggleLeds); 
  Alarm.timerRepeat(2, readLight);  
}

void loop() {
  digitalClockDisplay();
  Alarm.delay(1000); // wait one second between clock display
}

void readLight() {
  lightValue = analogRead(lightSensorPin);
  Serial.print("Value light: \t");
  Serial.println(lightValue); // ????
}

void readTemp() {
  int tempSensorVal = analogRead(tmpSensorPin);
  float tempVoltage = (tempSensorVal / 1024.0) * 5.0;
  tempValue = (tempVoltage - .5) * 100;
  Serial.print("Voltage: \t");
  Serial.print(tempVoltage);
  Serial.print("\t Temp value: \t");
  Serial.println(tempValue);
  
  // termo resistenza
  termoValue = calcTemp(analogRead(A2), 3950, 2800);
  Serial.print("\t Termo value: \t");
  Serial.println(termoValue);
}

void readDht() {
  // DHT
  // READ DATA
  Serial.print("DHT22, \t");
  int chk = DHT.read22(DHT22_PIN);

  // DISPLAY DATA
  Serial.print(DHT.humidity, 1);
  Serial.print("%,\t");
  Serial.print(DHT.temperature, 1);
  Serial.println("°C");
}

void toggleNeon() {
  if (relayToggle == true) {
    digitalWrite(relayPin, LOW);
    relayToggle = false;
  }
  else {
    relayToggle = true;
    digitalWrite(relayPin, HIGH);
  }
}

void toggleLeds() {
  Serial.print("toggleLeds");
  if (ledToggle == false) {
    ledToggle = true;
    digitalWrite(ledPin2, HIGH);
    digitalWrite(ledPin3, HIGH);
    digitalWrite(ledPin4, HIGH);
    digitalWrite(ledPin5, HIGH);
    digitalWrite(ledPin6, HIGH);
  } else {
    ledToggle = false;
    digitalWrite(ledPin2, LOW);
    digitalWrite(ledPin3, LOW);
    digitalWrite(ledPin4, LOW);
    digitalWrite(ledPin5, LOW);
    digitalWrite(ledPin6, LOW);
  }
}

double calcTemp(int value, int B, int R0)
{
  double V=(5/1023.00)*value;
  double R=((10000.00*5)/V)-10000;
  double T=B/log(R/(R0*pow(M_E,(-B/298.15))));
  return T-273.15;
}

void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println(); 
}

void printDigits(int digits)
{
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

