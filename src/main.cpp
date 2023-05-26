#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Servo.h>

Adafruit_PN532 nfc(-1, -1);
void checkNFC();
Servo setServo, dispenseServo;
int setting[] = {100, 200, 300, 500, 700, 1000, 0};
int currentSetting = 2;
int keys[] = {4, 101, 21, 69};

bool checkDistance(void);
void pressSet(int);
void pressDispense(void);
void incSetting(void);
bool authenticate(uint8_t* auth);


void setup() {
  Serial.begin(115200);
  pinMode(D5, OUTPUT);
  pinMode(D6, INPUT);
  setServo.attach(D3);
  dispenseServo.attach(D4);
  setServo.write(90);
  dispenseServo.write(90);

  nfc.begin();
  if (! nfc.getFirmwareVersion()) {
    Serial.print("Didn't find PN53x board");
    while (true) { delay(1); }
  }
}

void loop() {
  checkNFC();
  delay(2000);
}


void checkNFC()
{
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t auth[4]; // buffer for the authentication page, page 4
  uint8_t p6[4];
  uint8_t p5[4]; // buffer for data read from page 5, interpreted differently depending on byte 0:
  // data[0] == 0 -> data[1] contains the amount of water already saved, read -> increase -> write back
  // data[0] == 1 -> might be useful later, in case the water source is refilled, to be implemented

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) 
  {
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    for (int i = 0; i < uidLength; i++)
    {
      Serial.println(uid[i]);
    }
    
    nfc.mifareultralight_ReadPage(4, auth);
    Serial.println("Contents of Page 4:");
    for (int i = 0; i < 4; i++)
    {
      Serial.println(auth[i]);
    }

    if (! authenticate(auth))
    {
      Serial.println("not an Oasys signature, refusing interaction");
      return; //end here, wait for a proper card/app
    }

    //if this goes through, we can get to work

    nfc.mifareultralight_ReadPage(5, p5);
  }
}

bool checkDistance(void){
  digitalWrite(D5, HIGH);
  delayMicroseconds(10);
  digitalWrite(D5, LOW);
  long duration = pulseIn(D6, HIGH);
  float distance = duration * 0.034 / 2;
  Serial.println("The distance is " + String(distance) + " cm.");
  return distance <= 2.5;
  //return distance <= 4.0;
}

void incSetting(void) {
  currentSetting = (currentSetting == 6) ? 0 : currentSetting + 1;
}

void pressSet(int target){

  // press
  while (currentSetting != target)
  {

    incSetting; // increasing the setting


    // motion of pushing the button
    for(int pos = 95; pos <= 145; pos += 5){
    setServo.write(pos);
    delay(2);
  }
  for(int pos = 145; pos >= 90; pos -= 5){
    setServo.write(pos);
    delay(2);
  }
  // notify for debugging
  Serial.println("Settings button pressed, now in setting" + currentSetting);
  }
  
}

void pressDispense(void){
  for(int pos = 85; pos >= 45; pos -= 5){
    dispenseServo.write(pos);
    delay(2);
  }

  for(int pos = 45; pos <= 90; pos += 5){
    dispenseServo.write(pos);
    delay(2);
  }
}

bool authenticate(uint8_t* auth) {
  for (int i = 0; i < 4; i++)
    {
      if (auth[i] != keys[i])
      {
        return false;
      }
      return true;
      
    }
}