#include "Arduino.h"
#include "StepperDriver.h"

Stepper::Stepper(int stepsPerRevolution, int step_pin, int dir_pin)
{
    this->last_step_time = 0;
    this->number_of_steps = stepsPerRevolution;
    this->step_pin = step_pin;
    this->dir_pin = dir_pin;
    this->step_delay = 0;
    this->speed = 0;
    pinMode(this->step_pin, OUTPUT);
    pinMode(this->dir_pin, OUTPUT);
}

void Stepper::setSpeed(long whatSpeed)
{
    this->speed = whatSpeed;
    if(this->speed == 0) {
        this->move_continue = false;
    }
    //Serial.println("setSpeed");
    this->step_delay = 60L * 1000L * 1000L / this->number_of_steps / abs(whatSpeed);
    // Serial.println(this->step_delay);
    if (whatSpeed > 0)
    {
        dirMotor(1);
    }
    if (whatSpeed < 0)
    {
        dirMotor(0);
    }
}
void Stepper::setMove(int steps_to_move)
{
    this->steps_left = abs(steps_to_move); // how many steps to take
    this->last_step_time = micros();       // reset last_step_time to now
}
void Stepper::setMove(int steps_to_move, long whatSpeed)
{
    if(steps_to_move == 0 && whatSpeed != 0) {
        this->move_continue = true;
    } else {
        this->move_continue = false;
    }
    if(abs(whatSpeed) > 0) {
        this->setSpeed(whatSpeed);
        this->setMove(steps_to_move);
    } else {
        this->steps_left = 0;
    }
}
void Stepper::setMove(int steps_to_move, long whatSpeed, bool finish)
{
    this->setMove(steps_to_move, whatSpeed);
    if (finish)
        while (this->stepDelay())
        {
            delay(1);
        };
}
int Stepper::step()
{
    if (this->move_continue && this->step_delay != 0 && (this->step_delay + this->last_step_time) < micros())
    {
        // Serial.println("step");
        stepMotor();
        this->last_step_time = this->last_step_time + this->step_delay;
        // above would fail to catch up if the motor is behind a more than a step
    }
    return this->last_step_time;
}
int Stepper::stepDelay()
{
    if (this->steps_left < 1 && !this->move_continue)
        return 0;
    unsigned long now = micros();
    while (now - this->last_step_time < this->step_delay)
    {
        now = micros();
    }
    // increment or decrement the step number,
    // depending on direction:
    if (this->dir == 1)
    {
        this->step_number++;
        if (this->step_number == this->number_of_steps)
        {
            this->step_number = 0;
        }
    }
    else
    {
        if (this->step_number == 0)
        {
            this->step_number = this->number_of_steps;
        }
        this->step_number--;
    }
    // decrement the steps left:
    this->steps_left--;
    // step the motor to step number 0, 1, ..., {3 or 10}
    stepMotor();
    this->last_step_time = this->last_step_time + this->step_delay;
    return this->steps_left;
}

void Stepper::dirMotor(int dir)
{
    if (dir == 1 && digitalRead(this->dir_pin) != HIGH)
        digitalWrite(this->dir_pin, HIGH);
    if (dir == 0 && digitalRead(this->dir_pin) != LOW)
        digitalWrite(this->dir_pin, LOW);
}
void Stepper::stepMotor()
{
    digitalWrite(this->step_pin, HIGH);
    delayMicroseconds(50);
    digitalWrite(this->step_pin, LOW);
    delayMicroseconds(50);
}
