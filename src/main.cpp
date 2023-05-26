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

bool checkDistance(void);


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
    uint8_t data[4];
    nfc.mifareultralight_ReadPage(4, data);

    

    for (int i = 0; i < 4; i++)
    {
      Serial.println(data[i]);
    }
    if (data[0] == 0)   
    {
      data[1] += amounts[setting];
      nfc.mifareultralight_WritePage(4, data);
    }

    

bool checkDistance(void){
  digitalWrite(D5, HIGH);
  delayMicroseconds(10);
  digitalWrite(D5, LOW);
  long duration = pulseIn(D6, HIGH);
  float distance = duration * 0.034 / 2;
  Serial.println("The distance is " + String(distance) + " cm.");
  //return distance <= 2.5;
  return distance <= 4.0;
}

    
    
    

    
    

    
  }
}