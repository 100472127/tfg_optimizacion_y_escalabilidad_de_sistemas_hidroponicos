#define NUM_WTR_SENSORS 5

const int sensorPins[5] = {32, 33, 25, 26, 27};

bool sensorStates[5];

void setup() {
    Serial.begin(9600);

    for (int i = 0; i < NUM_WTR_SENSORS; i++) {
        pinMode(sensorPins[i], INPUT);
    }
    Serial.println("Iniciando prueba de sensores de nivel de agua ...");
}

void loop() {
    Serial.print("Agua detectada: ");

    for (int i = 0; i < 5; i++) {
        sensorStates[i] = !digitalRead(sensorPins[i]);
        Serial.print(sensorStates[i] ? "True" : "False");
        if (i < 4) Serial.print(" | ");
    }
    Serial.println();

    delay(2000); // Esperar 2 segundos antes de la siguiente lectura
}
