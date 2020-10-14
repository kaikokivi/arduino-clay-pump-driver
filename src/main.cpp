
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

const int fullStepRev = 200;
const int stepFrag = 5;
const int stepsPerRevolution = fullStepRev * stepFrag; // change this to fit the number of steps per revolution
int speed = 0;
unsigned long start = 0L;
// for your motor

// initialize the stepper library on pins 8, 9:
Stepper myStepper(stepsPerRevolution, 8, 9);

// initialise pressure sensor
PressureSensor pressureSensor = PressureSensor();

BLEService StepperService("2f62cdb2-d105-466e-a818-81ea9adfe9ae"); // BLE LED Service
BLEIntCharacteristic speedCharacteristic("f2dd248a-4c5f-48c4-bee7-cc237fd666b4", BLERead | BLEWrite | BLENotify);
BLEFloatCharacteristic pressureCharacteristic("2dbee6b6-3bb7-456a-bd83-d830ed191eea", BLERead | BLEWrite | BLENotify);

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
  BLE.setLocalName("Pump_driver_2");
  BLE.setDeviceName("Pump_driver_2");
  BLE.setAdvertisedService(StepperService);

  // add the characteristic to the service
  StepperService.addCharacteristic(speedCharacteristic);
  StepperService.addCharacteristic(pressureCharacteristic);

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
    if (!pressureSensor.begin())
    {
      Serial.println("Failed to find INA219 chip");
    }
    int pressure_plot_cycle = 0;

    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());
    
    while(central.connected()){
      digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
      step = myStepper.step();

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
        }

        // myStepper.setMove(0, myStepper.speed*(1 - (pressure-2.0) / 2.0));
        // Serial.print("Speed: ");
        // Serial.println(myStepper.speed);
      }

      // do step if required
      myStepper.step();
      

      // while(myStepper.step())
      digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
    }

    if (!central.connected())
    {
      // when the central disconnects, print it out:
      Serial.print(F("Disconnected from central: "));
      Serial.println(central.address());
      while(step && !central.connected()){
        myStepper.step();
      }
    }
  }
  // Serial.print("Loop time: ");
  // Serial.println(micros() - start);

  //Serial.println("setSpeed");
  // myStepper.setSpeed(120);
  // step one revolution  in one direction:
}
