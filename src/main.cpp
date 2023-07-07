#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

Adafruit_PN532 nfc(-1, -1);
Adafruit_SSD1306 display(128,64, &Wire, -1);

void checkNFC();

Servo setServo, dispenseServo;
int settings[] = {1, 2, 3, 5, 7, 1, 0};
int currentSetting = 2;
int keys[] = {4, 101, 21, 69};
int source = 0;

bool checkDistance(void);
void pressSet(int);
void pressDispense(uint8_t);
void incSetting(void);
bool authenticate(uint8_t* auth);
void buzz(void);
void displayOled(String);
void displayMultiple(String[]);
int lengthArray(String[]);

void setup() {
  Serial.begin(115200);
  pinMode(D5, OUTPUT);
  pinMode(D6, INPUT);
  pinMode(D7, OUTPUT);
  digitalWrite(D7, LOW);
  setServo.attach(D3);
  dispenseServo.attach(D4);
  setServo.write(90);
  dispenseServo.write(90);

  nfc.begin();
  if (! nfc.getFirmwareVersion()) {
    Serial.print("Didn't find PN53x board");
    while (true) { delay(1); }
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed!");
    while(true); // Don't proceed forever
  }


 
}

void loop() {
  String strArray1[] = {"Please present card", "Source at: " + String(source) + "ml"}; 
  if(source > 0){ 
  displayMultiple(strArray1);
  } else{ 
    displayOled("Please refill");
  }
  checkNFC();
  delay(2000);
}


void checkNFC()
{
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t auth[4]; // buffer for the authentication page, page 4
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
      displayOled("Wrong card!");
      return; //end here, wait for a proper card/app
    }

    displayOled("Card recognized!");

    //if this goes through, we can get to work

    nfc.mifareultralight_ReadPage(5, p5);
    // case to dispense some water
    if (p5[0] == 0)
    {
      if(source == 0){
        buzz();
        buzz();
        buzz();
        return;
      }
      p5[2] += (source >= settings [p5[1]]) ? settings[p5[1]] : source; //inc with amount stored in target setting
      Serial.println("water-counter has been increased to " + String(p5[2]));
    nfc.mifareultralight_WritePage(5, p5); //writeback done, now we can take our time
    buzz(); // notify that we are done interacting with the card
    
    pressSet(p5[1]);
    pressDispense(p5[2]);
    }


    // placeholder for case p5[0] == 1
    
    if (p5[0] == 1)
    {
      source = p5[1];
      displayOled("Source refilled");
      delay(1000);
    }
    
  }
}

bool checkDistance(void){
  digitalWrite(D5, HIGH);
  delayMicroseconds(10);
  digitalWrite(D5, LOW);
  long duration = pulseIn(D6, HIGH);
  float distance = duration * 0.034 / 2;
  Serial.println("The distance is " + String(distance) + " cm.");
  return distance <= 3;
  //return distance <= 4.0;
}

void incSetting(void) {
  currentSetting = (currentSetting == 6) ? 0 : currentSetting + 1;
}

void pressSet(int target){

  // press
  while (currentSetting != target)
  {

    incSetting(); // increasing the setting


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
  Serial.println("Settings button pressed, now in setting " + String(currentSetting));
  }
  
}

void pressDispense(uint8_t total){

  while(!checkDistance()) {
    //wait
    Serial.println("Waiting for glass to be placed");
    delay(200);
  }

  delay(500); // wait half a sec to avoid shooting immediately when a glass is discovered

  for(int pos = 85; pos >= 45; pos -= 5){
    dispenseServo.write(pos);
    delay(2);
  }

  for(int pos = 45; pos <= 90; pos += 5){
    dispenseServo.write(pos);
    delay(2);
  }  

  if(settings[currentSetting] * 100 > source){
    String strArray2[] = {"Dispensed amount:",  String(source) + " ml"};
    displayMultiple(strArray2);
    delay(3000);
    displayOled("New total: " + String(total * 100)  + " ml");
    delay(3000);
    source = 0;
    displayOled("Source at: " + String(source) + " ml");
    return;
  }
  else {
    source -= settings[currentSetting] * 100;
    String strArray3[] = {"Dispensed amount:",  String(settings[currentSetting] * 100) + " ml"};
    displayMultiple(strArray3);
    delay(3000);
    displayOled("New total: " + String(total * 100)  + " ml");
    delay(3000);
    displayOled("Source at: " + String(source) + " ml");
  }
  

  
  //delay(10000); // idk how long it takes to fill a glass, lets wait a bit to avoid going crazy
}

bool authenticate(uint8_t* auth) {
  for (int i = 0; i < 4; i++)
    {
      if (auth[i] != keys[i])
      {
        return false;
      }
      
    }
    return true;
}

void buzz() {
  digitalWrite(D7, HIGH);
  Serial.println("The buzzer makes a sound");
  delay(100);
  digitalWrite(D7, LOW);
}

void displayOled(String str){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,28);
  display.println(str);
  display.display();
}

int lengthArray(String strArray[]){
  int length = 0;
  while(strArray[length] != NULL){
    length++;
  }
  return length;
}

void displayMultiple(String strArray[]){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,28);
  for(int i = 0; i < lengthArray(strArray); i++){
    display.println(strArray[i]);
  }
  display.display();
}