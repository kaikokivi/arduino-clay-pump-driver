
/*
 Stepper Driver Control

 This program drives a stepper through a driver.
 The driver is attached to digital pins 8 and 9 and 3.3V of the Arduino.
 
 Created 28 May. 2020
 Modified 29 May. 2020
 by Kaiko Kivi

 */
#include <Arduino.h>
#include <ArduinoBLE.h>
#include <StepperDriver.h>
#include <PressureReader.h>

// BLE attributes for Pump controller
const char *BLE_LOCAL_NAME = "Pump";
const char *BLE_DEVICE_NAME = "Pump";
const char *BLE_STEPPER_SERVICE_ID = "2f62cdb2-d105-466e-a818-81ea9adfe9ae";
const char *BLE_SPEED_CHAR_ID = "f2dd248a-4c5f-48c4-bee7-cc237fd666b4";
const char *BLE_PRESSURE_TARGET_CHAR_ID = "b9ec56a5-b66c-4ec6-89e2-7d539d606171";
const char *BLE_PRESSURE_SENSOR_CHAR_ID = "2dbee6b6-3bb7-456a-bd83-d830ed191eea";

// BLE attributes for EXTURDER controller
// const char *BLE_LOCAL_NAME = "Extruder";
// const char *BLE_DEVICE_NAME = "Extruder";
// const char *BLE_STEPPER_SERVICE_ID = "19b10000-e8f2-537e-4f6c-d104768a1214";
// const char *BLE_SPEED_CHAR_ID = "19b10001-e8f2-537e-4f6c-d104768a1214";
// const char *BLE_PRESSURE_TARGET_CHAR_ID = "b9ec56a5-b66c-4ec6-89e2-7d539d606171";
// const char *BLE_PRESSURE_SENSOR_CHAR_ID = "991a50fa-b15c-434c-877d-9026a65363cb";

const int fullStepRev = 200;
const int stepFrag = 5;
const int stepsPerRevolution = fullStepRev * stepFrag; // change this to fit the number of steps per revolution

int speed = 0; // initial speed is 0
float pressureTarget = 5.0; // initial pressure target is 5.0
int pressure_plot_cycle = 0;

// initialize the stepper library on pins 8, 9:
Stepper pumpStepper(stepsPerRevolution, 8, 9);

// initialise pressure sensor
PressureSensor pressureSensor;

BLEService StepperService(BLE_STEPPER_SERVICE_ID); // BLE LED Service
BLEIntCharacteristic speedCharacteristic(BLE_SPEED_CHAR_ID, BLERead | BLEWrite | BLENotify);
BLEFloatCharacteristic pressureTargetCharacteristic(BLE_PRESSURE_TARGET_CHAR_ID, BLERead | BLEWrite | BLENotify);
BLEFloatCharacteristic pressureCharacteristic(BLE_PRESSURE_SENSOR_CHAR_ID, BLERead | BLEWrite | BLENotify);

void setup()
{
  // set the speed at 60 rpm:
  // initialize the serial port:
  Serial.begin(9600);
  // while (!Serial);

  pinMode(LED_BUILTIN, OUTPUT);

  if(!pressureSensor.begin()) {

    Serial.println("Starting sensor falied");
    pumpStepper.setMove(0,0);
  } else {
    float pressure = pressureSensor.readMA();

    Serial.print("Pressure ");
    Serial.println(pressure);
  }

  if (!BLE.begin())
  {
    Serial.println("Starting BLE failed!");

    while (1);
  }

  // set advertised local name and service UUID:
  BLE.setLocalName(BLE_LOCAL_NAME);
  BLE.setDeviceName(BLE_DEVICE_NAME);
  BLE.setAdvertisedService(StepperService);

  // add the characteristic to the service
  StepperService.addCharacteristic(speedCharacteristic);
  StepperService.addCharacteristic(pressureTargetCharacteristic);
  StepperService.addCharacteristic(pressureCharacteristic);

  // add service
  BLE.addService(StepperService);

  // set the initial value for the characeristic:
  speedCharacteristic.writeValue(speed);

  // start advertising
  BLE.advertise();

  Serial.println("Stepper Control BLE Peripheral");
}

