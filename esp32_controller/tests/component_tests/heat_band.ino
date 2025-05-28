#define HEATER_PIN 18

void setup() {
    Serial.begin(9600);
    pinMode(HEATER_PIN, OUTPUT);
    // Se establece el estado inicial del ventilador a LOW (apagado)
    digitalWrite(FAN_PIN, LOW);
    Serial.println("Iniciando prueba de la banda calefactable ...");
}

void loop() {
    Serial.println("Encendiendo banda calefactable");
    digitalWrite(HEATER_PIN, HIGH);
    delay(5000);

    Serial.println("Apagando banda calefactable");
    digitalWrite(HEATER_PIN, LOW);
    delay(5000);
}
