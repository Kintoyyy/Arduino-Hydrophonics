// Conditions
// For temp and humidity sensor
// The pump will on in the ff. time of the day.
// 5 AM and 5 PM
// the recommended temperature range is 25 - 27°C with relative humidity of 85-100%.
// If higher than the recommended temp. the pump will on
// If lower than the  recommended humidity the pump will on
// For Ph Level
// If ph level value low or higher than 6-7 ph value the pump will on.
// Water level sensor condition
// If after detect sa ph level then on na ang pump mo relay og signal ang water level sensor sa 3rd pump para mo refill og tubig then if puno na mo relay ang other water level sensor aron mo stop na refill ang pump.
// Note: If naay dli makaya then naa kay suggestion ayaw lang ka uwaw approach aron ma adjust kay basin impossible ra kaayo ang uban conditions. So mao rato and good evessss.
#include "Arduino.h"
#include "uRTCLib.h"
#include <dht.h>

// ------------- Configurations ------------------------

// humidity percentage to trigger water pump 1
int humidityValueTrigger = 85;

// temperature to trigger water pump 1
int temperatureValueTrigger = 26;

// water pump 1 duration
int sprinklerDuration = 30000;  // 30000ms = 30seconds

// max Ph level to trigger water change
float maxPhLevel = 7;  // example 7.2, 8.13, 5

// min Ph level to trigger water change
float minPhLevel = 6;  // example 3.2, 4.13, 5

// sprinkler on time 24 hour format
int time1 = 5;
int time2 = 17;

// -------------  Debugging purposes  -----------------

// set to false to see the sensor data in Serial Monitor (9600 baud)
bool turnOnPhsensor = true;

// adjust this value (0 - 10)
float phcalibration = 0.7;


//---------------------------------------------------------------------------------------------------
//   __  __           _        _             _    _       _                             __
//  |  \/  |         | |      | |           | |  (_)     | |                           _\ \  
//  | \  / | __ _  __| | ___  | |__  _   _  | | ___ _ __ | |_ ___  _   _ _   _ _   _  (_)\ \ 
//  | |\/| |/ _` |/ _` |/ _ \ | '_ \| | | | | |/ / | '_ \| __/ _ \| | | | | | | | | |     > >
//  | |  | | (_| | (_| |  __/ | |_) | |_| | |   <| | | | | || (_) | |_| | |_| | |_| |  _ / /
//  |_|  |_|\__,_|\__,_|\___| |_.__/ \__, | |_|\_\_|_| |_|\__\___/ \__, |\__, |\__, | (_)_/
//                                    __/ |                         __/ | __/ | __/ |
//                                   |___/                         |___/ |___/ |___/
//---------------------------------------------------------------------------------------------------


// Humidity and temperature sensor
#define DHT22_PIN 7

// Water pump relays
#define SprinklerPump 2  // Sprinkler
#define FillPump 3       // Water tank Fill pump
#define EmptyPump 4      // Water tank Empty pump

// Water Full sensor
#define FullSensor 6
#define FullSensorPin A2
// Water Empty sensor
#define EmtpySensor 5
#define EmtpySensorPin A1


// PS: Dont touch anything below this :)
uRTCLib rtc(0x68);
dht DHT;

//Variables
int val = 0;
float calibration_value = 21.34 - phcalibration;
int phval = 0;
unsigned long int avgval;
int buffer_arr[10], temp;


void setup() {
  Serial.begin(9600);
  delay(1000);
  URTCLIB_WIRE.begin();


  Serial.println(".......");
  //Set water pump as output
  pinMode(SprinklerPump, OUTPUT);
  pinMode(FillPump, OUTPUT);
  pinMode(EmptyPump, OUTPUT);

  //Set water pump to off
  digitalWrite(SprinklerPump, HIGH);
  digitalWrite(FillPump, HIGH);
  digitalWrite(EmptyPump, HIGH);

  pinMode(FullSensor, OUTPUT);
  digitalWrite(FullSensor, LOW);
  pinMode(EmtpySensor, OUTPUT);
  digitalWrite(EmtpySensor, LOW);
}

void loop() {
  //check time
  rtc.refresh();

  //set humidity and temp sensor
  int chk = DHT.read22(DHT22_PIN);

  delay(1000);

  // turn on water1 on 5AM and 5PM
  if (rtc.hour() == time1 || rtc.hour() == time2) {

    if (rtc.minute() == 0) {
      Serial.print("Water pump on");
      digitalWrite(SprinklerPump, LOW);
      delay(sprinklerDuration);
      digitalWrite(SprinklerPump, HIGH);
    }
  }

  // check humidity and temperature
  if (DHT.humidity < humidityValueTrigger || DHT.temperature > temperatureValueTrigger) {
    digitalWrite(SprinklerPump, LOW);
  } else {
    digitalWrite(SprinklerPump, HIGH);
  }

  for (int i = 0; i < 10; i++) {
    buffer_arr[i] = analogRead(A0);
    delay(30);
  }
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buffer_arr[i] > buffer_arr[j]) {
        temp = buffer_arr[i];
        buffer_arr[i] = buffer_arr[j];
        buffer_arr[j] = temp;
      }
    }
  }

  avgval = 0;
  for (int i = 2; i < 8; i++)
    avgval += buffer_arr[i];
  float volt = (float)avgval * 5.0 / 1024 / 6;
  float ph_act = -5.70 * volt + calibration_value;

  if (turnOnPhsensor) {
    //Check Ph Values
    if (ph_act > maxPhLevel || ph_act < minPhLevel) {
      while (!emptyWater()) {
      }
      delay(2000);
      while (!fillWater()) {
      }
      delay(2000);
    }
  }


  //Debugging
  Serial.print("Time: ");
  Serial.print(rtc.hour());
  Serial.print(':');
  Serial.print(rtc.minute());
  Serial.print(':');
  Serial.print(rtc.second());
  Serial.print(", Humidity: ");
  Serial.print(DHT.humidity);
  Serial.print("%, Temp: ");
  Serial.print(DHT.temperature);
  Serial.print("c");
  Serial.print(", pH Value: ");
  Serial.print(ph_act);
  Serial.print(", Water Empty Sensor: ");
  Serial.print(readWaterSensor(EmtpySensor, EmtpySensorPin));
  Serial.print(", Water Fill Sensor: ");
  Serial.println(readWaterSensor(FullSensor, FullSensorPin));
}

//Empty water tank function
bool emptyWater() {
  delay(30);
  if (readWaterSensor(EmtpySensor, EmtpySensorPin) > 30) {
    digitalWrite(EmptyPump, LOW);
    Serial.print("Removing Water : ");
    Serial.println(readWaterSensor(EmtpySensor, EmtpySensorPin));
    return false;
  }
  delay(500);
  digitalWrite(EmptyPump, HIGH);
  Serial.println("Done Drain");
  return true;
}

//Fill water tank function
bool fillWater() {
  delay(30);
  if (readWaterSensor(FullSensor, FullSensorPin) < 600) {
    digitalWrite(FillPump, LOW);
    Serial.print("Replacing Water : ");
    Serial.println(readWaterSensor(FullSensor, FullSensorPin));
    return false;
  }
  delay(500);
  digitalWrite(FillPump, HIGH);
  Serial.println("Done Refill");
  return true;
}


// read waterSensor function
int readWaterSensor(int WaterPower, int WaterPin) {
  digitalWrite(WaterPower, HIGH);
  delay(10);
  val = analogRead(WaterPin);
  digitalWrite(WaterPower, LOW);
  return val;
}
