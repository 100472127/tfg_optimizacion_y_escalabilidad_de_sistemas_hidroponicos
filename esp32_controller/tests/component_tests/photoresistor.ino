#define PHOTORESISTOR_PIN 36

void setup() {
    Serial.begin(9600);
    Serial.println("Iniciando prueba de la fotorresistencia ...");
}

void loop() {
    // Mide un valor entre 0 y 4095
    int rawValue = analogRead(PHOTORESISTOR_PIN);

    // Conversión del valor leído a porcentaje de luz (0 a 100%)
    float lightResistorValue = rawValue / 4095.0 * 100;

    Serial.print("Lectura ADC: ");
    Serial.print(rawValue);
    Serial.print(" | PH: ");
    Serial.println(lightResistorValue, 2);

    delay(2000);
}