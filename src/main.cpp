
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

// initialize the stepper library on pins 8, 9:
Stepper myStepper(stepsPerRevolution, 8, 9);

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
  while (!Serial);

  pinMode(LED_BUILTIN, OUTPUT);

  if(!pressureSensor.begin()) {
    Serial.println("Starting sensor falied");
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
      Serial.print("Connected to central: ");
      // print the central's MAC address:
      Serial.println(central.address());
      BLEStatus = true;
    }

    // while(central.connected()){
      digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)

      // check for characteristic write
      if (speedCharacteristic.written())
      {
        Serial.print("Speed: ");
        Serial.println(speedCharacteristic.value(), 10);
        speed = speedCharacteristic.value();
        // setMove(int <no of steps> | 0 <infinite> [, int <speed rpm> [, bool <move immediately>])
        myStepper.setMove(0, speed);
      }

      if (pressureTargetCharacteristic.written())
      {
        Serial.print("Pressure Tartget: ");
        Serial.println(pressureTargetCharacteristic.value());
        pressureTarget = pressureTargetCharacteristic.value();
        // setMove(int <no of steps> | 0 <infinite> [, int <speed rpm> [, bool <move immediately>])
        myStepper.setMove(0, speed);
      }
      // while(myStepper.step())
      digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
    // }
    
  }


  if (pressureSensor.connected)
  {
    int pressure_plot_cycle = 0;
    if ((1000.0 + pressureSensor.last_read_time) < micros())
    {
      float pressure = pressureSensor.readMA();
      if (pressure_plot_cycle < 100)
      {
        pressure_plot_cycle++;
      }
      else
      {
        pressureCharacteristic.writeValue(pressure);
        Serial.print("Pressure: ");
        Serial.println(pressure);
        pressure_plot_cycle = 0;
        if (pressure > pressureTarget)
        {
          myStepper.setMove(0, 0);
        }
        if (pressure < (pressureTarget * 0.95))
        {
          myStepper.setMove(0, speed);
        }
      }

      // hard stop when pressure hits a limit or is lost somehow
      if (pressure > 12 || pressure < 3) {
        myStepper.setMove(0, 0);
      }
    }

    // do step if required
    myStepper.step();
  } else {
    // Serial.println("step");
    // do step if required without a sensor
    myStepper.step();
  }

  if (!central.connected() && BLEStatus == true)
  {
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
    BLEStatus = false;
  }
}
