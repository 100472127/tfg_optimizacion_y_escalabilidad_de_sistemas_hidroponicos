#define LED_PIN 23

void setup() {
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT);
    // Se establece el estado inicial de la tira LED a LOW (apagado)
    digitalWrite(LED_PIN, LOW);
    Serial.println("Iniciando prueba de tira LED ...");
}

void loop() {
    Serial.println("Encendiendo tira LED");
    digitalWrite(LED_PIN, HIGH);
    delay(3000);

    Serial.println("Apagando tira LED");
    digitalWrite(LED_PIN, LOW);
    delay(3000);
}
