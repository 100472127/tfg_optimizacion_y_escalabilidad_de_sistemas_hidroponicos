#define NUM_BOMBAS 6

const int bombas[NUM_BOMBAS] = {15, 2, 4, 16, 17, 5};

void setup() {
    Serial.begin(9600);
    Serial.println("Iniciando prueba de las 6 bombas de agua ...");

    // Configurar cada pin como salida
    for (int i = 0; i < NUM_BOMBAS; i++) {
        pinMode(bombas[i], OUTPUT);
        // Se establece el estado inicial de las bombas a LOW (apagadas)
        digitalWrite(bombas[i], LOW);  
    }
}

void loop() {
    // Encendemos y apagamos cada bomba en secuencia
    for (int i = 0; i < NUM_BOMBAS; i++) {
        Serial.print("Encendiendo bomba ");
        Serial.println(i + 1);
        digitalWrite(bombas[i], HIGH);
        delay(3000);

        Serial.print("Apagando bomba ");
        Serial.println(i + 1);
        digitalWrite(bombas[i], LOW);
        delay(3000);
    }

    Serial.println("Ciclo completo. Repitiendo prueba en 10 segundos...");
    delay(10000);  // Espera antes de reiniciar el ciclo
}
