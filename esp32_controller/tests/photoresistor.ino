#define PHOTORESISTOR_PIN 36

void setup() {
    Serial.begin(9600);
    Serial.println("Iniciando prueba de la fotorresistencia ...");
}

void loop() {
    // Mide un valor entre 0 y 4095
    int rawValue = analogRead(PHOTORESISTOR_PIN) / 4095.0 * 100;

    Serial.print("Valor leído de la fotorresistencia: ");
    Serial.println(rawValue);
    delay(2000);
}