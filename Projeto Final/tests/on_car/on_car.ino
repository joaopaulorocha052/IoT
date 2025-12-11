/*==========================================================================
// Author : Handson Technology
// Project : Arduino Uno
// Description : L298N Motor Driver
// Source-Code : L298N_Motor.ino
// Program: Control 2 DC motors using L298N H Bridge Driver
//==========================================================================
*/
// Definitions Arduino pins connected to input H Bridge
int IN1 = 4;
int IN2 = 5;
int IN3 = 6;
int IN4 = 7;

int ENA = 10;
int ENB = 11;

void setup()
{
  // Set all the motor control pins to outputs
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

}

void loop()
{
digitalWrite(ENA, HIGH);
digitalWrite(ENB, HIGH);
// Rotate the Motor A clockwise
digitalWrite(IN1, HIGH);
digitalWrite(IN2, LOW);
delay(2000);
// Motor A
digitalWrite(IN1, HIGH);
digitalWrite(IN2, HIGH);
delay(500);
// Rotate the Motor B clockwise
digitalWrite(IN3, HIGH);
digitalWrite(IN4, LOW);
delay(2000);
// Motor B
digitalWrite(IN3, HIGH);
digitalWrite(IN4, HIGH);
delay(500);
// Rotates the Motor A counter-clockwise
digitalWrite(IN1, LOW);
digitalWrite(IN2, HIGH);
delay(2000);
// Motor A
digitalWrite(IN1, HIGH);
digitalWrite(IN2, HIGH);
delay(500);
// Rotates the Motor B counter-clockwise
digitalWrite(IN3, LOW);
digitalWrite(IN4, HIGH);
delay(2000);
// Motor B
digitalWrite(IN3, HIGH);
digitalWrite(IN4, HIGH);
delay(500);
}