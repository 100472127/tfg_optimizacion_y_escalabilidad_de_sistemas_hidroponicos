#define FAN_PIN 19

void setup() {
    Serial.begin(9600);
    pinMode(FAN_PIN, OUTPUT);
    // Se establece el estado inicial del ventilador a LOW (apagado)
    digitalWrite(FAN_PIN, LOW);
    Serial.println("Iniciando prueba del ventilador ...");
}

void loop() {
    Serial.println("Encendiendo ventilador");
    digitalWrite(FAN_PIN, HIGH);
    delay(3000);

    Serial.println("Apagando ventilador");
    digitalWrite(FAN_PIN, LOW);
    delay(3000);
}
