#include <Adafruit_INA219.h>
/*
 Pressure Reader Lib

 
 Created 12 Oct. 2020
 Modified 12 Oct. 2020
 by Kaiko Kivi

 */

class PressureSensor
{
public:
    PressureSensor();

    int last_read_time;
    float read(void);
    float readMA(void);
    bool begin(void);
    bool connected;

private:
    Adafruit_INA219 sensor;
    float pressure_MA;
    float MA_current_itt;
};
