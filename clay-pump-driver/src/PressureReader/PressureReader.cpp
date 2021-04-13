#include <Wire.h>
#include <Adafruit_INA219.h>
#include "PressureReader.h"

PressureSensor::PressureSensor () {
    Adafruit_INA219 ina219;
    this->sensor = ina219;
    this->connected = false;
    // this->sensor.begin();

    this->pressure_MA = -5.76;
    this->MA_current_itt = 0;
    this->last_read_time = micros();
}
bool PressureSensor::begin(){
    Serial.println("Sensor begin");
    this->connected = this->sensor.begin();
    if(this->connected) {
        Serial.println("Sensor connected");
    } else {
        Serial.println("Sensor not connected");
    }

    return this->connected;
}
float PressureSensor::read() {
    // Adafruit_INA219 ina219;
    if (!this->connected)
    {
        return pressure_MA;
    }

    float voltage_V = 0;
    float shuntVoltage_mV;
    float busVoltage_V;
    float current_mA = 0;
    float power_mW = 0;
    float energy_Wh = 0;
    long time_s = 0;

    // time_s = millis() / (1000); // convert time to sec
    // busVoltage_V = this->sensor.getBusVoltage_V();
    // shuntVoltage_mV = this->sensor.getShuntVoltage_mV();
    // voltage_V = busVoltage_V + (shuntVoltage_mV / 1000);
    current_mA = this->sensor.getCurrent_mA();
    //power_mW = this->sensor.getPower_mW();
    // power_mW = current_mA * voltage_V;
    // energy_Wh = (power_mW * time_s) / 3600; //energy in watt hour

    return current_mA;
}

float PressureSensor::readMA(void) {
    this->last_read_time = micros();

    float current_mA = this->read();

    if (this->MA_current_itt < 1000) {
        this->MA_current_itt++;
    } 

    // calculate moving average
    this->pressure_MA = ((this->pressure_MA * (this->MA_current_itt - 1) + current_mA) / this->MA_current_itt);
    // this->pressure_MA = MA_current;

    return this->pressure_MA;
}
