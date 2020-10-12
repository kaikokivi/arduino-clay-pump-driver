
/*
 Stepper Driver Control

 This program drives a stepper through a driver.
 The driver is attached to digital pins 8 and 9 and 3.3V of the Arduino.
 
 Created 28 May. 2020
 Modified 29 May. 2020
 by Kaiko Kivi

 */
#include <Arduino.h>
#include <StepperDriver.h>
#include <ArduinoBLE.h>


const int fullStepRev = 200;
const int stepFrag = 5;
const int stepsPerRevolution = fullStepRev * stepFrag; // change this to fit the number of steps per revolution
int speed = 0;
unsigned long start = 0L;
// for your motor

// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, 8, 9);
BLEService StepperService("19b10000-e8f2-537e-4f6c-d104768a1214"); // BLE LED Service
BLEIntCharacteristic speedCharacteristic("19b10001-e8f2-537e-4f6c-d104768a1214", BLERead | BLEWrite | BLENotify);

void setup()
{
  // set the speed at 60 rpm:
  // initialize the serial port:
  Serial.begin(9600);
  // while (!Serial);

  pinMode(LED_BUILTIN, OUTPUT);

  if (!BLE.begin())
  {
    Serial.println("starting BLE failed!");

    while (1)
      ;
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("Pump_driver");
  BLE.setDeviceName("Pump_driver");
  BLE.setAdvertisedService(StepperService);

  // add the characteristic to the service
  StepperService.addCharacteristic(speedCharacteristic);

  // add service
  BLE.addService(StepperService);

  // set the initial value for the characeristic:
  speedCharacteristic.writeValue(speed);

  // start advertising
  BLE.advertise();

  Serial.println("Stepper Control BLE Peripheral");
}

bool step = false;
void loop()
{
  // listen for BLE peripherals to connect:
  start = micros();
  BLEDevice central = BLE.central();
  // Serial.println("waiting for central");

  // if a central is connected to peripheral:
  if (central.connected())
  {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());
    
    while(central.connected()){
      digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
      step = myStepper.step() != 0;

      // check for characteristic write
      if (speedCharacteristic.written())
      {
        Serial.print("speed: ");
        Serial.println(speedCharacteristic.value(), 10);
        speed = speedCharacteristic.value();
        step = false;
        // setMove(int <no of steps> | 0 <infinite> [, int <speed rpm> [, bool <move immediately>]) 
        myStepper.setMove(0, speed);
      }

      // do step if required
      if (step) {
        // .step() returns steps left so if not 0 will keep stepping
        step = myStepper.step() != 0;
      }
      // Serial.print("move time: ");
      // Serial.println(micros() - start);

      // while(myStepper.step())
      digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
    }

    if (!central.connected())
    {
      // when the central disconnects, print it out:
      Serial.print(F("Disconnected from central: "));
      Serial.println(central.address());
      while(step && !central.connected()){
        step = myStepper.step() != 0;
      }
    }
  }
  // Serial.print("Loop time: ");
  // Serial.println(micros() - start);

  //Serial.println("setSpeed");
  // myStepper.setSpeed(120);
  // step one revolution  in one direction:
}
