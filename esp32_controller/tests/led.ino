#define LED_PIN 23

void setup() {
    pinMode(LED_PIN, OUTPUT);
    // Se establece el estado inicial de la tira LED a LOW (apagado)
    digitalWrite(ledControlPin, LOW);
    Serial.begin(9600);
    Serial.println("Iniciando prueba de tira LED ...");
}

void loop() {
    Serial.println("Encendiendo tira LED...");
    digitalWrite(ledControlPin, HIGH);
    delay(1000);

    Serial.println("Apagando tira LED...");
    digitalWrite(ledControlPin, LOW);
    delay(1000);
}
