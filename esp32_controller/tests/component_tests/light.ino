#include <Wire.h>
#include <Adafruit_TSL2561_U.h>

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

void setup() {
  Serial.begin(9600);

  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("Iniciando prueba del sensor de luz TSL2561...");
  while (!tsl.begin(&Wire)) {
    Serial.print(".");
    delay(1000);
  }

  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);
}

void loop() {
  sensors_event_t event;
  tsl.getEvent(&event);

  Serial.print("Luminosidad: ");
  Serial.print(event.light);
  Serial.println(" lux");

  delay(1000);
}
