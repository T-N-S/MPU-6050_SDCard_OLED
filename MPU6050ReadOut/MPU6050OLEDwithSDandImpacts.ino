/* ============================================

    Sketch for controlling a MPU6050
    Output on SD-Card (data logger)
    Author: T. Schumann
    Date: 2021-04-03
    V4.0 2021-06-13

    Dependencies:
    I2Cdev library collection - MPU6050 I2C device class -- https://github.com/jrowberg/i2cdevlib
    

  ===============================================
*/

const float offsetX = 0.15;       //input your offset here
const float offsetY = -0.3;       //input your offset here
const float offsetZ = -0.1;       //input your offset here



// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include <MPU6050.h>
#include <Wire.h>

#define ENLOG // Enable logging

#ifdef ENLOG
#include <SPI.h>
#include <SD.h>
#include <SSD1306AsciiWire.h>
#endif

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 mpu;
//MPU6050 accelgyro(0x69); // <-- use for AD0 high

SSD1306AsciiWire oled;

int16_t ax, ay, az;
float fx, fy, fz;

#define chipSelect 10      //CP Pin SD Module
#define I2C_ADDRESS 0x3C
#define BaudRate 115200
#define DECIMALS 4
#define LED_PIN 5             //Status LED
bool blinkState = false;
char filename[] = "LOGGER00.CSV";


//Variable for storing data on SD-Card
String dataString;

void setup() {
  // join I2C bus
  Wire.begin();

  // initialize serial communication
  Serial.begin(BaudRate);

  pinMode(LED_PIN, OUTPUT);
  pinMode(chipSelect, OUTPUT);

  Serial.print("Initializing SD card...");

  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(Adafruit5x7);
  oled.set2X();

#ifdef ENLOG
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    while (1) {   //output card failed
      oled.print("\n ");
      oled.println("SD-ERROR");
      oled.println("   !!!!");
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(1000);
      oled.clear();
    }
    return;
  }
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i / 10 + '0';
    filename[7] = i % 10 + '0';
    if (! SD.exists(filename)) {
      File logfile = SD.open(filename, FILE_WRITE);
      logfile.println("time in ms,x in m/s^2,y in m/s^2,z in m/s^2");
      logfile.close();
      break;  // leave the loop!
    }
  }

  Serial.print("Logging to: ");
  Serial.println(filename);

#endif

  // initialize device
  Serial.println("Initializing I2C devices...");
  mpu.initialize();
  // verify connection
  Serial.println("Testing device connections...");
  Serial.println(mpu.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");


  //---------Offsets for German MPU-6050----------
  /*Serial.println("Updating internal sensor offsets...");
    Serial.print(mpu.getXAccelOffset()); Serial.print("\t");
    Serial.print(mpu.getYAccelOffset()); Serial.print("\t");
    Serial.print(mpu.getZAccelOffset()); Serial.print("\t");
    Serial.print("\n");
    mpu.setXAccelOffset(4000);  //<-- place your values here
    mpu.setYAccelOffset(-16180);
    mpu.setZAccelOffset(230);
    Serial.print(mpu.getXAccelOffset()); Serial.print("\t");
    Serial.print(mpu.getYAccelOffset()); Serial.print("\t");
    Serial.print(mpu.getZAccelOffset()); Serial.print("\t");
    Serial.print("\n");*/



}

void loop() {
  // read raw accel measurements from device
  mpu.getAcceleration(&ax, &ay, &az);
  fx = (ay * 9.81 / 16384) + offsetX;
  fy = (az * 9.81 / 16384) + offsetY;
  fz = (ax * 9.81 / 16384) + offsetZ;
  printFloat("t: ", millis(), ",");  //print data to float
  printFloat("X: ", fx, ",");
  printFloat("Y: ", fy, ",");
  printFloat("Z: ", fz, ",");

#ifdef ENLOG
  SDwrite();    //write float to SD
  oled.clear(0, 0, 0, 0);
#endif

  // turn off sensor after impact or 60000ms
  if (ax > 30000 || ax < -30000 || millis() > 170000)
  {
    for (int i = 0; i < 50; i++) {  //safe next 50 values before shutting off
      mpu.getAcceleration(&ax, &ay, &az);
      fx = ay * 9.81 / 16384 + offsetX;
      fy = az * 9.81 / 16384 + offsetY;
      fz = ax * 9.81 / 16384 + offsetZ;
      printFloat("t: ", millis(), ",");  //print data to float
      printFloat("X: ", fx, ",");
      printFloat("Y: ", fy, ",");
      printFloat("Z: ", fz, ",");
#ifdef ENLOG
      SDwrite();
      oled.clear(0, 0, 0, 0);
#endif
      blinkState = !blinkState;
      digitalWrite(LED_PIN, blinkState);
    }
    while (1) {                     //drop successful, sensor still powered on
      digitalWrite(LED_PIN, HIGH);
      delay(4000);
    }
  }

  // blink LED to indicate activity
  blinkState = !blinkState;
  digitalWrite(LED_PIN, blinkState);
}


// write float to sd
#ifdef ENLOG
void SDwrite() {
  File logfile = SD.open(filename, FILE_WRITE);
  if (logfile) {
    logfile.println(dataString);
    logfile.close();
    Serial.println(dataString);
    dataString = "";
  }
  else {
    Serial.println(F("Error: Cannot write datalog.txt!"));
  }
}
#endif


static void printFloat(const char *str, float val, const char *sep) {
  oled.print(" ");
  oled.print(str);
  oled.println(val, DECIMALS / 2);
  dataString += String(val, DECIMALS);
  dataString += String(sep);
}
