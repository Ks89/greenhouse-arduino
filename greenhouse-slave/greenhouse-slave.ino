#include <dht.h>
#include <Time.h>
#include <TimeAlarms.h>

const int lightSensorPin = A0;

const int relayNeon1Pin = 4;
const int relayNeon2Pin = 5;
const int relayFanPin = 6;
boolean relayFanToggle = false;

int lightValue;

#define DHT22_PIN 8
dht DHT;


void setup() {
  Serial.begin(9600);
  pinMode(relayNeon1Pin, OUTPUT);
  pinMode(relayNeon2Pin, OUTPUT);
  pinMode(relayFanPin, OUTPUT);

  setTime(1, 23, 00, 22, 3, 18); // set time (FIXME replace with a real time)

  Alarm.alarmRepeat(1, 23, 20, enableNeons); // FIXME change to 7:30am
  Alarm.alarmRepeat(1, 24, 5, disableNeons); // FIXME change to 9:00pm

  Alarm.timerRepeat(58, updateFan); // FIXME this should be every 15 minutes
  Alarm.timerRepeat(3, readDht);
  Alarm.timerRepeat(3, readLight);
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

void enableNeons() {
  digitalWrite(relayNeon1Pin, HIGH);
  digitalWrite(relayNeon2Pin, HIGH);
}

void disableNeons() {
  digitalWrite(relayNeon1Pin, LOW);
  digitalWrite(relayNeon2Pin, LOW);
}

void updateFan() {
  if (relayFanToggle == true) {
    digitalWrite(relayFanPin, HIGH);
    relayFanToggle = false;
  }
  else {
    relayFanToggle = true;
    digitalWrite(relayFanPin, HIGH);
  }
}


void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println();
}

void printDigits(int digits) {
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

