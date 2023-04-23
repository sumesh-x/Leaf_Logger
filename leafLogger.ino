uint8_t address[6] = { 0x13, 0xE0, 0x2F, 0x8D, 0x5F, 0x5E };


#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

#define DEBUG_PORT Serial
#define ELM_PORT SerialBT
#define ARRAY_LENGTH 12
#define HEX_STRING_LENGTH 2
#define T 50  //50 klappt


#include <stdio.h>
#include <string.h>

// GPS Definitions
#define GPS_RX_PIN 4
#define GPS_TX_PIN 2
HardwareSerial gpsSerial(1);
TinyGPSPlus gps;

// GPS Variables
float latitude = 0.0;
float longitude = 0.0;
float altitude = 0.0;
float GpsSpeed = 0.0;
int satellites = 0;
unsigned long date = 0;
unsigned long GpsTime = 0;

//OBD Vars
int temp = 0;
int speed = 0;
int odometer = 0;
int gear = 0;
int quickCharge = 0;
int l1l2Charge = 0;
int chargeMode = 0;
int obcPwrOut = 0;
int motorPwr = 0;
int range = 0;
int soh = 0;
int accPedal = 0;
int torque = 0;
int soc = 0;
int hx = 0;
int ahr = 0;
int hvBatC1 = 0;
int hvBatC2 = 0;
int hvBatVol = 0;

String response = "";



char folderName[10];
char fileName[10];
char folderPath[20] = "/";
char filePath[20] = "";


bool folderCreated = false;
const char* CsvCol = "GpsTime,latitude,longitude,altitude,GpsSpeed,satellites,temp,speed,odometer,range,soh,soc,hx,ahr,hvBatC1,hvBatC2,hvBatVol\n";

#define LED_BUILTIN 14


void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  // Initialisierung der SD-Karte und Anlegen einer CSV Datei. Daten sollen im LOOP auf die Datei filePath beschriebnen werden.
  
  if (!SD.begin()) {
    Serial.println("SD Error");
    while(true){
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);        
    }
  
  }

  setFile();


  // Baue Verbindung zum OBD2 Gerät auf
  DEBUG_PORT.begin(9600);
  ELM_PORT.begin("ESP32test", true);

  // false setzen, falls keine verbindung zum OBD2 aufgebaut werden soll
  if (true) {
    if (!ELM_PORT.connect(address)) {
      DEBUG_PORT.println("Couldn't connect to OBD scanner");
      while(true){
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(2000);   
      }
  
    }
  }

  preConfig();
}

void loop() {

  digitalWrite(LED_BUILTIN, HIGH);

  unsigned long startTime = millis();

  // GPS Daten auslesen
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isValid()) {
    // Speichern der GPS-Daten in Variablen
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    altitude = gps.altitude.meters();
    GpsSpeed = gps.speed.kmph();
    satellites = gps.satellites.value();
    GpsTime = gps.time.value();
  }else{
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
    digitalWrite(LED_BUILTIN, HIGH);
  }


  // OBD Größen auslesen
  for (int i = 0; i < 2; i++) {
    temp = getAmbient_C_Temp();
    delay(10);
  }

  for (int i = 0; i < 2; i++) {
    speed = getSpeed();
    delay(T);
  }

  for (int i = 0; i < 2; i++) {
    odometer = getOdometer();
    delay(T);
  }

  for (int i = 0; i < 2; i++) {
    range = getRange();
    delay(30);
  }
  for (int i = 0; i < 2; i++) {
    soh = getSOH();
    delay(T);
  }


  for (int i = 0; i < 2; i++) {
    response = getLbcResponse();
    delay(70);
  }


  soc = getSOC(response);
  hx = getHX(response);
  ahr = getAhr(response);
  hvBatC1 = getHVBatCurrent1(response);
  hvBatC2 = getHVBatCurrent2(response);
  hvBatVol = getHVBatVolt(response);

  unsigned long endTime = millis();  // Endzeit erfassen


  char GPSString[50];
  sprintf(GPSString, "%lu,%f,%f,%f,%f,%d", GpsTime, latitude, longitude, altitude, GpsSpeed, satellites);
  char OBDString[200];                                                                                                                  // speichert maximal 200 Zeichen inklusive dem Null-Byte
  sprintf(OBDString, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", temp, speed, odometer, range, soh, soc, hx, ahr, hvBatC1, hvBatC2, hvBatVol);  // umwandeln


  if (true) {
    Serial.print(GPSString);
    Serial.print(",");
    Serial.println(OBDString);
  }


  appendFile(SD, filePath, GPSString);
  appendFile(SD, filePath, ",");
  appendFile(SD, filePath, OBDString);
  appendFile(SD, filePath, "\n");
}

/// Helper Funktionen

void createDir(fs::FS& fs, const char* path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}


void writeFile(fs::FS& fs, const char* path, const char* message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}


