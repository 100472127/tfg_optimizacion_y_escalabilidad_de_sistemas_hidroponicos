#include <ESP32Servo.h>

#define SERVO_PIN 14

Servo miServo;

void setup() {
    Serial.begin(9600);
    Serial.println("Iniciando prueba del servomotor ...");
    miServo.attach(SERVO_PIN);
}

void loop() {
    Serial.println("Giro a 0°");
    miServo.write(0);
    delay(3000);

    Serial.println("Giro a 90°");
    miServo.write(90);
    delay(3000);

    Serial.println("Giro a 180°");
    miServo.write(180);
    delay(3000);
}