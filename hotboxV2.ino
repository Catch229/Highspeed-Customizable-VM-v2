//HB v2 ATMEL 32u4 based voltmeter
//Designed for highspeed updates and to be user customizable.
//Optimized for minimal memory usage due to large buffer for OLED screen.

#include <extEEPROM.h>
#include <U8g2lib.h>
#include <U8x8lib.h>

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

extEEPROM myEEPROM(kbits_32, 1, 64, 0x50);

uint8_t logo[1024] = {};
uint8_t banner[256] = {};
byte configData[5];
const uint8_t letterV[] PROGMEM = {
0x0f, 0xf0, 0x0f, 0xf0, 0x1f, 0xf8, 0x1e, 0x78, 0x1e, 0x78, 0x3e, 0x7c, 0x3c, 0x3c, 0x3c, 0x3c,
0x78, 0x1e, 0x78, 0x1e, 0xf8, 0x1f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xe0, 0x07, 0xe0, 0x07
};

float voltageOffset, lowAlarm, highAlarm, rawVoltage;
int drawColor = 1;

void setup() {
  
  pinMode(2, INPUT);

  myEEPROM.begin(myEEPROM.twiClock100kHz);
  u8g2.setI2CAddress(0x78);
  u8g2.begin();
  u8g2.setFontMode(1);
  //u8g2.setDisplayRotation(U8G2_MIRROR);

    myEEPROM.read(1280, (byte *)configData, 5);
    voltageOffset = ((float)atoi(configData))/100;

    myEEPROM.read(1285, (byte *)configData, 5);
    lowAlarm = ((float)atoi(configData))/100;
    
    myEEPROM.read(1290, (byte *)configData, 5);
    highAlarm = ((float)atoi(configData))/100;
  
  int programMode = analogRead(2);
  delay(200);
  if (programMode < 300) {
    
    //digitalWrite(13, HIGH);
    Serial.begin(9600);
    u8g2.firstPage();  
    do {
      drawProgramming();
    } while( u8g2.nextPage() );
    while (true) {
      serialToEEPROM();
      delay(200);
    }
    
  } else {

    eepromToArray(0, 1024, logo);
    eepromToArray(1024, 256, banner);
    
    int timer = 200;
    int t = 0;
    u8g2.firstPage();
    while(t < timer){
      do{
          drawBootScreen();
          t++;
      } while(u8g2.nextPage());
    }
    
  }
}

void loop() {
  u8g2.firstPage();    //Starts Loop
  rawVoltage = (analogRead(2));  //Softens readings
  delay(5);
  rawVoltage = ((rawVoltage + analogRead(2))/2);       //Sets reading to an 11-bit percision
  
  rawVoltage = rawVoltage*(0.02115234375)+ voltageOffset;
  
  if(rawVoltage < lowAlarm || rawVoltage > highAlarm) {
    drawColor = abs(drawColor-1);   
  } else {
    drawColor = 1;
  }
  
  do {              //Now with data draw to the screen
    drawVoltage(rawVoltage);
  } while(u8g2.nextPage());
}

void serialToEEPROM() {
  int dataByte; //Byte helps determine what is being sent
  unsigned int addr = 0;
  int firstSize = 1024; //Size of first block of data to determine second location
  int secondSize = 256+firstSize; //Size of second block of data to determine third location
  
  if (Serial.peek() > 0) {
    dataByte = Serial.read();
    Serial.print(dataByte);
    delay(100);
    int i = 0;
    int dataSize = 1;
    while (i < dataSize){
      
      switch (dataByte) {
        case 48:
          myEEPROM.write(addr, Serial.read());
          i++;
          dataSize = 1024;
          break;
        case 49:
          myEEPROM.write(firstSize+addr, Serial.read());
          i++;
          dataSize = 256;
          break;
        case 50:
          Serial.readStringUntil('+').toCharArray(configData, 5);
          myEEPROM.write(secondSize+addr*5, (byte *)configData, 5);
          i++;
          dataSize = 1;
          break;
        case 51:
          Serial.readStringUntil('+').toCharArray(configData, 5);
          myEEPROM.write(secondSize+5+addr*5, (byte *)configData, 5);
          i++;
          dataSize = 2;
          break;
      }
      
      addr++;
      //delay(1);
    }
    Serial.println("Done writing.");
  }
}

void eepromToArray(unsigned int startAddr, int len, uint8_t a[]) {
  for (int i = 0; i < len; i++){
    a[i] = myEEPROM.read(i+startAddr);
  }
}

void drawProgramming() {
  u8g2.setFont(u8g_font_profont17);
  u8g2.drawStr(15,12, "PROGRAMMING");
  u8g2.drawStr(47,25, "MODE");
  u8g2.setFont(u8g_font_helvR08);
  u8g2.drawStr(8,60, "HV: ");
  u8g2.setCursor(28,60);
  u8g2.print(highAlarm);
  u8g2.drawStr(72,60, "LV: ");
  u8g2.setCursor(92,60);
  u8g2.print(lowAlarm);
  u8g2.drawStr(24,46, "V Offset: ");
  u8g2.setCursor(72,46);
  u8g2.print(voltageOffset);
}

void drawBootScreen() {
  //Code to read bootscreen from EEPROM will go here
  u8g2.drawBitmap(0,0,16,64,logo);
}

void drawVoltage(double voltage) {
  u8g2.setDrawColor(1);
  u8g2.drawBitmap(0,0,16,16,banner); //Top banner
  u8g2.drawXBMP( 112, 47, 16, 16, letterV); //Letter V
  u8g2.setFont(u8g2_font_fub35_tn); //Set font for display
  u8g2.setCursor(2, 62);  //Setup for writing voltage
  
  //if (voltage > 7.5) {
    u8g2.setDrawColor(drawColor);
    u8g2.print(voltage,1);
  //} else {
    //u8g2.print(F("0.00"));
  //}
}





