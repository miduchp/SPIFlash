/*
 *----------------------------------------------------------------------------------------------------------------------------------*
 |                                                            Winbond Flash                                                         |
 |                                                      SPIFlash library test v2.1.0                                                |
 |----------------------------------------------------------------------------------------------------------------------------------|
 |                                                                Marzogh                                                           |
 |                                                              08.10.2015                                                          |
 |----------------------------------------------------------------------------------------------------------------------------------|
 */
#include<SPIFlash.h>
#include<SPI.h>

#define cs 10

int strPage, strSize;
byte strOffset;

SPIFlash flash(cs);

void setup() {
  Serial.begin(115200);
  uint16_t tempPage;
  uint8_t tempOffset;
  flash.begin();
  /*flash.getAddress(sizeof(byte), tempPage, tempOffset);
  Serial.print(F("Page number for byte: "));
  Serial.println(tempPage);
  Serial.print(F("Offset for byte: "));
  Serial.println(tempOffset);
  Serial.println();
  flash.getAddress(sizeof(int), tempPage, tempOffset);
  Serial.print(F("Page number for int: "));
  Serial.println(tempPage);
  Serial.print(F("Offset for int: "));
  Serial.println(tempOffset);
  Serial.println();
  flash.getAddress(sizeof(long), tempPage, tempOffset);
  Serial.print(F("Page number for long: "));
  Serial.println(tempPage);
  Serial.print(F("Offset for long: "));
  Serial.println(tempOffset);
  Serial.println();
  flash.getAddress(sizeof(float), tempPage, tempOffset);
  Serial.print(F("Page number for float: "));
  Serial.println(tempPage);
  Serial.print(F("Offset for float: "));
  Serial.println(tempOffset);
  Serial.println();
  flash.getAddress(sizeof(int), tempPage, tempOffset);
  Serial.print(F("Page number for int: "));
  Serial.println(tempPage);
  Serial.print(F("Offset for int: "));
  Serial.println(tempOffset);
  Serial.println();*/

  Serial.println(F("Please type the string into the console"));
  randomSeed(analogRead(A0));
  strPage = random(0, 4095);
  strOffset = random(0, 255);
<<<<<<< HEAD
  String inputString;// = "Test String!! :D";
  while (!readSerialStr(inputString));
  Serial.println(flash.writeStr(strPage, strOffset, inputString));
=======
  String inputString;
  while (!readSerialStr(inputString));
  flash.writeStr(strPage, strOffset, inputString);
>>>>>>> master
  Serial.print(F("Written string: "));
  Serial.print(inputString);
  Serial.print(F(" to page "));
  Serial.print(strPage);
  Serial.print(F(", at offset "));
  Serial.println(strOffset);
  String outputString = "";
  flash.readStr(strPage, strOffset, outputString);
  Serial.print(F("Read string: "));
  Serial.print(outputString);
  Serial.print(F(" from page "));
  Serial.print(strPage);
  Serial.print(F(", at offset "));
  Serial.println(strOffset);
  flash.eraseSector(strPage, 0);
}

void loop() {

}

//Reads a string from Serial
bool readSerialStr(String &inputStr) {
<<<<<<< HEAD
  if (!Serial)
    Serial.begin(115200);
  while (Serial.available()) {
    inputStr = Serial.readStringUntil('\n');
    return true;
=======
  if(!Serial)
    Serial.begin(115200);
  while (Serial.available()) {
      inputStr = Serial.readStringUntil('\n');
      return true;
>>>>>>> master
  }
  return false;
}
