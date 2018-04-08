// include the DHT library
#include <dht.h>
// include the TimeAlarms library
#include <Time.h>
#include <TimeAlarms.h>
// include the WiFi library
#include <SPI.h>
#include <WiFi101.h>
// include the SD library
#include <SPI.h>
#include <SD.h>
// Required by Bluetooth module
#include <SoftwareSerial.h>

const int lightSensorPin = A0;

const int relayNeon1Pin = 22;
const int relayNeon2Pin = 24;
const int relayFanPin = 26;
const int relayHumidifierPin = 28;
const int relayHeaterPin = 30;

boolean relayFanToggle = true;
boolean relayHumidifierToggle = true;

int lightValue;

#define DHT22_PIN 32
dht DHT;

#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status
// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
// MKRZero SD: SDCARD_SS_PIN
const int chipSelect = 4;

File myFile;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  pinMode(relayNeon1Pin, OUTPUT);
  pinMode(relayNeon2Pin, OUTPUT);
  pinMode(relayFanPin, OUTPUT);
  pinMode(relayHumidifierPin, OUTPUT);
  pinMode(relayHeaterPin, OUTPUT);

  // I don't know if this is necessary for sd card
  // In case of errors, please enable it
  // pinMode(4,OUTPUT);

  setTime(1, 23, 00, 22, 3, 18); // set time (FIXME replace with a real time)

  Alarm.alarmRepeat(1, 23, 20, enableNeons); // FIXME change to 7:30am
  Alarm.alarmRepeat(1, 24, 5, disableNeons); // FIXME change to 9:00pm

  Alarm.timerRepeat(58, updateFan); // FIXME this should be every 15 minutes
  Alarm.timerRepeat(58, updateHumidifier); // FIXME this should be every 15 minutes
  Alarm.timerRepeat(3, readDht);
  Alarm.timerRepeat(3, readLight);

  // force heater always on, because managed by an internal thermostat
  digitalWrite(relayHeaterPin, HIGH);

  // enable SD SPI
  digitalWrite(chipSelect, LOW);

  // ------------------------------
  // ----------- SD card ----------
  // ------------------------------
  getSdCardData();
  while (!SD.begin(chipSelect)) {
    Serial.println("initialization sdcard failed!");
    delay(1000);
    // while (1);
  }
  Serial.println("initialization sdcard done.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("test.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.println("testing 1, 2, 3.");
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

  // re-open the file for reading:
  myFile = SD.open("test.txt");
  if (myFile) {
    Serial.println("test.txt:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
  // ------------------------------
  // ------------------------------
  // ------------------------------

  // disable SD SPI
  digitalWrite(chipSelect, HIGH);

  // ------------------------------
  // ----------- Wifi -------------
  // ------------------------------
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network");
  printCurrentNet();
  printWiFiData();
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  char server[] = "http://api.openweathermap.org";
  if (client.connect(server, 80)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.println("GET /data/2.5/weather?q=paderno&APPID=7c15db211783f681d69a8b0a46913857 HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
  }
  // ------------------------------
  // ------------------------------
  // ------------------------------
}

void loop() {
  digitalClockDisplay();
  Alarm.delay(1000); // wait one second between clock display

  // ----------- Wifi -------------
  printCurrentNet();
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
    digitalWrite(relayFanPin, LOW);
    relayFanToggle = false;
  }
  else {
    relayFanToggle = true;
    digitalWrite(relayFanPin, HIGH);
  }
}

void updateHumidifier() {
  if (relayHumidifierToggle == true) {
    digitalWrite(relayHumidifierPin, LOW);
    relayHumidifierToggle = false;
  }
  else {
    relayHumidifierToggle = true;
    digitalWrite(relayHumidifierPin, HIGH);
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


// ------------------------------
// ----------- Wifi -------------
// ------------------------------
void printWiFiData() {
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);
}
void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  Serial.print(bssid[5], HEX);
  Serial.print(":");
  Serial.print(bssid[4], HEX);
  Serial.print(":");
  Serial.print(bssid[3], HEX);
  Serial.print(":");
  Serial.print(bssid[2], HEX);
  Serial.print(":");
  Serial.print(bssid[1], HEX);
  Serial.print(":");
  Serial.println(bssid[0], HEX);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}
// ------------------------------
// ------------------------------
// ------------------------------

// ------------------------------
// ---------- SC Card -----------
// ------------------------------
void getSdCardData() {
  Serial.print("\nInitializing SD card...");

  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    while (1);
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  // print the type of card
  Serial.println();
  Serial.print("Card type:         ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    while (1);
  }

  Serial.print("Clusters:          ");
  Serial.println(volume.clusterCount());
  Serial.print("Blocks x Cluster:  ");
  Serial.println(volume.blocksPerCluster());

  Serial.print("Total Blocks:      ");
  Serial.println(volume.blocksPerCluster() * volume.clusterCount());
  Serial.println();

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("Volume type is:    FAT");
  Serial.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
  Serial.print("Volume size (Kb):  ");
  Serial.println(volumesize);
  Serial.print("Volume size (Mb):  ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Gb):  ");
  Serial.println((float)volumesize / 1024.0);
}
// ------------------------------
// ------------------------------
// ------------------------------
