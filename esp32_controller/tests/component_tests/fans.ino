#define FAN_PIN 19

// Se establece la configuración del PWM
const int pwmChannel = 0;
const int pwmFreq = 1000;
const int pwmResolution = 8;
const int maxDuty = 255;
const int minDuty = 0;

void setup(){
    Serial.begin(9600);
    pinMode(FAN_PIN, OUTPUT);
    // Configurar PWM
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(FAN_PIN, pwmChannel);
    // Se establece el estado inicial del ventilador a apagado
    ledcWrite(pwmChannel, minDuty);
    Serial.println("Iniciando prueba del ventilador ...");
    delay(2000);
}

void loop(){
    // Encender al máximo por 3 segundos

    Serial.println("Encendiendo ventilador al máximo");
    ledcWrite(pwmChannel, maxDuty);
    delay(3000);

    // Apagar completamente por 3 segundos

    Serial.println("Apagando ventilador");
    ledcWrite(pwmChannel, minDuty);
    delay(3000);

    // Aumentar intensidad gradualmente en 5 segundos
    Serial.println("Aumentando intensidad del ventilador gradualmente");
    for (int duty = minDuty; duty <= maxDuty; duty++){
        ledcWrite(pwmChannel, duty);
        delay(5000 / maxDuty);
    }

    // Disminuir intensidad gradualmente en 5 segundos
    Serial.println("Disminuyendo intensidad del ventilador gradualmente");
    for (int duty = maxDuty; duty >= minDuty; duty--){
        ledcWrite(pwmChannel, duty);
        delay(5000 / maxDuty);
    }

    // Esperar 10 segundos antes de repetir el ciclo
    delay(10000);
}