void appendFile(fs::FS& fs, const char* path, const char* message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void setFile() {
  while (!folderCreated) {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(100);   
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }

    if (gps.location.isValid()) {
      date = gps.date.value();
      GpsTime = gps.time.value();
      Serial.println(GpsTime);

      sprintf(folderName, "%lu", date);
      sprintf(fileName, "%lu", GpsTime);

      strcat(folderPath, folderName);

      strcat(filePath, folderPath);
      strcat(filePath, "/");
      strcat(filePath, fileName);
      strcat(filePath, ".csv");

      createDir(SD, folderPath);
      //TODO: ADD Column String here
      writeFile(SD, filePath, CsvCol);
      folderCreated = true;
    }
  }
}

String resetELM() {
  ELM_PORT.println("ATZ");
  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }
  return response;
}

String getElmBrand() {

  ELM_PORT.println("VTI");
  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }
  delay(T);
  return response;
}

void preConfig() {

  ELM_PORT.println("ATZ");
  delay(1000);

  ELM_PORT.println("ATE0");  //turning echo off.
  delay(T);
  ELM_PORT.println("ATH1");  // activating header.
  delay(T);
  ELM_PORT.println("ATSP6");  // reset protocol to 6
  delay(T);
  ELM_PORT.println("ATS0");  // turning off auto connection to obds
  delay(T);
  ELM_PORT.println("ATM0");  // turning off auto mode picking for vh
  delay(T);
  ELM_PORT.println("ATAT1");  // turning off adaptive timing
  delay(T);
}

void setHeaders(String queryHeader, String answerHeader) {
  String a = "ATSH" + queryHeader;
  String b = "ATFCSH" + queryHeader;
  String c = "ATFCSD 300000";
  String d = "ATFCSM1";
  String e = "ATCRA" + answerHeader;
  int dly = 50;  //50 funktioniert
  //ELM_PORT.println("ATH0");
  //delay(T);
  ELM_PORT.println("ATH1");  // activating header. zwingend erforderlich!!
  delay(T);
  ELM_PORT.println(a);
  delay(dly);
  ELM_PORT.println(b);
  delay(dly);
  ELM_PORT.println(c);
  delay(dly);
  ELM_PORT.println(d);
  delay(dly);
  ELM_PORT.println(e);
  delay(dly);
}

unsigned char getData(char data[], int id) {
  int a = 3 + id * 2;
  int b = a + 1;

  char c1 = data[a];
  char c2 = data[b];

  char str[3];
  str[0] = c1;
  str[1] = c2;
  str[2] = '\0';
  unsigned long hexValue = strtoul(str, NULL, 16);
  unsigned char result = (unsigned char)hexValue;
  return result;
}


unsigned char getDataFromLine(char data[], int id, int line) {
  int a = 3 + id * 2 + 20 * line;
  int b = a + 1;

  char c1 = data[a];
  char c2 = data[b];

  char str[3];
  str[0] = c1;
  str[1] = c2;
  str[2] = '\0';
  unsigned long hexValue = strtoul(str, NULL, 16);
  unsigned char result = (unsigned char)hexValue;
  return result;
}

int getSpeed() {

  setHeaders("797", "79A");
  ELM_PORT.println("22121A");

  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }


  //Serial.println(response);
  //String response = "79A0562121A00010000";
  const int length = response.length();
  char* data1 = new char[length + 1];
  strcpy(data1, response.c_str());

  unsigned char a = getData(data1, 4);
  unsigned char b = getData(data1, 5);


  int result = ((a << 8) | b) / 10;

  delete[] data1;
  return result;
}

int getGear() {

  setHeaders("797", "79A");
  ELM_PORT.println("03221156");

  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }
  // delay(T);

  //String response = "76306620E01000264";
  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getData(data, 4);

  int result = a;

  delete[] data;
  return result;
}

int getQuick_Charge() {

  setHeaders("797", "79A");
  ELM_PORT.println("03220E01");

  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }
  // delay(T);

  //String response = "76306620E01000264";
  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getData(data, 4);
  unsigned char b = getData(data, 5);

  int result = (a << 8) | b;

  delete[] data;
  return result;
}

int getL1_L2_Charge() {

  setHeaders("797", "79A");
  ELM_PORT.println("03221205");

  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }
  //delay(T);

  //String response = "76306620E01000264";
  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getData(data, 4);
  unsigned char b = getData(data, 5);

  int result = (a << 8) | b;

  delete[] data;
  return result;
}

int getAmbient_C_Temp() {
  setHeaders("797", "79A");
  ELM_PORT.println("22115D");

  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }
  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getData(data, 4);

  int preRes = (a * 0.9) - 40.9;
  int result = (preRes - 32) * 5 / 9;
  delete[] data;
  return result;
}

int getCharge_Mode() {

  setHeaders("797", "79A");
  ELM_PORT.println("0322114E");

  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }
  //delay(T);

  //String response = "76306620E01000264";
  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getData(data, 4);


  int result = a;

  delete[] data;
  return result;
}

