#define LED_PIN 23

// Se establece la configuración del PWM
const int pwmChannel = 0;
const int pwmFreq = 1000;
const int pwmResolution = 8;
const int maxDuty = 255;
const int minDuty = 0;

void setup() {
    Serial.begin(9600);
    // Configurar PWM
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(LED_PIN, pwmChannel);
    Serial.println("Iniciando prueba del la tira LED ...");
    delay(2000);
}

void loop() {
    // Encender al máximo por 3 segundos
    ledcWrite(pwmChannel, maxDuty);
    delay(3000);

    // Apagar completamente por 3 segundos
    ledcWrite(pwmChannel, minDuty);
    delay(3000);

    // Aumentar intensidad gradualmente en 5 segundos
    for (int duty = minDuty; duty <= maxDuty; duty++) {
        ledcWrite(pwmChannel, duty);
        delay(5000 / maxDuty);
    }

    // Disminuir intensidad gradualmente en 5 segundos
    for (int duty = maxDuty; duty >= minDuty; duty--) {
        ledcWrite(pwmChannel, duty);
        delay(5000 / maxDuty);
    }

    // Esperar 10 segundos antes de repetir el ciclo
    delay(10000);
}
