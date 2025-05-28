#include "DHT.h"

#define DHT_PIN 13
#define DHTTYPE DHT11

DHT dht(DHT_PIN, DHTTYPE);

void setup() {
    Serial.begin(9600);
    dht.begin();
    Serial.println("Iniciando prueba del DHT11 ...");
}

void loop() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        Serial.println("Error al leer del DHT11");
        return;
    }

    Serial.print("Humedad: ");
    Serial.print(h);
    Serial.print(" %\t");

    Serial.print("Temperatura: ");
    Serial.print(t);
    Serial.println(" °C");
    delay(2000);
}