bool BLEStatus = false;

void loop()
{
  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();
  // Serial.println("waiting for central");

  // if a central is connected to peripheral:
  if (central.connected())
  {
    if(BLEStatus == false) {
      digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
      Serial.print("Connected to central: ");
      // print the central's MAC address:
      Serial.println(central.address());
      BLEStatus = true;
    }

      // check for characteristic write
      if (speedCharacteristic.written())
      {
        Serial.print("Speed: ");
        Serial.println(speedCharacteristic.value(), 10);
        speed = speedCharacteristic.value();
        pressureTarget = 0;
        // setMove(int <no of steps> | 0 <infinite> [, int <speed rpm> [, bool <move immediately>])
        pumpStepper.setMove(0, speed);
      }

      if (pressureTargetCharacteristic.written())
      {
        Serial.print("Pressure Target: ");
        Serial.println(pressureTargetCharacteristic.value());
        Serial.print("Using speed: ");
        Serial.println(speed);

        pressureTarget = pressureTargetCharacteristic.value();
        // setMove(int <no of steps> | 0 <infinite> [, int <speed rpm> [, bool <move immediately>])
        if(pressureSensor.connected) {
          float pressure = pressureSensor.readMA();
          Serial.print("Pressure ");
          Serial.println(pressure);
          pressureCharacteristic.writeValue(pressure);

          pumpStepper.setMove(0, speed);
          speedCharacteristic.writeValue(speed);
        }
        else
        {
          if (pressureSensor.begin())
          {
            pumpStepper.setMove(0, speed);
            speedCharacteristic.writeValue(speed);
          }
          else
          {
            pumpStepper.setMove(0, 0);
            speed = 0;
            speedCharacteristic.writeValue(0);
            pressureTargetCharacteristic.writeValue(0);
            Serial.println("Pressure sensor not connected!");
          }
        }
      }
      // while(pumpStepper.step())
    
  }

  if (pressureSensor.connected)
  {
    if ((pressureSensor.last_read_time + 10.0) < micros())
    {
      float pressure = pressureSensor.readMA();
      if (pressure_plot_cycle < 100)
      {
        pressure_plot_cycle++;
      }
      else
      {
        Serial.print("Pressure ");
        Serial.println(pressure);
        pressureCharacteristic.writeValue(pressure);

        if (pressure > pressureTarget)
        {
          pumpStepper.setMove(0, 0);
          speedCharacteristic.writeValue(0);
        }
        if(pressure < pressureTarget) {
          if (pressure < (pressureTarget * 0.98) && pressureTarget != 0)
          {
            pumpStepper.setMove(0, speed);
            speedCharacteristic.writeValue(speed);
          } else {
            float speedFrac = (pressureTarget - pressure) / pressureTarget / 0.02;
            Serial.print("SpeedFrac");
            Serial.println(speedFrac);
            Serial.println(speed * speedFrac);
            pumpStepper.setMove(0, speed);
            speedCharacteristic.writeValue(speed * speedFrac);
          }
        }
        pressure_plot_cycle = 0;
      }

      // hard stop when pressure hits a limit or is lost somehow
      if (pressure > 12 || pressure < 3) {
        pumpStepper.setMove(0, 0);
        pressureCharacteristic.writeValue(pressure);
        speedCharacteristic.writeValue(0);
      }
    }
  } else {
    if (pressureTarget != 0) {
      pressureTarget = 0;
      pumpStepper.setMove(0, 0);
    }
  }

  // do step if required
  pumpStepper.step();

  if (!central.connected() && BLEStatus == true)
  {
    // write the speed setting to BLE so that on reconnect the central can read it
    speedCharacteristic.writeValue(speed);
    digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
    BLEStatus = false;
  }
}
