// include common libs
#include <SPI.h>
// include the DHT library
#include <dht.h>
// include the TimeAlarms library
#include <Time.h>
#include <TimeAlarms.h>
// include the WiFi library
#include <WiFi101.h>
// include the SD library
#include <SD.h>
// Required by Bluetooth module
#include <SoftwareSerial.h>
// include json library (https://github.com/bblanchon/ArduinoJson)
#include <ArduinoJson.h>

// --------------------- WIRING --------------------------
// -------------------------------------------------------
const int lightSensorPin = A0;
const int moistureSensorPin = A1;
const int relayNeon1Pin = 22;
const int relayNeon2Pin = 24;
const int relayFanPin = 26;
const int relayHumidifierPin = 28;
const int relayHeaterPin = 30;
#define DHT22_PIN 32
// -------------------------------------------------------
// ----------------- initial config ----------------------
const int hourNeonEnable[] = {18, 40, 00}; // FIXME change to 7:30am
const int hourNeonDisable[] = {18, 45, 00}; // FIXME change to 9:00pm
const int fanReadValDelay = 58; // seconds
const int humidifierReadValDelay = 58; // seconds
const int dhtReadValDelay = 3; // seconds
const int lightReadValDelay = 3; // seconds
const int moistureReadValDelay = 10; // seconds
// -------------------------------------------------------
// ---------------------- others -------------------------
const int serverPort = 80;
// -------------------------------------------------------
// -------------------------------------------------------

dht DHT;

int lightValue = 0;
int moistureValue = 0;
boolean relayFanToggle = true;
boolean relayHumidifierToggle = true;


#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
IPAddress server(35,206,99,222);  // numeric IP (no DNS)
//char server[] = "35.206.99.222";    // name address (using DNS)
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

  // enable SD SPI
  //digitalWrite(chipSelect, LOW);

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
  myFile = SD.open("data.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.println("-------Arduino restarted-------");
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

  // re-open the file for reading:
//  myFile = SD.open("test.txt");
//  if (myFile) {
//    Serial.println("test.txt:");
//
//    // read from the file until there's nothing else in it:
//    while (myFile.available()) {
//      Serial.write(myFile.read());
//    }
//    // close the file:
//    myFile.close();
//  } else {
//    // if the file didn't open, print an error:
//    Serial.println("error opening test.txt");
//  }
  // ------------------------------
  // ------------------------------
  // ------------------------------

  // disable SD SPI
  // digitalWrite(chipSelect, HIGH);

  // --------------------------------------------------------------------
  // ---------------- wifi + json rest api to remote server -------------
  // --------------------------------------------------------------------
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to wifi");
  printWiFiStatus();

  Serial.println("\nStarting connection to server...");
  
  if (!client.connect(server, serverPort)) {
    Serial.println("Connection failed");
    return;
  }

  Serial.println("Connected!");
  
  // Make a HTTP request:
  client.println("GET /api/keepAlive HTTP/1.1");
  client.println("Host: 35.206.99.222");
  client.println("Connection: close");

  if (client.println() == 0) {
    Serial.println("Failed to send request");
    return;
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print("Unexpected response: ");
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println("Invalid response");
    return;
  }


  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonBuffer jsonBuffer(capacity);

  // Parse JSON object
  JsonObject& root = jsonBuffer.parseObject(client);
  if (!root.success()) {
    Serial.println("Parsing failed!");
    return;
  }

  // Extract values
  Serial.println("Response:");
  Serial.println(root["year"].as<char*>());
  Serial.println(root["month"].as<char*>());
  Serial.println(root["day"].as<char*>());
  Serial.println(root["hours"].as<char*>());
  Serial.println(root["minutes"].as<char*>());
  Serial.println(root["seconds"].as<char*>());
  Serial.println(root["offset"].as<char*>());

  // Disconnect
  client.stop();
  // ------------------------------
  // ------------------------------
  // ------------------------------

  setTime(root["hours"], root["minutes"], root["seconds"],
    root["day"], root["month"], root["year"]); // set time (FIXME replace with a real time)

  Alarm.alarmRepeat(hourNeonEnable[0], hourNeonEnable[1], hourNeonEnable[3], enableNeons);
  Alarm.alarmRepeat(hourNeonDisable[0], hourNeonDisable[1], hourNeonDisable[3], disableNeons);

  Alarm.timerRepeat(fanReadValDelay, updateFan); // FIXME this should be every 15 minutes
  Alarm.timerRepeat(humidifierReadValDelay, updateHumidifier); // FIXME this should be every 15 minutes
  Alarm.timerRepeat(dhtReadValDelay, readDht);
  Alarm.timerRepeat(lightReadValDelay, readLight);
  Alarm.timerRepeat(moistureReadValDelay, readMoisture);

  // force heater always on, because managed by an internal thermostat
  digitalWrite(relayHeaterPin, HIGH);
}

void loop() {
  digitalClockDisplay();
  Alarm.delay(1000); // wait one second between clock display

  // ----------- Wifi -------------
  // printCurrentNet();
}

void readLight() {
  lightValue = analogRead(lightSensorPin);
  Serial.print("Value light: \t");
  Serial.println(lightValue);
  // writeToSdCard(String("Value light: ") + lightValue);
}

void readMoisture() {
  moistureValue = analogRead(moistureSensorPin);
  Serial.print("Value moisture: \t");
  Serial.println(moistureValue);
  // writeToSdCard(String("Value moisture: ") + moistureValue);
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

  //  String dataToWrite;
  //  dataToWrite.concat("DHT humidifier");
  //  dataToWrite.concat(DHT.humidity);
  //  dataToWrite.concat("%, ");
  //  dataToWrite.concat(DHT.temperature);
  //  dataToWrite.concat("°C");
  //  writeToSdCard(dataToWrite);
}

void enableNeons() {
  digitalWrite(relayNeon1Pin, HIGH);
  digitalWrite(relayNeon2Pin, HIGH);
  writeToSdCard("Neons enabled");
}

void disableNeons() {
  digitalWrite(relayNeon1Pin, LOW);
  digitalWrite(relayNeon2Pin, LOW);
  writeToSdCard("Neons disabled");
}

void updateFan() {
  if (relayFanToggle == true) {
    digitalWrite(relayFanPin, LOW);
    relayFanToggle = false;
    writeToSdCard("Update fan true");
  }
  else {
    relayFanToggle = true;
    digitalWrite(relayFanPin, HIGH);
    writeToSdCard("Update fan false");
  }
}

void updateHumidifier() {
  if (relayHumidifierToggle == true) {
    digitalWrite(relayHumidifierPin, LOW);
    relayHumidifierToggle = false;
    writeToSdCard("Update humidifier false");
  }
  else {
    relayHumidifierToggle = true;
    digitalWrite(relayHumidifierPin, HIGH);
    writeToSdCard("Update humidifier true");
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
void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
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

void writeToSdCard(String message) {
  Serial.println("writing to sdcard");

  // enable SD SPI
  digitalWrite(chipSelect, LOW);

  while (!SD.begin(chipSelect)) {
    Serial.println("initialization sdcard failed!");
    delay(1000);
    // while (1);
  }

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("data.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.println(message);
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
}
// ------------------------------
// ------------------------------
// ------------------------------