int getOBC_Out_Pwr() {

  setHeaders("797", "79A");
  ELM_PORT.println("03221236");

  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }
  // delay(T);

  //String response = "76306620E01000264";
  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getData(data, 4);
  unsigned char b = getData(data, 5);


  int result = ((a << 8) | b) * 100;

  delete[] data;
  return result;
}

int getMotor_Pwr() {

  setHeaders("797", "79A");
  ELM_PORT.println("03221146");

  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }
  //delay(T);

  //String response = "76306620E01000264";
  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getData(data, 4);
  unsigned char b = getData(data, 5);

  int result = ((a << 8) | b) * 40;

  delete[] data;
  return result;
}

int getRange() {

  setHeaders("743", "763");
  ELM_PORT.println("220E24");

  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }


  //String response = "76306620E01000264";
  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getData(data, 4);
  unsigned char b = getData(data, 5);

  int result = ((a << 8) | b) / 10;

  delete[] data;
  return result;
}

int getSOH() {

  setHeaders("79B", "7BB");
  ELM_PORT.println("022161");

  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }
  //delay(T);

  //String response = "76306620E01000264";
  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getData(data, 6);
  unsigned char b = getData(data, 7);


  int result = ((a << 8) | b) / 100;

  delete[] data;
  return result;
}

int getAcc_Pedal() {

  setHeaders("740", "760");
  ELM_PORT.println("03221115");

  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }
  //delay(T);

  //String response = "76306620E01000264";
  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getData(data, 4);

  int result = a;

  delete[] data;
  return result;
}

int getTorque() {

  setHeaders("784", "78C");
  ELM_PORT.println("03221225");

  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }
  //delay(T);

  //String response = "76306620E01000264";
  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getData(data, 4);
  unsigned char b = getData(data, 5);


  int result = (a << 8) | b;
  if (result & 32768 == 32768) {
    result = result | -65536;
    result = result / 64.0;
  }

  delete[] data;
  return result;
}

int getOdometer() {

  setHeaders("743", "763");
  ELM_PORT.println("220E01");


  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }

  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getData(data, 4);
  unsigned char b = getData(data, 5);
  unsigned char c = getData(data, 6);

  int result = (a << 16 | ((b << 8) | c));

  delete[] data;
  return result;
}


String getLbcResponse() {

  setHeaders("79B", "7BB");
  ELM_PORT.println("2101");

  String response = "";
  while (ELM_PORT.available()) {
    char c = ELM_PORT.read();
    response += c;
  }
  return response;
}


int getSOC(String response) {

  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getDataFromLine(data, 7, 4);
  unsigned char b = getDataFromLine(data, 1, 5);
  unsigned char c = getDataFromLine(data, 2, 5);

  int result = (a << 16 | ((b << 8) | c)) / 10000;

  delete[] data;
  return result;
}

int getHX(String response) {

  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getDataFromLine(data, 4, 4);
  unsigned char b = getDataFromLine(data, 5, 4);


  int result = ((a << 8) | b) / 102.4;

  delete[] data;
  return result;
}

int getAhr(String response) {

  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getDataFromLine(data, 4, 5);
  unsigned char b = getDataFromLine(data, 5, 5);
  unsigned char c = getDataFromLine(data, 6, 5);

  int result = (a << 16 | ((b << 8) | c)) / 10000;

  delete[] data;
  return result;
}

int getHVBatCurrent1(String response) {

  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getDataFromLine(data, 4, 0);
  unsigned char b = getDataFromLine(data, 5, 0);
  unsigned char c = getDataFromLine(data, 6, 0);
  unsigned char d = getDataFromLine(data, 7, 0);

  int result = (a << 24) | (b << 16 | ((c << 8) | d));
  if (result & 0x8000000 == 0x8000000) {
    result = (result | -0x100000000) / 1024;
  } else {
    result = result / 1024;
  }

  delete[] data;
  return result;
}


int getHVBatCurrent2(String response) {

  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getDataFromLine(data, 3, 1);
  unsigned char b = getDataFromLine(data, 4, 1);
  unsigned char c = getDataFromLine(data, 5, 1);
  unsigned char d = getDataFromLine(data, 6, 1);

  int result = (a << 24) | (b << 16 | ((c << 8) | d));
  if (result & 0x8000000 == 0x8000000) {
    result = (result | -0x100000000) / 1024;
  } else {
    result = result / 1024;
  }

  delete[] data;
  return result;
}

int getHVBatVolt(String response) {

  const int length = response.length();
  char* data = new char[length + 1];
  strcpy(data, response.c_str());

  unsigned char a = getDataFromLine(data, 1, 3);
  unsigned char b = getDataFromLine(data, 2, 3);


  int result = ((a << 8) | b) / 100;

  delete[] data;
  return result;
}
