#define TDS_PIN 34

void setup() {
    Serial.begin(9600);
    Serial.println("Iniciando prueba de la sensor TDS ...");
}

void loop() {
    // Mide un valor entre 0 y 4095
    int rawValue = analogRead(TDS_PIN);

    // Convertir el valor leído a voltaje
    float voltage = rawValue * (3.3 / 4095.0);

    // Calcular el valor TDS en ppm (partes por millón)
    float tdsValue = (133.42 * voltage * voltage * voltage
                    - 255.86 * voltage * voltage
                    + 857.39 * voltage) * 0.5; 

    Serial.print("Lectura ADC: ");
    Serial.print(rawValue);
    Serial.print(" | Voltaje: ");
    Serial.print(voltage, 3);
    Serial.print(" V");
    Serial.print(" | PPM: ");
    Serial.println(tdsValue);

    delay(2000);
}