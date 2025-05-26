#define PH_PIN 35

void setup() {
    Serial.begin(9600);
    Serial.println("Iniciando prueba del sensor de PH ...");
}

void loop() {
    // Mide un valor entre 0 y 4095
    int rawValue = analogRead(PH_PIN);

    // Convertir el valor leído a voltaje
    float voltage = rawValue * (3.3 / 4095.0);

    // Conversión aproximada de voltaje a pH
    float phValue = 7 + ((2.5 - voltage) / 0.18); 

    Serial.print("Lectura ADC: ");
    Serial.print(rawValue);
    Serial.print(" | Voltaje: ");
    Serial.print(voltage, 3);
    Serial.print(" V | PH: ");
    Serial.println(phValue, 2);

    delay(2000);
}
