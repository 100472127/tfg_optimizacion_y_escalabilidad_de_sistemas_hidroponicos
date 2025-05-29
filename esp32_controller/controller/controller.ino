// Librerias
#include <WiFi.h>
#include <HTTPClient.h> 
#include <WebServer.h> 
#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <Fuzzy.h>
#include <Preferences.h>
#include "time.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>


// Identificador del microcontrolador
#define CONTROLLER_ID "1"


// Configuración de conexión WiFi
#define WIFI_SSID ""
#define WIFI_PSWD ""


// PINS
#define WATERLVLSENSORPIN 27
#define WATERLVLSENSORMAX 26
#define WATERLVLSENSORMEZMAX 25
#define WATERLVLSENSORMEZMIN 33
#define WATERLVLSENSORRESMAX 32
#define PHSENSORPIN 35
#define WATERQUALITYSENSORPIN 34
#define BOMBAACIDA 15
#define BOMBAALCALINA 2
#define BOMBAMEZCLA 4
#define BOMBANUTRIENTES 16
#define BOMBAMEZCLARES 17
#define BOMBARES 5
#define HUMIDITYSENSOR 13
#define DHTTYPE DHT11
#define SPRAY 14
#define FAN 19
#define HEATER 18
#define RESISTOR 36
#define LED 23
#define SDA_PIN 21
#define SCL_PIN 22

// Variable para guardar la IP que se le asignará al microcontrolador al conectarse a la red wifi
String ip_value;

// URL del servidor principal (Raspberry Pi)
const char* serverUrl = "http://192.168.73.200:3000";

// Creamos un pequeño servidor web local en el puerto 80
WebServer server(80);

// Variables de lectura y estado del sistema
float phRead = 0.0;
float waterqualityRead = 0.0;
float hours = 0.0;
float dataSensorsInfo[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int statusPumps[6] = {0, 0, 0, 0, 0, 0};
int currentPWMValue = 0;


// Sensores y actuadores
DHT dht(HUMIDITYSENSOR, DHTTYPE);
Servo sprayServo;
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT);


// Otras variables de sistema
const float RESISTOR_THRESHOLD = 1.0;
const float LUX_THRESHOLD = 500.0;
float lastTemperatureCheck = 0;
int fanIntensity = 0;
int heaterIntensity = 0;
int lastFanIntensity = -1;
int lastHeaterIntensity = -1;

const float OffsetPH = -2.5;
const float temperature = 25.0;
int lastPos = 0;
unsigned long lightHours = 0;
unsigned long startLight = 0;
float startDay = 0;
unsigned long counterMezcla = 0;
unsigned long lastSpray = 0;
unsigned long sprayUseInterval = 60;
unsigned long pumpUseInterval = 3600;
int manualORauto = 0;
unsigned long lastChronoOn = 0;


// Guardado en flash de variables
Preferences preferences;
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// Logica Difusa
Fuzzy *fuzzyTEMPERATURE = new Fuzzy();
Fuzzy *fuzzyPHTDS = new Fuzzy();
Fuzzy *fuzzyHUMIDITY = new Fuzzy();
Fuzzy *fuzzyLIGHT = new Fuzzy();

// Variables de ajuste de rangos difusos
float pHOptMin = 5.5;
float pHOptMax = 6.5;
float TDSOptMin = 800.0;
float TDSOptMax = 1200.0;
float HumOptMin = 60;
float HumOptMax = 75;
float TempOptMin = 14;
float TempOptMax = 18;
float LumOptMin = 10000;
float LumOptMax = 20000;
float LumOptHrsMin = 14;
float LumOptHrsMax = 16;

//Función para obtener el tiempo actual
unsigned long getTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return 0;
    }
    return mktime(&timeinfo);  // Convierte la estructura de tiempo en timestamp UNIX
}

// Funcion para obtener el dia actual
float getActualDay() {
    return (((getTime() / 86400) + 4) % 7);
}


// ------------------------------
// FUNCIONES CONTROL DEL PH y TDS
// ------------------------------

// checkPH(): evaluara el estado del PH cada minuto y actuara ajustandolo al rango deseado
void checkPHTDS()
{
    // Establecer entradas para el sistema difuso
    if (manualORauto == 0)
    {
        fuzzySystem->setInput(1, dataSensorsInfo[0]); // pH
        fuzzySystem->setInput(2, dataSensorsInfo[1]); // TDS
        // Fuzzificar las entradas
        fuzzySystem->fuzzify();
        // Defuzzificar las salidas
        float outputAlcalina = fuzzySystem->defuzzify(1);
        float outputAcida = fuzzySystem->defuzzify(2);
        float outputNutrientes = fuzzySystem->defuzzify(3);
        // Controlar bombas segun las salidas del sistema difuso
        int newStatusAlcalina = outputAlcalina > 0.5 ? 1 : 0;
        int newStatusAcida = outputAcida > 0.5 ? 1 : 0;
        int newStatusNutrientes = outputNutrientes > 0.5 ? 1 : 0;
        // Comprobar y actualizar el estado de la bomba alcalina
        if (statusPumps[1] != newStatusAlcalina && statusPumps[2] == 0)
        {
            statusPumps[1] = newStatusAlcalina;
            digitalWrite(BOMBAALCALINA, newStatusAlcalina);
        }
        // Comprobar y actualizar el estado de la bomba acida
        if (statusPumps[0] != newStatusAcida && statusPumps[2] == 0)
        {
            statusPumps[0] = newStatusAcida;
            digitalWrite(BOMBAACIDA, newStatusAcida);
        }
        // Comprobar y actualizar el estado de la bomba de nutrientes
        if (statusPumps[3] != newStatusNutrientes && statusPumps[2] == 0)
        {
            statusPumps[3] = newStatusNutrientes;
            digitalWrite(BOMBANUTRIENTES, newStatusNutrientes);
        }
    }
}

void setFuzzyPHTDS()
{
    fuzzyPHTDS = new Fuzzy();

    // Definir el conjunto difuso para pH
    FuzzyInput *inputPH = new FuzzyInput(1);
    FuzzySet *veryLowPH = new FuzzySet(0, 0, 3, 4, "MuyBajo");
    inputPH->addFuzzySet(veryLowPH);
    FuzzySet *lowPH = new FuzzySet(3, 3.5, 5, 5.5, "Bajo");
    inputPH->addFuzzySet(lowPH);
    FuzzySet *optimalPH = new FuzzySet(5, 5.5, 6.5, 7, "Optimo");
    inputPH->addFuzzySet(optimalPH);
    FuzzySet *highPH = new FuzzySet(6.5, 7, 8, 10, "Alto");
    inputPH->addFuzzySet(highPH);
    FuzzySet *veryHighPH = new FuzzySet(8, 8.5, 14, 14, "MuyAlto");
    inputPH->addFuzzySet(veryHighPH);
    fuzzyPHTDS->addFuzzyInput(inputPH);

    // Definir el conjunto difuso para TDS
    FuzzyInput *inputTDS = new FuzzyInput(2);
    FuzzySet *veryLowTDS = new FuzzySet(0, 0, 400, 500, "Muy Baja");
    inputTDS->addFuzzySet(veryLowTDS);
    FuzzySet *lowTDS = new FuzzySet(300, 500, 750, 800, "Baja");
    inputTDS->addFuzzySet(lowTDS);
    FuzzySet *optimalTDS = new FuzzySet(750, 800, 1200, 1250, "Optima");
    inputTDS->addFuzzySet(optimalTDS);
    FuzzySet *highTDS = new FuzzySet(1200, 1250, 1400, 1450, "Alta");
    inputTDS->addFuzzySet(highTDS);
    FuzzySet *veryHighTDS = new FuzzySet(1300, 1450, 1500, 1500, "Muy Alta");
    inputTDS->addFuzzySet(veryHighTDS);
    fuzzyPHTDS->addFuzzyInput(inputTDS);

    // Definir el conjunto difuso para bombas
    FuzzyOutput *outputBombaAlcalina = new FuzzyOutput(1);
    FuzzySet *activateAlkaline = new FuzzySet(0.5, 1, 1.5, "Activar");
    outputBombaAlcalina->addFuzzySet(activateAlkaline);
    FuzzySet *noActivateAlkaline = new FuzzySet(-0.5, 0, 0.5, "No Activar");
    outputBombaAlcalina->addFuzzySet(noActivateAlkaline);
    fuzzyPHTDS->addFuzzyOutput(outputBombaAlcalina);

    FuzzyOutput *outputBombaAcida = new FuzzyOutput(2);
    FuzzySet *activateAcidic = new FuzzySet(0.5, 1, 1.5, "Activar");
    outputBombaAcida->addFuzzySet(activateAcidic);
    FuzzySet *noActivateAcidic = new FuzzySet(-0.5, 0, 0.5, "No Activar");
    outputBombaAcida->addFuzzySet(noActivateAcidic);
    fuzzyPHTDS->addFuzzyOutput(outputBombaAcida);

    FuzzyOutput *outputBombaNutrientes = new FuzzyOutput(3);
    FuzzySet *activateNutrients = new FuzzySet(0.5, 1, 1.5, "Activar");
    outputBombaNutrientes->addFuzzySet(activateNutrients);
    FuzzySet *noActivateNutrients = new FuzzySet(-0.5, 0, 0.5, "No Activar");
    outputBombaNutrientes->addFuzzySet(noActivateNutrients);
    fuzzyPHTDS->addFuzzyOutput(outputBombaNutrientes);

    FuzzyRuleAntecedent *antecedentPHMuyBajoTDSMuyBajo = new FuzzyRuleAntecedent();
    antecedentPHMuyBajoTDSMuyBajo->joinWithAND(veryLowPH, veryLowTDS);
    FuzzyRuleAntecedent *antecedentPHMuyBajoTDSBajo = new FuzzyRuleAntecedent();
    antecedentPHMuyBajoTDSBajo->joinWithAND(veryLowPH, lowTDS);
    FuzzyRuleAntecedent *antecedentPHMuyBajoTDSOptimo = new FuzzyRuleAntecedent();
    antecedentPHMuyBajoTDSOptimo->joinWithAND(veryLowPH, optimalTDS);
    FuzzyRuleAntecedent *antecedentPHMuyBajoTDSHigh = new FuzzyRuleAntecedent();
    antecedentPHMuyBajoTDSHigh->joinWithAND(veryLowPH, highTDS);
    FuzzyRuleAntecedent *antecedentPHMuyBajoTDSVeryHigh = new FuzzyRuleAntecedent();
    antecedentPHMuyBajoTDSVeryHigh->joinWithAND(veryLowPH, veryHighTDS);
    FuzzyRuleAntecedent *antecedentPHBajoTDSMuyBajo = new FuzzyRuleAntecedent();
    antecedentPHBajoTDSMuyBajo->joinWithAND(lowPH, veryLowTDS);
    FuzzyRuleAntecedent *antecedentPHBajoTDSBajo = new FuzzyRuleAntecedent();
    antecedentPHBajoTDSBajo->joinWithAND(lowPH, lowTDS);
    FuzzyRuleAntecedent *antecedentPHBajoTDSOptimo = new FuzzyRuleAntecedent();
    antecedentPHBajoTDSOptimo->joinWithAND(lowPH, optimalTDS);
    FuzzyRuleAntecedent *antecedentPHBajoTDSAlta = new FuzzyRuleAntecedent();
    antecedentPHBajoTDSAlta->joinWithAND(lowPH, highTDS);
    FuzzyRuleAntecedent *antecedentPHBajoTDSMuyAlta = new FuzzyRuleAntecedent();
    antecedentPHBajoTDSMuyAlta->joinWithAND(lowPH, veryHighTDS);
    FuzzyRuleAntecedent *antecedentPHOptimoTDSMuyBajo = new FuzzyRuleAntecedent();
    antecedentPHOptimoTDSMuyBajo->joinWithAND(optimalPH, veryLowTDS);
    FuzzyRuleAntecedent *antecedentPHOptimoTDSBaja = new FuzzyRuleAntecedent();
    antecedentPHOptimoTDSBaja->joinWithAND(optimalPH, lowTDS);
    FuzzyRuleAntecedent *antecedentPHOptimoTDSOptimo = new FuzzyRuleAntecedent();
    antecedentPHOptimoTDSOptimo->joinWithAND(optimalPH, optimalTDS);
    FuzzyRuleAntecedent *antecedentPHOptimoTDSAlto = new FuzzyRuleAntecedent();
    antecedentPHOptimoTDSAlto->joinWithAND(optimalPH, highTDS);
    FuzzyRuleAntecedent *antecedentPHOptimoTDSMuyAlta = new FuzzyRuleAntecedent();
    antecedentPHOptimoTDSMuyAlta->joinWithAND(optimalPH, veryHighTDS);
    FuzzyRuleAntecedent *antecedentPHAltoTDSMuyBaja = new FuzzyRuleAntecedent();
    antecedentPHAltoTDSMuyBaja->joinWithAND(highPH, veryLowTDS);
    FuzzyRuleAntecedent *antecedentPHAltoTDSBajo = new FuzzyRuleAntecedent();
    antecedentPHAltoTDSBajo->joinWithAND(highPH, lowTDS);
    FuzzyRuleAntecedent *antecedentPHAltoTDSOptimo = new FuzzyRuleAntecedent();
    antecedentPHAltoTDSOptimo->joinWithAND(highPH, optimalTDS);
    FuzzyRuleAntecedent *antecedentPHAltoTDSAlta = new FuzzyRuleAntecedent();
    antecedentPHAltoTDSAlta->joinWithAND(highPH, highTDS);
    FuzzyRuleAntecedent *antecedentPHAltoTDSMuyAlta = new FuzzyRuleAntecedent();
    antecedentPHAltoTDSMuyAlta->joinWithAND(highPH, veryHighTDS);
    FuzzyRuleAntecedent *antecedentPHMuyAltoTDSMuyBaja = new FuzzyRuleAntecedent();
    antecedentPHMuyAltoTDSMuyBaja->joinWithAND(veryHighPH, veryLowTDS);
    FuzzyRuleAntecedent *antecedentPHMuyAltoTDSBaja = new FuzzyRuleAntecedent();
    antecedentPHMuyAltoTDSBaja->joinWithAND(veryHighPH, lowTDS);
    FuzzyRuleAntecedent *antecedentPHMuyAltoTDSOptimo = new FuzzyRuleAntecedent();
    antecedentPHMuyAltoTDSOptimo->joinWithAND(veryHighPH, optimalTDS);
    FuzzyRuleAntecedent *antecedentPHMuyAltoTDSAlta = new FuzzyRuleAntecedent();
    antecedentPHMuyAltoTDSAlta->joinWithAND(veryHighPH, highTDS);
    FuzzyRuleAntecedent *antecedentPHMuyAltoTDSMuyAlta = new FuzzyRuleAntecedent();
    antecedentPHMuyAltoTDSMuyAlta->joinWithAND(veryHighPH, veryHighTDS);
    FuzzyRuleConsequent *thenActivarBombaAlcalina = new FuzzyRuleConsequent();
    thenActivarBombaAlcalina->addOutput(activateAlkaline);
    FuzzyRuleConsequent *thenActivarBombaAcida = new FuzzyRuleConsequent();
    thenActivarBombaAcida->addOutput(activateAcidic);
    FuzzyRuleConsequent *thenActivarBombaNutrientes = new FuzzyRuleConsequent();
    thenActivarBombaNutrientes->addOutput(activateNutrients);
    FuzzyRuleConsequent *thenNoActivarBombaAlcalina = new FuzzyRuleConsequent();
    thenNoActivarBombaAlcalina->addOutput(noActivateAlkaline);
    FuzzyRuleConsequent *thenNoActivarBombaAcida = new FuzzyRuleConsequent();
    thenNoActivarBombaAcida->addOutput(noActivateAcidic);
    FuzzyRuleConsequent *thenNoActivarBombaNutrientes = new FuzzyRuleConsequent();
    thenNoActivarBombaNutrientes->addOutput(noActivateNutrients);

    FuzzyRule *fuzzyRule1 = new FuzzyRule(1, antecedentPHMuyBajoTDSMuyBajo, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule1);
    FuzzyRule *fuzzyRule2 = new FuzzyRule(2, antecedentPHMuyBajoTDSMuyBajo, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule2);
    FuzzyRule *fuzzyRule3 = new FuzzyRule(3, antecedentPHMuyBajoTDSMuyBajo, thenActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule3);
    FuzzyRule *fuzzyRule4 = new FuzzyRule(4, antecedentPHMuyBajoTDSBajo, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule4);
    FuzzyRule *fuzzyRule5 = new FuzzyRule(5, antecedentPHMuyBajoTDSBajo, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule5);
    FuzzyRule *fuzzyRule6 = new FuzzyRule(6, antecedentPHMuyBajoTDSBajo, thenActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule6);
    FuzzyRule *fuzzyRule7 = new FuzzyRule(7, antecedentPHMuyBajoTDSOptimo, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule7);
    FuzzyRule *fuzzyRule8 = new FuzzyRule(8, antecedentPHMuyBajoTDSOptimo, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule8);
    FuzzyRule *fuzzyRule9 = new FuzzyRule(9, antecedentPHMuyBajoTDSOptimo, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule9);
    FuzzyRule *fuzzyRule10 = new FuzzyRule(10, antecedentPHMuyBajoTDSHigh, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule10);
    FuzzyRule *fuzzyRule11 = new FuzzyRule(11, antecedentPHMuyBajoTDSHigh, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule11);
    FuzzyRule *fuzzyRule12 = new FuzzyRule(12, antecedentPHMuyBajoTDSHigh, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule12);
    FuzzyRule *fuzzyRule13 = new FuzzyRule(13, antecedentPHMuyBajoTDSVeryHigh, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule13);
    FuzzyRule *fuzzyRule14 = new FuzzyRule(14, antecedentPHMuyBajoTDSVeryHigh, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule14);
    FuzzyRule *fuzzyRule15 = new FuzzyRule(15, antecedentPHMuyBajoTDSVeryHigh, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule15);
    FuzzyRule *fuzzyRule16 = new FuzzyRule(16, antecedentPHBajoTDSMuyBajo, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule16);
    FuzzyRule *fuzzyRule17 = new FuzzyRule(17, antecedentPHBajoTDSMuyBajo, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule17);
    FuzzyRule *fuzzyRule18 = new FuzzyRule(18, antecedentPHBajoTDSMuyBajo, thenActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule18);
    FuzzyRule *fuzzyRule19 = new FuzzyRule(19, antecedentPHBajoTDSBajo, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule19);
    FuzzyRule *fuzzyRule20 = new FuzzyRule(20, antecedentPHBajoTDSBajo, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule20);
    FuzzyRule *fuzzyRule21 = new FuzzyRule(21, antecedentPHBajoTDSBajo, thenActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule21);
    FuzzyRule *fuzzyRule22 = new FuzzyRule(22, antecedentPHBajoTDSOptimo, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule22);
    FuzzyRule *fuzzyRule23 = new FuzzyRule(23, antecedentPHBajoTDSOptimo, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule23);
    FuzzyRule *fuzzyRule24 = new FuzzyRule(24, antecedentPHBajoTDSOptimo, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule24);
    FuzzyRule *fuzzyRule25 = new FuzzyRule(25, antecedentPHBajoTDSAlta, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule25);
    FuzzyRule *fuzzyRule26 = new FuzzyRule(26, antecedentPHBajoTDSAlta, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule26);
    FuzzyRule *fuzzyRule27 = new FuzzyRule(27, antecedentPHBajoTDSAlta, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule27);
    FuzzyRule *fuzzyRule28 = new FuzzyRule(28, antecedentPHBajoTDSMuyAlta, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule28);
    FuzzyRule *fuzzyRule29 = new FuzzyRule(29, antecedentPHBajoTDSMuyAlta, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule29);
    FuzzyRule *fuzzyRule30 = new FuzzyRule(30, antecedentPHBajoTDSMuyAlta, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule30);
    FuzzyRule *fuzzyRule31 = new FuzzyRule(31, antecedentPHOptimoTDSMuyBajo, thenNoActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule31);
    FuzzyRule *fuzzyRule32 = new FuzzyRule(32, antecedentPHOptimoTDSMuyBajo, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule32);
    FuzzyRule *fuzzyRule33 = new FuzzyRule(33, antecedentPHOptimoTDSMuyBajo, thenActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule33);
    FuzzyRule *fuzzyRule34 = new FuzzyRule(34, antecedentPHOptimoTDSBaja, thenNoActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule34);
    FuzzyRule *fuzzyRule35 = new FuzzyRule(35, antecedentPHOptimoTDSBaja, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule35);
    FuzzyRule *fuzzyRule36 = new FuzzyRule(36, antecedentPHOptimoTDSBaja, thenActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule36);
    FuzzyRule *fuzzyRule37 = new FuzzyRule(37, antecedentPHOptimoTDSOptimo, thenNoActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule37);
    FuzzyRule *fuzzyRule38 = new FuzzyRule(38, antecedentPHOptimoTDSOptimo, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule38);
    FuzzyRule *fuzzyRule39 = new FuzzyRule(39, antecedentPHOptimoTDSOptimo, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule39);
    FuzzyRule *fuzzyRule40 = new FuzzyRule(40, antecedentPHOptimoTDSAlto, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule40);
    FuzzyRule *fuzzyRule41 = new FuzzyRule(41, antecedentPHOptimoTDSAlto, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule41);
    FuzzyRule *fuzzyRule42 = new FuzzyRule(42, antecedentPHOptimoTDSAlto, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule42);
    FuzzyRule *fuzzyRule43 = new FuzzyRule(43, antecedentPHOptimoTDSMuyAlta, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule43);
    FuzzyRule *fuzzyRule44 = new FuzzyRule(44, antecedentPHOptimoTDSMuyAlta, thenNoActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule44);
    FuzzyRule *fuzzyRule45 = new FuzzyRule(45, antecedentPHOptimoTDSMuyAlta, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule45);
    FuzzyRule *fuzzyRule46 = new FuzzyRule(46, antecedentPHAltoTDSMuyBaja, thenNoActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule46);
    FuzzyRule *fuzzyRule47 = new FuzzyRule(47, antecedentPHAltoTDSMuyBaja, thenActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule47);
    FuzzyRule *fuzzyRule48 = new FuzzyRule(48, antecedentPHAltoTDSMuyBaja, thenActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule48);
    FuzzyRule *fuzzyRule49 = new FuzzyRule(49, antecedentPHAltoTDSBajo, thenNoActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule49);
    FuzzyRule *fuzzyRule50 = new FuzzyRule(50, antecedentPHAltoTDSBajo, thenActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule50);
    FuzzyRule *fuzzyRule51 = new FuzzyRule(51, antecedentPHAltoTDSBajo, thenActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule51);
    FuzzyRule *fuzzyRule52 = new FuzzyRule(52, antecedentPHAltoTDSOptimo, thenNoActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule52);
    FuzzyRule *fuzzyRule53 = new FuzzyRule(53, antecedentPHAltoTDSOptimo, thenActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule53);
    FuzzyRule *fuzzyRule54 = new FuzzyRule(54, antecedentPHAltoTDSOptimo, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule54);
    FuzzyRule *fuzzyRule55 = new FuzzyRule(55, antecedentPHAltoTDSAlta, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule55);
    FuzzyRule *fuzzyRule56 = new FuzzyRule(56, antecedentPHAltoTDSAlta, thenActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule56);
    FuzzyRule *fuzzyRule57 = new FuzzyRule(57, antecedentPHAltoTDSAlta, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule57);
    FuzzyRule *fuzzyRule58 = new FuzzyRule(58, antecedentPHAltoTDSMuyAlta, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule58);
    FuzzyRule *fuzzyRule59 = new FuzzyRule(59, antecedentPHAltoTDSMuyAlta, thenActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule59);
    FuzzyRule *fuzzyRule60 = new FuzzyRule(60, antecedentPHAltoTDSMuyAlta, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule60);
    FuzzyRule *fuzzyRule61 = new FuzzyRule(61, antecedentPHMuyAltoTDSMuyBaja, thenNoActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule61);
    FuzzyRule *fuzzyRule62 = new FuzzyRule(62, antecedentPHMuyAltoTDSMuyBaja, thenActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule62);
    FuzzyRule *fuzzyRule63 = new FuzzyRule(63, antecedentPHMuyAltoTDSMuyBaja, thenActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule63);
    FuzzyRule *fuzzyRule64 = new FuzzyRule(64, antecedentPHMuyAltoTDSBaja, thenNoActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule64);
    FuzzyRule *fuzzyRule65 = new FuzzyRule(65, antecedentPHMuyAltoTDSBaja, thenActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule65);
    FuzzyRule *fuzzyRule66 = new FuzzyRule(66, antecedentPHMuyAltoTDSBaja, thenActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule66);
    FuzzyRule *fuzzyRule67 = new FuzzyRule(67, antecedentPHMuyAltoTDSOptimo, thenNoActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule67);
    FuzzyRule *fuzzyRule68 = new FuzzyRule(68, antecedentPHMuyAltoTDSOptimo, thenActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule68);
    FuzzyRule *fuzzyRule69 = new FuzzyRule(69, antecedentPHMuyAltoTDSOptimo, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule69);
    FuzzyRule *fuzzyRule70 = new FuzzyRule(70, antecedentPHMuyAltoTDSAlta, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule70);
    FuzzyRule *fuzzyRule71 = new FuzzyRule(71, antecedentPHMuyAltoTDSAlta, thenActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule71);
    FuzzyRule *fuzzyRule72 = new FuzzyRule(72, antecedentPHMuyAltoTDSAlta, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule72);
    FuzzyRule *fuzzyRule73 = new FuzzyRule(73, antecedentPHMuyAltoTDSMuyAlta, thenActivarBombaAlcalina);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule73);
    FuzzyRule *fuzzyRule74 = new FuzzyRule(74, antecedentPHMuyAltoTDSMuyAlta, thenActivarBombaAcida);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule74);
    FuzzyRule *fuzzyRule75 = new FuzzyRule(75, antecedentPHMuyAltoTDSMuyAlta, thenNoActivarBombaNutrientes);
    fuzzyPHTDS->addFuzzyRule(fuzzyRule75);

    fuzzyPHTDS->setMethodOfInference(Fuzzy ::min);
    fuzzyPHTDS->setMethodOfAggregation(Fuzzy ::max);
    fuzzyPHTDS->setMethodOfDefuzzification(Fuzzy ::centroid);
}

void readingPHTDS()
{
    // Leer pH
    phRead = (3.5 * analogRead(PHSENSORPIN) * 5.0 / 4095 + OffsetPH);
    dataSensorsInfo[0] = phRead;

    // Leer calidad del agua (TDS)
    float coefCompensation = 1.0 + 0.02 * (temperature - 25.0);
    float voltCompensation = analogRead(WATERQUALITYSENSORPIN) / coefCompensation * 5 / 4095;
    waterqualityRead = (133.42 * voltCompensation * voltCompensation * voltCompensation - 255.86 * voltCompensation * voltCompensation + 857.39 * voltCompensation) * 0.5;
    dataSensorsInfo[1] = waterqualityRead;
}

// -------------------------------
// FUNCIONES CONTROL DE LA HUMEDAD
// -------------------------------

// checkDegreesSpray(): evita movimientos mayores a un rango de grados del servomotor
void checkDegreesSpray(int lastPos, int degrees)
{
    if (degrees != 0 && degrees != 45)
    {
        if (degrees < 0 || degrees > 45)
        {
            sprayServo.write(lastPos);
            if (abs(45 - degrees) < abs(0 - degrees))
            {
                sprayServo.write(45);
                lastPos = 45;
            }
            else if (abs(45 - degrees) > abs(0 - degrees))
            {
                sprayServo.write(0);
                lastPos = 0;
            }
        }
        else
        {
            if (lastPos == 45)
            {
                sprayServo.write(0);
                lastPos = 0;
            }
            else if (lastPos == 0)
            {
                sprayServo.write(45);
                lastPos = 45;
            }
        }
    }
    else
    {
        sprayServo.write(degrees);
        lastPos = degrees;
    }
}

// checkSprayUse():funcion de ejecuccion del spray
void checkSprayUse(float humidityActuatorsOutput)
{
    if (!isnan(dataSensorsInfo[3]) && (getTime() - lastSpray) >= sprayUseInterval)
    {
        if (humidityActuatorsOutput >= -0.5 && humidityActuatorsOutput < 0.5)
        {
            // No activar spray
            return;
        }
        else if (humidityActuatorsOutput >= 0.5 && humidityActuatorsOutput < 1.5)
        {
            // Activar spray 1 vez
            digitalWrite(BOMBAMEZCLA, 0);
            checkDegreesSpray(lastPos, 45);
            delay(1500);
            checkDegreesSpray(lastPos, 0);
            delay(1500);
        }
        else if (humidityActuatorsOutput >= 1.5 && humidityActuatorsOutput <= 2.5)
        {
            // Activar spray 2 veces
            for (int i = 0; i < 2; i++)
            {
                digitalWrite(BOMBAMEZCLA, 0);
                checkDegreesSpray(lastPos, 45);
                delay(1500);
                checkDegreesSpray(lastPos, 0);
                delay(1500);
            }
        }
        // Restaurar estado de la bomba
        digitalWrite(BOMBAMEZCLA, statusPumps[2]);
        lastSpray = getTime();
        preferences.begin("SprayInterval", false);
        preferences.putFloat("sprayUseIntervalRange", sprayUseInterval);
        preferences.end();
    }
}

// checkHUM(): evaluara el estado de la humedad y actuara en consecuencia
void checkHUM()
{
    float humidityFuzzyInput = dataSensorsInfo[3];
    fuzzyHUMIDITY->setInput(1, humidityFuzzyInput);
    fuzzyHUMIDITY->fuzzify();
    float humidityActuatorsOutput = fuzzyHUMIDITY->defuzzify(1);
    checkSprayUse(humidityActuatorsOutput);
}

// setFuzzyHUM(): genera el sistema de logica difusa del control de humedad
void setFuzzyHUM()
{
    fuzzyHUMIDITY = new Fuzzy();

    // Definicion de los conjuntos difusos para la entrada de humedad
    FuzzyInput *humidityReadedValue = new FuzzyInput(1);
    FuzzySet *muyBaja = new FuzzySet(0, 0, 49, 50);
    humidityReadedValue->addFuzzySet(muyBaja);
    FuzzySet *baja = new FuzzySet(49, 51, 58, 60);
    humidityReadedValue->addFuzzySet(baja);
    FuzzySet *optima = new FuzzySet(59, 60, 75, 80);
    humidityReadedValue->addFuzzySet(optima);
    FuzzySet *alta = new FuzzySet(70, 80, 100, 100);
    humidityReadedValue->addFuzzySet(alta);
    fuzzyHUMIDITY->addFuzzyInput(humidityReadedValue);

    // Definicion de las salidas para controlar el spray
    FuzzyOutput *accionSpray = new FuzzyOutput(1);
    FuzzySet *noActivar = new FuzzySet(-0.5, 0, 0.5);
    accionSpray->addFuzzySet(noActivar);
    FuzzySet *activar1 = new FuzzySet(0.5, 1, 1.5);
    accionSpray->addFuzzySet(activar1);
    FuzzySet *activar2 = new FuzzySet(1.5, 2, 2.5);
    accionSpray->addFuzzySet(activar2);
    fuzzyHUMIDITY->addFuzzyOutput(accionSpray);

    // Reglas difusas para el sistema de control de la humedad
    FuzzyRuleAntecedent *ifMuyBaja = new FuzzyRuleAntecedent();
    ifMuyBaja->joinSingle(muyBaja);
    FuzzyRuleConsequent *thenActivar2 = new FuzzyRuleConsequent();
    thenActivar2->addOutput(activar2);
    FuzzyRule *fuzzyRuleHUMIDITY01 = new FuzzyRule(1, ifMuyBaja, thenActivar2);
    fuzzyHUMIDITY->addFuzzyRule(fuzzyRuleHUMIDITY01);
    FuzzyRuleAntecedent *ifBaja = new FuzzyRuleAntecedent();
    ifBaja->joinSingle(baja);
    FuzzyRuleConsequent *thenActivar1 = new FuzzyRuleConsequent();
    thenActivar1->addOutput(activar1);
    FuzzyRule *fuzzyRuleHUMIDITY02 = new FuzzyRule(2, ifBaja, thenActivar1);
    fuzzyHUMIDITY->addFuzzyRule(fuzzyRuleHUMIDITY02);
    FuzzyRuleAntecedent *ifOptima = new FuzzyRuleAntecedent();
    ifOptima->joinSingle(optima);
    FuzzyRuleConsequent *thenNoActivar = new FuzzyRuleConsequent();
    thenNoActivar->addOutput(noActivar);
    FuzzyRule *fuzzyRuleHUMIDITY03 = new FuzzyRule(3, ifOptima, thenNoActivar);
    fuzzyHUMIDITY->addFuzzyRule(fuzzyRuleHUMIDITY03);
    FuzzyRuleAntecedent *ifAlta = new FuzzyRuleAntecedent();
    ifAlta->joinSingle(alta);
    FuzzyRuleConsequent *thenNoActivarHigh = new FuzzyRuleConsequent();
    thenNoActivarHigh->addOutput(noActivar);
    FuzzyRule *fuzzyRuleHUMIDITY04 = new FuzzyRule(4, ifAlta, thenNoActivarHigh);
    fuzzyHUMIDITY->addFuzzyRule(fuzzyRuleHUMIDITY04);
    fuzzyHUMIDITY->setMethodOfInference(Fuzzy ::min);
    fuzzyHUMIDITY->setMethodOfAggregation(Fuzzy ::max);
    fuzzyHUMIDITY->setMethodOfDefuzzification(Fuzzy ::centroid);
}

// humidityReading(): lectura de la humedad ambiente del sistema
void humidityReading()
{
    dataSensorsInfo[3] = dht.readHumidity();
}

// -----------------------------------
// FUNCIONES CONTROL DE LA TEMPERATURA
// -----------------------------------

void checkActuatorsIntensity(float temperatureActuatorsOutput)
{
    if (temperatureActuatorsOutput >= 1.5)
    {
        if (heaterIntensity == 0 && fanIntensity == 0)
        {
            heaterIntensity += 40;
        }
        else if (heaterIntensity == 0 && fanIntensity >= 40)
        {
            fanIntensity -= 40
        }
        else if (heaterIntensity == 0 && fanIntensity < 40)
        {
            dif = 40 - fanIntensity;
            fanIntensity = 0;
            heaterIntensity = dif;
        }
        else if (heaterIntensity > 0 && fanIntensity == 0)
        {
            heaterIntensity += 40;
        }
    }
    else if (temperatureActuatorsOutput >= 0.5)
    {
        if (heaterIntensity == 0 && fanIntensity == 0)
        {
            heaterIntensity += 20;
        }
        else if (heaterIntensity == 0 && fanIntensity >= 20)
        {
            fanIntensity -= 20
        }
        else if (heaterIntensity == 0 && fanIntensity < 20)
        {
            dif = 20 - fanIntensity;
            fanIntensity = 0;
            heaterIntensity = dif;
        }
        else if (heaterIntensity > 0 && fanIntensity == 0)
        {
            heaterIntensity += 20;
        }
    }
    if (temperatureActuatorsOutput <= -1.5)
    {
        if (heaterIntensity == 0 && fanIntensity == 0)
        {
            fanIntensity += 40;
        }
        else if (fanIntensity == 0 && heaterIntensity >= 40)
        {
            heaterIntensity -= 40
        }
        else if (fanIntensity == 0 && heaterIntensity < 40)
        {
            dif = 40 - heaterIntensity;
            heaterIntensity = 0;
            fanIntensity = dif;
        }
        else if (fanIntensity > 0 && heaterIntensity == 0)
        {
            fanIntensity += 40;
        }
    }
    else if (temperatureActuatorsOutput <= -0.5)
    {
        if (heaterIntensity == 0 && fanIntensity == 0)
        {
            fanIntensity += 20;
        }
        else if (fanIntensity == 0 && heaterIntensity >= 20)
        {
            heaterIntensity -= 20
        }
        else if (fanIntensity == 0 && heaterIntensity < 20)
        {
            dif = 20 - heaterIntensity;
            heaterIntensity = 0;
            fanIntensity = dif;
        }
        else if (fanIntensity > 0 && heaterIntensity == 0)
        {
            fanIntensity += 20;
        }
    }

    fanIntensity = constrain(fanIntensity, 0, 255);
    heaterIntensity = constrain(heaterIntensity, 0, 255);

    // Solo actualizar los actuadores si es necesario
    if (fanIntensity != lastFanIntensity)
    {
        analogWrite(FAN, fanIntensity);
        lastFanIntensity = fanIntensity;
    }
    if (heaterIntensity != lastHeaterIntensity)
    {
        analogWrite(HEATER, heaterIntensity);
        lastHeaterIntensity = heaterIntensity;
    }
}

// checkTemperatureControl(): funcion de control de la temperatura
void checkTemperatureControl(float temperatureActuatorsOutput)
{
    if (!isnan(dataSensorsInfo[2]) && (getTime() - lastTemperatureCheck) >= 30)
    { // Control de la temperatura cada 30 segundos
        checkActuatorsIntensity(temperatureActuatorsOutput);
        lastTemperatureCheck = getTime();
    }
}

// checkTEMP(): evaluara el estado de la temperatura y actuara en consecuencia
void checkTEMP()
{
    float temperatureFuzzyInput = dataSensorsInfo[2];
    fuzzyTEMPERATURE->setInput(1, temperatureFuzzyInput);
    fuzzyTEMPERATURE->fuzzify();
    float temperatureActuatorsOutput = fuzzyTEMPERATURE->defuzzify(1);
    checkTemperatureControl(temperatureActuatorsOutput);
}

void setFuzzyTEMP()
{
    fuzzyTEMPERATURE = new Fuzzy();

    // Definir el valor de entrada (lectura de temperatura)
    FuzzyInput *temperatureReadedValue = new FuzzyInput(1);

    // Funciones de membresia para la temperatura
    FuzzySet *veryLowTEMP = new FuzzySet(0, 0, 6, 8);
    temperatureReadedValue->addFuzzySet(veryLowTEMP);
    FuzzySet *lowTEMP = new FuzzySet(7, 9, 10, 14);
    temperatureReadedValue->addFuzzySet(lowTEMP);
    FuzzySet *optimalTEMP = new FuzzySet(13, 14, 18, 19);
    temperatureReadedValue->addFuzzySet(optimalTEMP);
    FuzzySet *highTEMP = new FuzzySet(18, 20, 23, 24);
    temperatureReadedValue->addFuzzySet(highTEMP);
    FuzzySet *veryHighTEMP = new FuzzySet(23, 25, 50, 50);
    temperatureReadedValue->addFuzzySet(veryHighTEMP);
    fuzzyTEMPERATURE->addFuzzyInput(temperatureReadedValue);

    // Definir la salida (control de temperatura)
    FuzzyOutput *temperatureControl = new FuzzyOutput(1);

    // Funciones de membresia para el control de temperatura
    FuzzySet *increaseMuch = new FuzzySet(1.5, 2, 2.5);
    temperatureControl->addFuzzySet(increaseMuch);
    FuzzySet *increase = new FuzzySet(0.5, 1, 1.5);
    temperatureControl->addFuzzySet(increase);
    FuzzySet *maintain = new FuzzySet(-0.5, 0, 0.5);
    temperatureControl->addFuzzySet(maintain);
    FuzzySet *decrease = new FuzzySet(-1.5, -1, -0.5);
    temperatureControl->addFuzzySet(decrease);
    FuzzySet *decreaseMuch = new FuzzySet(-2.5, -2, -1.5);
    temperatureControl->addFuzzySet(decreaseMuch);
    fuzzyTEMPERATURE->addFuzzyOutput(temperatureControl);

    // Reglas difusas
    FuzzyRuleAntecedent *ifVeryLowTemperature = new FuzzyRuleAntecedent();
    ifVeryLowTemperature->joinSingle(veryLowTEMP);
    FuzzyRuleConsequent *thenIncreaseMuch = new FuzzyRuleConsequent();
    thenIncreaseMuch->addOutput(increaseMuch);
    FuzzyRule *fuzzyRuleTEMP01 = new FuzzyRule(1, ifVeryLowTemperature, thenIncreaseMuch);
    fuzzyTEMPERATURE->addFuzzyRule(fuzzyRuleTEMP01);
    FuzzyRuleAntecedent *ifLowTemperature = new FuzzyRuleAntecedent();
    ifLowTemperature->joinSingle(lowTEMP);
    FuzzyRuleConsequent *thenIncrease = new FuzzyRuleConsequent();
    thenIncrease->addOutput(increase);
    FuzzyRule *fuzzyRuleTEMP02 = new FuzzyRule(2, ifLowTemperature, thenIncrease);
    fuzzyTEMPERATURE->addFuzzyRule(fuzzyRuleTEMP02);
    FuzzyRuleAntecedent *ifOptimalTemperature = new FuzzyRuleAntecedent();
    ifOptimalTemperature->joinSingle(optimalTEMP);
    FuzzyRuleConsequent *thenMaintain = new FuzzyRuleConsequent();
    thenMaintain->addOutput(maintain);
    FuzzyRule *fuzzyRuleTEMP03 = new FuzzyRule(3, ifOptimalTemperature, thenMaintain);
    fuzzyTEMPERATURE->addFuzzyRule(fuzzyRuleTEMP03);
    FuzzyRuleAntecedent *ifHighTemperature = new FuzzyRuleAntecedent();
    ifHighTemperature->joinSingle(highTEMP);
    FuzzyRuleConsequent *thenDecrease = new FuzzyRuleConsequent();
    thenDecrease->addOutput(decrease);
    FuzzyRule *fuzzyRuleTEMP04 = new FuzzyRule(4, ifHighTemperature, thenDecrease);
    fuzzyTEMPERATURE->addFuzzyRule(fuzzyRuleTEMP04);
    FuzzyRuleAntecedent *ifVeryHighTemperature = new FuzzyRuleAntecedent();
    ifVeryHighTemperature->joinSingle(veryHighTEMP);
    FuzzyRuleConsequent *thenDecreaseMuch = new FuzzyRuleConsequent();
    thenDecreaseMuch->addOutput(decreaseMuch);
    FuzzyRule *fuzzyRuleTEMP05 = new FuzzyRule(5, ifVeryHighTemperature, thenDecreaseMuch);

    fuzzyTEMPERATURE->addFuzzyRule(fuzzyRuleTEMP05);
    fuzzyTEMPERATURE->setMethodOfInference(Fuzzy ::min);
    fuzzyTEMPERATURE->setMethodOfAggregation(Fuzzy ::max);
    fuzzyTEMPERATURE->setMethodOfDefuzzification(Fuzzy ::centroid);
}

// tempReading(): lectura de la temperatura relativa del sistema
void tempReading()
{
    dataSensorsInfo[2] = dht.readTemperature();
}

// ---------------------
// FUNCIONES LUMINOSIDAD
// ---------------------

// checkLight():ajusta el crono con el tiempo de luz obtenida
void checkLight(float resistorValue, float lightSensorValue)
{
    if ((getTime() - startLight) >= 5)
    {
        if (lightSensorValue > LUX_THRESHOLD)
        {
            if (resistorValue < RESISTOR_THRESHOLD)
            {                                                   // Asegurarse de que el valor de la resistencia sea valido
                lightHours += (getTime() - startLight);         // Acumula el tiempo en segundos
                hours = lightHours / static_cast<float>(36000); // Paso a horas
            }
        }
        startLight = getTime();
    }
    // Guardar las horas de luz acumuladas en la memoria persistente
    preferences.begin("lHrsData", false);
    preferences.putULong("lHrsData", lightHours);
    preferences.end();
}

void adjustLED(float lightfuzzyOutput)
{
    int pwmValueChange = 0; // Cambios en el valor de PWM
    // Interpretar la salida del sistema difuso y determinar el cambio en el PWM
    if (lightfuzzyOutput < -0.5)
    {
        pwmValueChange = -currentPWMValue; // Eliminar luminosidad (apagado completo del LED)
    }
    else if (lightfuzzyOutput >= -0.5 && lightfuzzyOutput < 0.5)
    {
        pwmValueChange = -currentPWMValue; // Eliminar luminosidad (practicamente apagado)
    }
    else if (lightfuzzyOutput >= 0.5 && lightfuzzyOutput < 1.5)
    {
        pwmValueChange = -40; // Disminuir mucho luminosidad
    }
    else if (lightfuzzyOutput >= 1.5 && lightfuzzyOutput < 2.5)
    {
        pwmValueChange = -20; // Disminuir luminosidad
    }
    else if (lightfuzzyOutput >= 2.5 && lightfuzzyOutput < 3.5)
    {
        pwmValueChange = 0; // Mantener luminosidad
    }
    else if (lightfuzzyOutput >= 3.5 && lightfuzzyOutput < 4.5)
    {
        pwmValueChange = 20; // Aumentar luminosidad
    }
    else if (lightfuzzyOutput >= 4.5 && lightfuzzyOutput <= 5.5)
    {
        pwmValueChange = 40; // Aumentar mucho luminosidad
    }
    // Aplicar el cambio al valor actual de PWM
    currentPWMValue += pwmValueChange;
    // Limitar el valor de PWM entre 0 y 255
    currentPWMValue = constrain(currentPWMValue, 0, 255);
    // Ajustar el PWM del LED
    ledcWrite(0, currentPWMValue);
}

void checkLUM()
{
    float lightSensorValue = dataSensorsInfo[6];
    float resistorValue = dataSensorsInfo[4];
    checkLight(resistorValue, lightSensorValue);
    float lhours = hours;
    fuzzyLIGHT->setInput(1, lightSensorValue);
    fuzzyLIGHT->setInput(2, lhours);
    fuzzyLIGHT->fuzzify();
    float lightfuzzyOutput = fuzzyLIGHT->defuzzify(1);
    adjustLED(lightfuzzyOutput);
}

// setFuzzyLUM(): genera el sistema de logica difusa de la cantidad de luz de la plantacion
void setFuzzyLUM()
{
    fuzzyLIGHT = new Fuzzy();

    // Definicion del input ’Luminosidad’
    FuzzyInput *luminosidad = new FuzzyInput(1);
    FuzzySet *muyBajaLum = new FuzzySet(0, 0, 4000, 5000);           // MF2 - Muy Baja
    FuzzySet *bajaLum = new FuzzySet(4000, 5000, 9000, 10000);       // MF1 - Baja
    FuzzySet *optimaLum = new FuzzySet(9000, 10000, 20000, 21000);   // MF4 - Optima
    FuzzySet *altaLum = new FuzzySet(20000, 21000, 30000, 31000);    // MF3 - Alta
    FuzzySet *muyAltaLum = new FuzzySet(30000, 31000, 40000, 40000); // MF5 - Muy Alta
    luminosidad->addFuzzySet(muyBajaLum);
    luminosidad->addFuzzySet(bajaLum);
    luminosidad->addFuzzySet(optimaLum);
    luminosidad->addFuzzySet(altaLum);
    luminosidad->addFuzzySet(muyAltaLum);
    fuzzyLIGHT->addFuzzyInput(luminosidad);

    // Definicion del input ’HorasLuz’
    FuzzyInput *horasLuz = new FuzzyInput(2);
    FuzzySet *muyBajasHoras = new FuzzySet(0, 0, 10, 10.5);  // MF1 - Muy Bajas
    FuzzySet *bajasHoras = new FuzzySet(10, 11, 13, 14);     // MF2 - Bajas
    FuzzySet *optimasHoras = new FuzzySet(13.5, 14, 16, 16); // MF3 - Optimas
    FuzzySet *altasHoras = new FuzzySet(16, 17, 24, 24);     // MF4 - Altas
    horasLuz->addFuzzySet(muyBajasHoras);
    horasLuz->addFuzzySet(bajasHoras);
    horasLuz->addFuzzySet(optimasHoras);
    horasLuz->addFuzzySet(altasHoras);
    fuzzyLIGHT->addFuzzyInput(horasLuz);

    // Definicion del output ’LuminosidadLED’
    FuzzyOutput *luminosidadLED = new FuzzyOutput(1);
    FuzzySet *eliminarLum = new FuzzySet(-0.5, 0, 0.5);      // MF1 - Eliminar Luminosidad
    FuzzySet *disminuirMuchoLum = new FuzzySet(0.5, 1, 1.5); // MF2 - Disminuir Mucho Luminosidad
    FuzzySet *disminuirLum = new FuzzySet(1.5, 2, 2.5);      // MF3 - Disminuir Luminosidad
    FuzzySet *mantenerLum = new FuzzySet(2.5, 3, 3.5);       // MF4 - Mantener Luminosidad
    FuzzySet *aumentarLum = new FuzzySet(3.5, 4, 4.5);       // MF5 - Aumentar Luminosidad
    FuzzySet *aumentarMuchoLum = new FuzzySet(4.5, 5, 5.5);  // MF6 - Aumentar Mucho Luminosidad
    luminosidadLED->addFuzzySet(eliminarLum);
    luminosidadLED->addFuzzySet(disminuirMuchoLum);
    luminosidadLED->addFuzzySet(disminuirLum);
    luminosidadLED->addFuzzySet(mantenerLum);
    luminosidadLED->addFuzzySet(aumentarLum);
    luminosidadLED->addFuzzySet(aumentarMuchoLum);
    fuzzyLIGHT->addFuzzyOutput(luminosidadLED);

    // 1. Luminosidad Muy Baja , HorasLuz Muy Bajas => Aumentar Mucho Luminosidad
    FuzzyRuleAntecedent *ifLumMuyBajaHorasMuyBajas = new FuzzyRuleAntecedent();
    ifLumMuyBajaHorasMuyBajas->joinWithAND(muyBajaLum, muyBajasHoras);
    FuzzyRuleConsequent *thenAumentarMuchoLum = new FuzzyRuleConsequent();
    thenAumentarMuchoLum->addOutput(aumentarMuchoLum);
    FuzzyRule *rule1 = new FuzzyRule(1, ifLumMuyBajaHorasMuyBajas, thenAumentarMuchoLum);
    fuzzyLIGHT->addFuzzyRule(rule1);

    // 2. Luminosidad Muy Baja , HorasLuz Bajas => Aumentar Mucho Luminosidad
    FuzzyRuleAntecedent *ifLumMuyBajaHorasBajas = new FuzzyRuleAntecedent();
    ifLumMuyBajaHorasBajas->joinWithAND(muyBajaLum, bajasHoras);
    FuzzyRuleConsequent *thenAumentarMuchoLum2 = new FuzzyRuleConsequent();
    thenAumentarMuchoLum2->addOutput(aumentarMuchoLum);
    FuzzyRule *rule2 = new FuzzyRule(2, ifLumMuyBajaHorasBajas, thenAumentarMuchoLum2);
    fuzzyLIGHT->addFuzzyRule(rule2);

    // 3. Luminosidad Muy Baja , HorasLuz Optimas => Aumentar Mucho Luminosidad
    FuzzyRuleAntecedent *ifLumMuyBajaHorasOptimas = new FuzzyRuleAntecedent();
    ifLumMuyBajaHorasOptimas->joinWithAND(muyBajaLum, optimasHoras);
    FuzzyRuleConsequent *thenAumentarMuchoLum3 = new FuzzyRuleConsequent();
    thenAumentarMuchoLum3->addOutput(aumentarMuchoLum);
    FuzzyRule *rule3 = new FuzzyRule(3, ifLumMuyBajaHorasOptimas, thenAumentarMuchoLum3);
    fuzzyLIGHT->addFuzzyRule(rule3);

    // 4. Luminosidad Muy Baja , HorasLuz Altas => Eliminar Luminosidad
    FuzzyRuleAntecedent *ifLumMuyBajaHorasAltas = new FuzzyRuleAntecedent();
    ifLumMuyBajaHorasAltas->joinWithAND(muyBajaLum, altasHoras);
    FuzzyRuleConsequent *thenEliminarLum = new FuzzyRuleConsequent();
    thenEliminarLum->addOutput(eliminarLum);
    FuzzyRule *rule4 = new FuzzyRule(4, ifLumMuyBajaHorasAltas, thenEliminarLum);
    fuzzyLIGHT->addFuzzyRule(rule4);

    // 5. Luminosidad Baja , HorasLuz Muy Bajas => Aumentar Luminosidad
    FuzzyRuleAntecedent *ifLumBajaHorasMuyBajas = new FuzzyRuleAntecedent();
    ifLumBajaHorasMuyBajas->joinWithAND(bajaLum, muyBajasHoras);
    FuzzyRuleConsequent *thenAumentarLum = new FuzzyRuleConsequent();
    thenAumentarLum->addOutput(aumentarLum);
    FuzzyRule *rule5 = new FuzzyRule(5, ifLumBajaHorasMuyBajas, thenAumentarLum);
    fuzzyLIGHT->addFuzzyRule(rule5);

    // 6. Luminosidad Baja , HorasLuz Bajas => Aumentar Luminosidad
    FuzzyRuleAntecedent *ifLumBajaHorasBajas = new FuzzyRuleAntecedent();
    ifLumBajaHorasBajas->joinWithAND(bajaLum, bajasHoras);
    FuzzyRuleConsequent *thenAumentarLum2 = new FuzzyRuleConsequent();
    thenAumentarLum2->addOutput(aumentarLum);
    FuzzyRule *rule6 = new FuzzyRule(6, ifLumBajaHorasBajas, thenAumentarLum2);
    fuzzyLIGHT->addFuzzyRule(rule6);

    // 7. Luminosidad Baja , HorasLuz Optimas => Aumentar Luminosidad
    FuzzyRuleAntecedent *ifLumBajaHorasOptimas = new FuzzyRuleAntecedent();
    ifLumBajaHorasOptimas->joinWithAND(bajaLum, optimasHoras);
    FuzzyRuleConsequent *thenAumentarLum3 = new FuzzyRuleConsequent();
    thenAumentarLum3->addOutput(aumentarLum);
    FuzzyRule *rule7 = new FuzzyRule(7, ifLumBajaHorasOptimas, thenAumentarLum3);
    fuzzyLIGHT->addFuzzyRule(rule7);

    // 8. Luminosidad Baja , HorasLuz Altas => Eliminar Luminosidad
    FuzzyRuleAntecedent *ifLumBajaHorasAltas = new FuzzyRuleAntecedent();
    ifLumBajaHorasAltas->joinWithAND(bajaLum, altasHoras);
    FuzzyRuleConsequent *thenEliminarLum2 = new FuzzyRuleConsequent();
    thenEliminarLum2->addOutput(eliminarLum);
    FuzzyRule *rule8 = new FuzzyRule(8, ifLumBajaHorasAltas, thenEliminarLum2);
    fuzzyLIGHT->addFuzzyRule(rule8);

    // 9. Luminosidad Optima , HorasLuz Muy Bajas => Mantener Luminosidad
    FuzzyRuleAntecedent *ifLumOptimaHorasMuyBajas = new FuzzyRuleAntecedent();
    ifLumOptimaHorasMuyBajas->joinWithAND(optimaLum, muyBajasHoras);
    FuzzyRuleConsequent *thenMantenerLum = new FuzzyRuleConsequent();
    thenMantenerLum->addOutput(mantenerLum);
    FuzzyRule *rule9 = new FuzzyRule(9, ifLumOptimaHorasMuyBajas, thenMantenerLum);
    fuzzyLIGHT->addFuzzyRule(rule9);

    // 10. Luminosidad Optima , HorasLuz Bajas => Mantener Luminosidad
    FuzzyRuleAntecedent *ifLumOptimaHorasBajas = new FuzzyRuleAntecedent();
    ifLumOptimaHorasBajas->joinWithAND(optimaLum, bajasHoras);
    FuzzyRuleConsequent *thenMantenerLum2 = new FuzzyRuleConsequent();
    thenMantenerLum2->addOutput(mantenerLum);
    FuzzyRule *rule10 = new FuzzyRule(10, ifLumOptimaHorasBajas, thenMantenerLum2);
    fuzzyLIGHT->addFuzzyRule(rule10);

    // 11. Luminosidad Optima , HorasLuz Optimas => Mantener Luminosidad
    FuzzyRuleAntecedent *ifLumOptimaHorasOptimas = new FuzzyRuleAntecedent();
    ifLumOptimaHorasOptimas->joinWithAND(optimaLum, optimasHoras);
    FuzzyRuleConsequent *thenMantenerLum3 = new FuzzyRuleConsequent();
    thenMantenerLum3->addOutput(mantenerLum);
    FuzzyRule *rule11 = new FuzzyRule(11, ifLumOptimaHorasOptimas, thenMantenerLum3);
    fuzzyLIGHT->addFuzzyRule(rule11);

    // 12. Luminosidad Optima , HorasLuz Altas => Eliminar Luminosidad
    FuzzyRuleAntecedent *ifLumOptimaHorasAltas = new FuzzyRuleAntecedent();
    ifLumOptimaHorasAltas->joinWithAND(optimaLum, altasHoras);
    FuzzyRuleConsequent *thenEliminarLum3 = new FuzzyRuleConsequent();
    thenEliminarLum3->addOutput(eliminarLum);
    FuzzyRule *rule12 = new FuzzyRule(12, ifLumOptimaHorasAltas, thenEliminarLum3);
    fuzzyLIGHT->addFuzzyRule(rule12);

    // 13. Luminosidad Alta , HorasLuz Muy Bajas => Disminuir Luminosidad
    FuzzyRuleAntecedent *ifLumAltaHorasMuyBajas = new FuzzyRuleAntecedent();
    ifLumAltaHorasMuyBajas->joinWithAND(altaLum, muyBajasHoras);
    FuzzyRuleConsequent *thenDisminuirLum = new FuzzyRuleConsequent();
    thenDisminuirLum->addOutput(disminuirLum);
    FuzzyRule *rule13 = new FuzzyRule(13, ifLumAltaHorasMuyBajas, thenDisminuirLum);
    fuzzyLIGHT->addFuzzyRule(rule13);

    // 14. Luminosidad Alta , HorasLuz Bajas => Disminuir Luminosidad
    FuzzyRuleAntecedent *ifLumAltaHorasBajas = new FuzzyRuleAntecedent();
    ifLumAltaHorasBajas->joinWithAND(altaLum, bajasHoras);
    FuzzyRuleConsequent *thenDisminuirLum2 = new FuzzyRuleConsequent();
    thenDisminuirLum2->addOutput(disminuirLum);
    FuzzyRule *rule14 = new FuzzyRule(14, ifLumAltaHorasBajas, thenDisminuirLum2);
    fuzzyLIGHT->addFuzzyRule(rule14);

    // 15. Luminosidad Alta , HorasLuz Optimas => Disminuir Luminosidad
    FuzzyRuleAntecedent *ifLumAltaHorasOptimas = new FuzzyRuleAntecedent();
    ifLumAltaHorasOptimas->joinWithAND(altaLum, optimasHoras);
    FuzzyRuleConsequent *thenDisminuirLum3 = new FuzzyRuleConsequent();
    thenDisminuirLum3->addOutput(disminuirLum);
    FuzzyRule *rule15 = new FuzzyRule(15, ifLumAltaHorasOptimas, thenDisminuirLum3);
    fuzzyLIGHT->addFuzzyRule(rule15);

    // 16. Luminosidad Alta , HorasLuz Altas => Eliminar Luminosidad
    FuzzyRuleAntecedent *ifLumAltaHorasAltas = new FuzzyRuleAntecedent();
    ifLumAltaHorasAltas->joinWithAND(altaLum, altasHoras);
    FuzzyRuleConsequent *thenEliminarLum4 = new FuzzyRuleConsequent();
    thenEliminarLum4->addOutput(eliminarLum);
    FuzzyRule *rule16 = new FuzzyRule(16, ifLumAltaHorasAltas, thenEliminarLum4);
    fuzzyLIGHT->addFuzzyRule(rule16);

    // 17. Luminosidad Muy Alta , HorasLuz Muy Bajas => Disminuir Mucho Luminosidad
    FuzzyRuleAntecedent *ifLumMuyAltaHorasMuyBajas = new FuzzyRuleAntecedent();
    ifLumMuyAltaHorasMuyBajas->joinWithAND(muyAltaLum, muyBajasHoras);
    FuzzyRuleConsequent *thenDisminuirMuchoLum = new FuzzyRuleConsequent();
    thenDisminuirMuchoLum->addOutput(disminuirMuchoLum);
    FuzzyRule *rule17 = new FuzzyRule(17, ifLumMuyAltaHorasMuyBajas, thenDisminuirMuchoLum);
    fuzzyLIGHT->addFuzzyRule(rule17);

    // 18. Luminosidad Muy Alta , HorasLuz Bajas => Disminuir Mucho Luminosidad
    FuzzyRuleAntecedent *ifLumMuyAltaHorasBajas = new FuzzyRuleAntecedent();
    ifLumMuyAltaHorasBajas->joinWithAND(muyAltaLum, bajasHoras);
    FuzzyRuleConsequent *thenDisminuirMuchoLum2 = new FuzzyRuleConsequent();
    thenDisminuirMuchoLum2->addOutput(disminuirMuchoLum);
    FuzzyRule *rule18 = new FuzzyRule(18, ifLumMuyAltaHorasBajas, thenDisminuirMuchoLum2);
    fuzzyLIGHT->addFuzzyRule(rule18);

    // 19. Luminosidad Muy Alta , HorasLuz Optimas => Disminuir Mucho Luminosidad
    FuzzyRuleAntecedent *ifLumMuyAltaHorasOptimas = new FuzzyRuleAntecedent();
    ifLumMuyAltaHorasOptimas->joinWithAND(muyAltaLum, optimasHoras);
    FuzzyRuleConsequent *thenDisminuirMuchoLum3 = new FuzzyRuleConsequent();
    thenDisminuirMuchoLum3->addOutput(disminuirMuchoLum);
    FuzzyRule *rule19 = new FuzzyRule(19, ifLumMuyAltaHorasOptimas, thenDisminuirMuchoLum3);
    fuzzyLIGHT->addFuzzyRule(rule19);

    // 20. Luminosidad Muy Alta , HorasLuz Altas => Eliminar Luminosidad
    FuzzyRuleAntecedent *ifLumMuyAltaHorasAltas = new FuzzyRuleAntecedent();
    ifLumMuyAltaHorasAltas->joinWithAND(muyAltaLum, altasHoras);
    FuzzyRuleConsequent *thenEliminarLum5 = new FuzzyRuleConsequent();
    thenEliminarLum5->addOutput(eliminarLum);
    FuzzyRule *rule20 = new FuzzyRule(20, ifLumMuyAltaHorasAltas, thenEliminarLum5);
    fuzzyLIGHT->addFuzzyRule(rule20);

    fuzzyLIGHT->setMethodOfInference(Fuzzy ::min);
    fuzzyLIGHT->setMethodOfAggregation(Fuzzy ::max);
    fuzzyLIGHT->setMethodOfDefuzzification(Fuzzy ::centroid);
}

void lightResistorReading()
{
    dataSensorsInfo[4] = analogRead(RESISTOR) / 4095.0 * 100;
}

void lightSensorReading()
{
    sensors_event_t event;
    tsl.getEvent(&event);
    if (event.light)
    {
        dataSensorsInfo[6] = event.light; // Actualiza el arreglo con el valor leido
    }
    else
    {
        dataSensorsInfo[6] = 0; // Valor por defecto en caso de error
    }
}

// ifDayChanged(): comprobacion del cambio de dia y reajuste del crono de luz
void ifDayChanged()
{
    if (getActualDay() != startDay)
    {
        startLight = getTime();
        lightHours = 0;
        startDay = getActualDay();
        preferences.begin("startDayData", false);
        preferences.putFloat("startDayData", startDay);
        preferences.end();
    }
    preferences.begin("lHrsData", false);
    preferences.putULong("lHrsData", lightHours);
    preferences.end();
}

// ---------------------------
// FUNCIONES DEL NIVEL DE AGUA
// ---------------------------

void leerSensorMinPlan()
{
    dataSensorsInfo[5] = digitalRead(WATERLVLSENSORPIN);
}

void leerSensorMaxPlan()
{
    dataSensorsInfo[10] = digitalRead(WATERLVLSENSORMAX);
}

void leerSensorMinMez()
{
    dataSensorsInfo[7] = digitalRead(WATERLVLSENSORMEZMIN);
}

void leerSensorMaxMez()
{
    dataSensorsInfo[8] = digitalRead(WATERLVLSENSORMEZMAX);
}

void leerSensorRes()
{
    dataSensorsInfo[9] = digitalRead(WATERLVLSENSORRESMAX);
}

void controlLiquidos()
{
    unsigned long time;
    int optimo = 0;
    if (manualORauto == 0)
    {
        digitalWrite(BOMBAMEZCLA, 0);
        digitalWrite(BOMBAACIDA, 0);
        digitalWrite(BOMBAALCALINA, 0);
        digitalWrite(BOMBANUTRIENTES, 0);
        digitalWrite(BOMBARES, 0);
        digitalWrite(BOMBAMEZCLARES, 0);
        statusPumps[0] = 0;
        statusPumps[1] = 0;
        statusPumps[2] = 0;
        statusPumps[3] = 0;
        statusPumps[4] = 0;
        statusPumps[5] = 0;

        // BLOQUE 1
        if (dataSensorsInfo[7] == 0)
        { // Si no hay agua minima en el tanque de mezcla
            digitalWrite(BOMBAACIDA, 1);
            digitalWrite(BOMBAALCALINA, 1);
            digitalWrite(BOMBANUTRIENTES, 1);
            statusPumps[0] = 1;
            statusPumps[1] = 1;
            statusPumps[3] = 1;
            while (dataSensorsInfo[7] == 0)
            { // Mientras no haya agua en el sensor minimo de mezcla
                leerSensorMinMez();
            }
            digitalWrite(BOMBANUTRIENTES, 0);
            digitalWrite(BOMBAACIDA, 0);
            digitalWrite(BOMBAALCALINA, 0);
            statusPumps[0] = 0;
            statusPumps[1] = 0;
            statusPumps[3] = 0;
        }

        // BLOQUE 2
        if (dataSensorInfo[9] == 1)
        { // Si el tanque de residuos esta al maximo desaguar durante 10 segundos
            digitalWrite(BOMBARES, 1);
            statusPumps[5] = 1;
            time = millis();
            while (millis() - time < 10000 || dataSensorInfo[9] == 1)
            {
                leerSensorRes();
            }
            digitalWrite(BOMBARES, 0);
            statusPumps[5] = 0;
        }

        // BLOQUE 3
        if (dataSensorsInfo[8] == 1)
        { // Si el tanque de mezcla esta al maximo desaguar del tanque de mezcla al tanque de residuos y vaciar también el tanque de residuos
            digitalWrite(BOMBAMEZCLARES, 1);
            digitalWrite(BOMBARES, 1);
            statusPumps[4] = 1;
            statusPumps[5] = 1;
            time = millis();
            while ((millis() - time) < 10000 || dataSensorInfo[8] == 1)
            {
                leerSensorMaxMez();
            }
            digitalWrite(BOMBAMEZCLARES, 0);
            digitalWrite(BOMBARES, 0);
            statusPumps[4] = 0;
            statusPumps[5] = 0;
        }

        // BLOQUE 4
        while (optimo == 0)
        {
            if (dataSensorsInfo[8] == 1) // Si el tanque de mezcla esta al maximo
            {
                digitalWrite(BOMBAMEZCLARES, 1);
                digitalWrite(BOMBARES, 1);
                statusPumps[4] = 1;
                statusPumps[5] = 1;
                time = millis();
                while (millis() - time < 10000 || (dataSensorsInfo[8] == 1))
                {
                    leerSensorMaxMez();
                }
                digitalWrite(BOMBAMEZCLARES, 0);
                digitalWrite(BOMBARES, 0);
                statusPumps[4] = 0;
                statusPumps[5] = 0;
                readingPHTDS();
                checkPHTDS();
                time = millis();
                while ((millis() - time < 10000) && (dataSensorsInfo[8] == 0))
                {
                }
                if (statusPumps[0] = 0 && statusPumps[1] = 0 && statusPumps[3] = 0)
                {
                    optimo = 1;
                }
                digitalWrite(BOMBANUTRIENTES, 0);
                digitalWrite(BOMBAACIDA, 0);
                digitalWrite(BOMBAALCALINA, 0);
                statusPumps[0] = 0;
                statusPumps[1] = 0;
                statusPumps[3] = 0;
            }
            else if (dataSensorsInfo[8] == 0) // Si el tanque de mezcla no esta al maximo
            {
                readingPHTDS();
                checkPHTDS();
                time = millis();
                while ((millis() - time < 10000) && (dataSensorsInfo[8] == 0))
                {
                }
                if (statusPumps[0] = 0 && statusPumps[1] = 0 && statusPumps[3] = 0)
                {
                    optimo = 1;
                }
                digitalWrite(BOMBANUTRIENTES, 0);
                digitalWrite(BOMBAACIDA, 0);
                digitalWrite(BOMBAALCALINA, 0);
                statusPumps[0] = 0;
                statusPumps[1] = 0;
                statusPumps[3] = 0;
            }
        }
        if (dataSensorsInfo[8] == 1) // Si el tanque de mezcla esta al maximo
        {
            digitalWrite(BOMBAMEZCLARES, 1);
            digitalWrite(BOMBARES, 1);
            statusPumps[4] = 1;
            statusPumps[5] = 1;
            time = millis();
            while (millis() - time < 10000 || (dataSensorsInfo[8] == 1))
            {
                leerSensorMaxMez();
            }
            digitalWrite(BOMBAMEZCLARES, 0);
            digitalWrite(BOMBARES, 0);
            statusPumps[4] = 0;
            statusPumps[5] = 0;
        }
        // BLOQUE 5
        if (dataSensorsInfo[10] == 1) // Si el tanque de platanción está al máximo
        {
            digitalWrite(BOMBAMEZCLA, 0);
            statusPumps[2] = 0;
        }
        else if (dataSensorsInfo[10] == 0) // Si el tanque de plantación no está al máximo
        {
            if (dataSensorsInfo[5] == 0) // Si el tanque de platanción no llega al mínimo
            {
                counterMezcla = getTime();
                digitalWrite(BOMBAMEZCLA, 1);
                statusPumps[2] = 1;
                time = millis();
                while ((millis() - time < 20000) && (dataSensorsInfo[5] == 0) && (dataSensorsInfo[10] == 0) && (dataSensorsInfo[7] == 1))
                {
                }
                digitalWrite(BOMBAMEZCLA, 0);
                statusPumps[2] = 0;
            }
            else if (dataSensorsInfo[5] == 1) // Si el tanque de platanción llega al mínimo
            {
                if ((getTime() - counterMezcla) >= pumpUseInterval)
                {
                    counterMezcla = getTime();
                    digitalWrite(BOMBAMEZCLA, 1);
                    statusPumps[2] = 1;
                    time = millis();
                    while ((millis() - time < 20000) && (dataSensorsInfo[10] == 0) && (dataSensorsInfo[7] == 1))
                    {
                    }
                    digitalWrite(BOMBAMEZCLA, 0);
                    statusPumps[2] = 0;
                }
            }
        }
    }
}


//-----------------------------------------------
//FUNCIONES NUEVAS PARA EL CAMBIO DE ARQUITECTURA
//-----------------------------------------------

// Función para enviar la información que detectan los sensores al servidor principal
void sendData (){
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String fullUrl = String(serverUrl) + "/data/" + String(CONTROLLER_ID);
        http.begin(fullUrl);
        http.addHeader("Content-Type", "application/json");

        // Crear el JSON con los datos
        String jsonData = "{";
        jsonData += "\"phRead\":\"" + String(dataSensorsInfo[0]) + "\",";
        jsonData += "\"waterQualityRead\":\"" + String(dataSensorsInfo[1]) + "\",";
        jsonData += "\"tempRead\":\"" + String(dataSensorsInfo[2]) + "\",";
        jsonData += "\"humidityRead\":\"" + String(dataSensorsInfo[3]) + "\",";
        jsonData += "\"lightResistorRead\":\"" + String(dataSensorsInfo[4]) + "\",";
        jsonData += "\"lightSensorRead\":\"" + String(dataSensorsInfo[6]) + "\",";
        jsonData += "\"waterLvlMinPlanRead\":\"" + String(dataSensorsInfo[5]) + "\",";
        jsonData += "\"waterLvlMaxPlanRead\":\"" + String(dataSensorsInfo[10]) + "\",";
        jsonData += "\"waterLvlMinMezRead\":\"" + String(dataSensorsInfo[7]) + "\",";
        jsonData += "\"waterLvlMaxMezRead\":\"" + String(dataSensorsInfo[8]) + "\",";
        jsonData += "\"waterLvlMinResRead\":\"" + String(dataSensorsInfo[9]) + "\",";
        
        jsonData += "\"statusBombaAcida\":\"" + String(statusPumps[0]) + "\",";
        jsonData += "\"statusBombaAlcalina\":\"" + String(statusPumps[1]) + "\",";
        jsonData += "\"statusBombaMezcla\":\"" + String(statusPumps[2]) + "\",";
        jsonData += "\"statusBombaNutrientes\":\"" + String(statusPumps[3]) + "\",";
        jsonData += "\"statusBombaMezclaRes\":\"" + String(statusPumps[4]) + "\",";
        jsonData += "\"statusBombaRes\":\"" + String(statusPumps[5]) + "\",";

        jsonData += "\"pHOptMin\":\"" + String(pHOptMin) + "\",";
        jsonData += "\"pHOptMax\":\"" + String(pHOptMax) + "\",";
        jsonData += "\"TDSOptMin\":\"" + String(TDSOptMin) + "\",";
        jsonData += "\"TDSOptMax\":\"" + String(TDSOptMax) + "\",";
        jsonData += "\"HumOptMin\":\"" + String(HumOptMin) + "\",";
        jsonData += "\"HumOptMax\":\"" + String(HumOptMax) + "\",";
        jsonData += "\"TempOptMin\":\"" + String(TempOptMin) + "\",";
        jsonData += "\"TempOptMax\":\"" + String(TempOptMax) + "\",";
        jsonData += "\"LumOptMin\":\"" + String(LumOptMin) + "\",";
        jsonData += "\"LumOptMax\":\"" + String(LumOptMax) + "\",";
        jsonData += "\"LumOptHrsMin\":\"" + String(LumOptHrsMin) + "\",";
        jsonData += "\"LumOptHrsMax\":\"" + String(LumOptHrsMax) + "\",";
        jsonData += "\"pumpMode\":\"" + String(manualORauto) + "\"";
        jsonData += "}";
    
        int httpResponseCode = http.POST(jsonData);

        if (httpResponseCode == 200) {
            Serial.println("OK: /data/" + String(CONTROLLER_ID) + " (" + httpResponseCode + ") " + http.getString());
        } else {
            Serial.println("ERROR: /data/" + String(CONTROLLER_ID) + " (" + httpResponseCode + ") " + http.getString());
        }
        http.end();
    } else {
        Serial.println("ERROR: /sendData: No conectado a WiFi");
    }
}

// Función que se ejecuta en el setup para enviar el ID y la IP asignada al microcontrolador al servidor principal
void sendIP(){
    if(WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String fullUrl = String(serverUrl) + "/ipAssignment";
        http.begin(fullUrl);
        http.addHeader("Content-Type", "application/json");
        ip_value = WiFi.localIP().toString();

        // Crear el JSON con los datos
        String jsonData = "{";
        jsonData += "\"" + String(CONTROLLER_ID) + "\": \"" + String(ip_value) + "\"";
        jsonData += "}";

        int httpResponseCode = http.POST(jsonData);

        if (httpResponseCode == 200) {
            Serial.println("OK: /ipAssignment (" + httpResponseCode + ") " + http.getString());
        } else {
            Serial.println("ERROR: /ipAssignment (" + httpResponseCode + ") " + http.getString());
        }

        http.end(); // Finalizar conexión
    } else {
        Serial.println("ERROR: /ipAssignment: No conectado a WiFi");
    }
}

// Función que extrae el los valores de la petición POST {value: "valor"}, y los devuelve en un vector
String extractValues() {
    String jsonString = server.arg("plain");
    String value = "";
    // Deserializar el JSON para extraer el valor
    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, jsonString);
    if (!error) {
        value = doc["value"].as<String>();
    }
    return value;
}



//--------------------------------------------------------------
//FUNCIONES PARA GESTIONAR LAS PETICIONES DEL SERVIDOR PRINCIPAL
//--------------------------------------------------------------


// Al recibir la petición cambia el estatus de la bomba acida a 1 o 0
void handleBombaAcida(){
    if (manualORauto == 1) {
        statusPumps[0] = extractValues().toInt();
        digitalWrite(BOMBAACIDA, statusPumps[0]);
        Serial.println("Valor de la bomba acida: " + String(statusPumps[0]));
        server.send(200, "application/json", "{\"mensaje\":\"Status bomba acida actualizada\"}");
    } else {
        server.send(400, "application/json", "{\"mensaje\":\"El modo manual está desactivado.\"}");
    }
}

// Al recibir la petición cambia el estatus de la bomba alcalina a 1 o 0
void handleBombaAlcalina(){
    if (manualORauto == 1){
        statusPumps[1] = extractValues().toInt();
        digitalWrite(BOMBAACIDA, statusPumps[1]);
        Serial.println("Valor de la bomba alcalina: " + String(statusPumps[1]));
        server.send(200, "application/json", "{\"mensaje\":\"Status bomba alcalina actualizada\"}");
    } else {
        server.send(400, "application/json", "{\"mensaje\":\"El modo manual está desactivado.\"}");
    }
}

// Al recibir la petición cambia el estatus de la bomba de mezcla a 1 o 0
void handleBombaMezcla(){
    if (manualORauto == 1){
        statusPumps[2] = extractValues().toInt();
        digitalWrite(BOMBAACIDA, statusPumps[2]);
        Serial.println("Valor de la bomba mezcla: " + String(statusPumps[2]));
        server.send(200, "application/json", "{\"mensaje\":\"Status bomba mezcla actualizada\"}");
    } else{
        server.send(400, "application/json", "{\"mensaje\":\"El modo manual está desactivado.\"}");
    }
}

// Al recibir la petición cambia el estatus de la bomba de mezcla a 1 o 0
void handleBombaNutrientes(){
    if (manualORauto == 1){
        statusPumps[3] = extractValues().toInt();
        digitalWrite(BOMBANUTRIENTES, statusPumps[3]);
        Serial.println("Valor de la bomba nutrientes: " + String(statusPumps[3]));
        server.send(200, "application/json", "{\"mensaje\":\"Status bomba nutrientes actualizada\"}");
    } else {
        server.send(400, "application/json", "{\"mensaje\":\"El modo manual está desactivado.\"}");
    }
}

// Al recibir la petición cambia el estatus de la bomba de mezcla a 1 o 0
void handleBombaMezclaResiduos(){
    if (manualORauto == 1){
        statusPumps[4] = extractValues().toInt();
        digitalWrite(BOMBAACIDA, statusPumps[4]);
        Serial.println("Valor de la bomba mezclaResiduos: " + String(statusPumps[4]));
        server.send(200, "application/json", "{\"mensaje\":\"Status bomba mezclaResiduos actualizada\"}");    
    } else {
        server.send(400, "application/json", "{\"mensaje\":\"El modo manual está desactivado.\"}");
    }
}

// Al recibir la petición cambia el estatus de la bomba de mezcla a 1 o 0
void handleBombaResiduos(){
    if (manualORauto == 1){
        statusPumps[5] = extractValues().toInt();
        digitalWrite(BOMBAACIDA, statusPumps[5]);
        Serial.println("Valor de la bomba residuos: " + String(statusPumps[5]));
        server.send(200, "application/json", "{\"mensaje\":\"Status bomba residuos actualizada\"}");
    } else {
        server.send(400, "application/json", "{\"mensaje\":\"El modo manual está desactivado.\"}");
    }
}

// Al recibir la petición activa el spray
void handleSpray(){
    Serial.println("Spray activado");
    sprayServo.write(45);
    delay(1000);
    sprayServo.write(0);
    delay(1000);
    server.send(200, "application/json", "{\"mensaje\":\"Spray activado correctamente\"}");
}

// Al recibir la petición cambia el modo de control de manual a automático y viceversa y devuelve el nuevo modo
void handleSelectControlMode(){
    manualORauto = (manualORauto + 1) % 2;
    Serial.println("Modo de control de las bombas: " + String(manualORauto));
    server.send(200, "application/json", "{\"manualORauto\":\"" + String(manualORauto) + "\"}");
}

// Recibe el tiempo en segundos para el intervalo de activación de la bomba con el formato {value: "valor"}
void handleAjustePumpAutoUse(){
    pumpUseInterval = extractValues().toFloat();
    preferences.begin("PumpInterval", false);
    preferences.putFloat("pumpUseIntervalRange", pumpUseInterval);
    preferences.end();
    Serial.println("Intervalo de activación de las bombas: " + String(pumpUseInterval));
    server.send(200, "application/json", "{\"mensaje\":\"Ajuste de intervalo de activación de las bombas actualizado\"}");
}

// Recibe el tiempo en segundos para el intervalo de activación del spray con el formato {value: "valor"}
void handleAjusteSprayUse(){
    sprayUseInterval = extractValues().toInt();
    preferences.begin("SprayInterval", false);
    preferences.putFloat("sprayUseIntervalRange", sprayUseInterval);
    preferences.end();
    digitalWrite(BOMBAMEZCLA, 0);
    checkDegreesSpray(lastPos, 45);
    delay(1000);
    checkDegreesSpray(lastPos, 0);
    delay(1000);
    digitalWrite(BOMBAMEZCLA, statusPumps[2]);
    lastSpray = getTime();
    Serial.println("Intervalo de activación del spray: " + String(sprayUseInterval));
    server.send(200, "application/json", "{\"mensaje\":\"Ajuste de intervalo de activación del spray actualizado\"}");
}

// Recibe el mínimo valor óptimo de luz con el formato {value: "valor"}
void handleAjusteLum(){
    String range = extractValues();
    int separatorIndex = range.indexOf('-');
    if (separatorIndex != -1){ 
        LumOptMin = range.substring(0, separatorIndex).toFloat();
        LumOptMax = range.substring(separatorIndex + 1).toFloat();
    } else{ // Se ponen los valores por defecto si llega "auto" que es la otra posibilidad
        LumOptMin = 10000.0;
        LumOptMax = 20000.0;
    }
    preferences.begin("LUMMin", false);
    preferences.putFloat("lumMinData", LumOptMin);
    preferences.end();
    preferences.begin("LUMMax", false);
    preferences.putFloat("lumMaxData", LumOptMax);
    preferences.end();
    Serial.println("Rango óptimo de luz: " + String(LumOptMin) + " - " + String(LumOptMax));
    setFuzzyLUM();
    server.send(200, "application/json", "{\"mensaje\":\"Ajuste de luminosidad actualizado\"}");
}

// Recibe el mínimo y máximo número de horas de luz necesarias con el formato {value: "min-max"}
void handleAjusteLumHours(){
    String range = extractValues();
    int separatorIndex = range.indexOf('-');

    if (separatorIndex != -1){ // Se envía un rango 
        LumOptHrsMin = range.substring(0, separatorIndex).toFloat();
        LumOptHrsMax = range.substring(separatorIndex + 1).toFloat();
    } else{ // Se ponen los valores por defecto si llega "auto" que es la otra posibilidad
        LumOptHrsMin = 14.0;
        LumOptHrsMax = 16.0;
    }
    preferences.begin("LUMHRSMin", false);
    preferences.putFloat("lumHrsMinData", LumOptHrsMin);
    preferences.end();
    preferences.begin("LUMHRSMax", false);
    preferences.putFloat("lumHrsMaxData", LumOptHrsMax);
    preferences.end();
    Serial.println("Rango óptimo de horas de luz: " + String(LumOptHrsMin) + " - " + String(LumOptHrsMax));
    server.send(200, "application/json", "{\"mensaje\":\"Ajuste de rango de horas de luz actualizado\"}");
}

void handleAjustePh(){
    String ranges = extractValues();
    int rangesSeparatorIndex = ranges.indexOf(';');
    String rangesPh = ranges.substring(0, rangesSeparatorIndex);
    String rangesTDS = ranges.substring(separatorIndex + 1);
    int phSeparatorIndex = rangesPh.indexOf('-');
    int tdsSeparatorIndex = rangesTDS.indexOf('-');

    if (phSeparatorIndex != -1){
        pHOptMin = rangesPh.substring(0, phSeparatorIndex)
        if(pHOptMin != "auto"){ // Si no es "auto" es un número float 
            pHOptMin = pHOptMin.toFloat();
        } else{
            pHOptMin = 5.5;
        }
        pHOptMax = rangesPh.substring(phSeparatorIndex + 1);
        if(pHOptMax != "auto"){ // Si no es "auto" es un número float 
            pHOptMax = pHOptMax.toFloat();
        } else{
            pHOptMax = 6.5;
        }
    }
    preferences.begin("PHMin", false);
    preferences.putFloat("phMinData", pHOptMin);
    preferences.end();
    preferences.begin("PHMax", false);
    preferences.putFloat("phMaxData", pHOptMax);
    preferences.end();

    if (tdsSeparatorIndex != -1){
        TDSOptMin = rangesTDS.substring(0, tdsSeparatorIndex)
        if(TDSOptMin != "auto"){ // Si no es "auto" es un número float 
            TDSOptMin = TDSOptMin.toFloat();
        } else{
            TDSOptMin = 800.0;
        }
        TDSOptMax = rangesTDS.substring(tdsSeparatorIndex + 1);
        if(TDSOptMax != "auto"){ // Si no es "auto" es un número float 
            TDSOptMax = TDSOptMax.toFloat();
        } else{
            TDSOptMax = 1200.0;
        }
    }

    preferences.begin("TDSMin", false);
    preferences.putFloat("TDSMinData", TDSOptMin);
    preferences.end();
    preferences.begin("TDSMax", false);
    preferences.putFloat("TDSMaxData", TDSOptMax);
    preferences.end();
    setFuzzyPHTDS();    
    Serial.println("Rango óptimo del PH: " + String(pHOptMin) + " - " + String(pHOptMax) + "\n"
                   "Rango óptimo del TDS: " + String(TDSOptMin) + " - " + String(TDSOptMax));
    server.send(200, "application/json", "{\"mensaje\":\"Ajuste de pH y TDS actualizado\"}");
}

void handleAjusteTemp(){
    String range = extractValues();
    int separatorIndex = range.indexOf('-');

    if(separatorIndex != -1){
        TempOptMin = range.substring(0, separatorIndex).toFloat();
        TempOptMax = range.substring(separatorIndex + 1).toFloat();
    } else {
        TempOptMin = 60.0;
        TempOptMax = 75.0;
    }
    preferences.begin("TEMPMin", false);
    preferences.putFloat("tempMinData", TempOptMin);
    preferences.end();
    preferences.begin("TEMPMax", false);
    preferences.putFloat("tempMaxData", TempOptMax);
    preferences.end();
    Serial.println("Rango óptimo de la temperatura: " + String(TempOptMin) + " - " + String(TempOptMax));
    server.send(200, "application/json", "{\"mensaje\":\"Ajuste de temperatura actualizado\"}");
}

void handleAjusteHum(){
    String range = extractValues();
    int separatorIndex = range.indexOf('-');
    
    if(separatorIndex != -1){
        HumOptMin = range.substring(0, separatorIndex).toFloat();
        HumOptMax = range.substring(separatorIndex + 1).toFloat();
    } else {
        HumOptMin = 14.0;
        HumOptMax = 18.0;
    }
    preferences.begin("HUMMin", false);
    preferences.putFloat("humMinData", HumOptMin);
    preferences.end();
    preferences.begin("HUMMax", false);
    preferences.putFloat("humMaxData", HumOptMax);
    preferences.end();
    setFuzzyHUM();
    Serial.println("Rango óptimo de la temperatura: " + String(HumOptMin) + " - " + String(HumOptMax));
    server.send(200, "application/json", "{\"mensaje\":\"Ajuste de humedad actualizado\"}");
}

void handleHorasLuz(){
    server.send(200, "application/json", "{\"horasLuz\":" + String(lightHours) + "}");
}


void handleTiempoUltimoSpray(){
    server.send(200, "application/json", "{\"actualTime\":" + String(getTime()) + 
                                        ", \"lastSpray\":" + String(lastSpray) +
                                        ", \"sprayUseInterval\":" + String(pumpUseInterval) + "}");
}

void handleTiempoActBombaAuto(){
    if (statusPumps[2] == 0){
        server.send(200, "application/json", "{\"actualTime\":" + String(getTime()) + 
                                            ", \"counterMezcla\":" + String(counterMezcla) + 
                                            ", \"pumpUseInterval\":" + String(pumpUseInterval) + "}");
    } else{
        server.send(200, "application/json", "{\"actualTime\":" + String(lastChronoOn) + 
                                            ", \"counterMezcla\":" + String(counterMezcla) + 
                                            ", \"pumpUseInterval\":" + String(pumpUseInterval) + "}");
    }

}

// SETUP inicial del sistema
void setup()
{
    Serial.begin(9600);

    // Conectamos el servo al PIN y ajustamos su posicion inicial a 0 y ultima a 45
    sprayServo.attach(SPRAY);
    lastPos = 45;
    checkDegreesSpray(lastPos, 0);

    // Establecemos los PINS que correspondan
    pinMode(WATERQUALITYSENSORPIN, INPUT_PULLDOWN);
    pinMode(PHSENSORPIN, INPUT_PULLDOWN);
    pinMode(WATERLVLSENSORPIN, INPUT_PULLDOWN);
    pinMode(WATERLVLSENSORMEZMAX, INPUT_PULLDOWN);
    pinMode(WATERLVLSENSORMEZMIN, INPUT_PULLDOWN);
    pinMode(WATERLVLSENSORRESMAX, INPUT_PULLDOWN);
    pinMode(WATERLVLSENSORPIN, INPUT_PULLDOWN);
    pinMode(HUMIDITYSENSOR, INPUT);

    // Conectamos las bombas a sus PINES correspondientes y la apagamos inicialmente
    pinMode(BOMBAACIDA, OUTPUT);
    digitalWrite(BOMBAACIDA, 0);
    pinMode(BOMBAALCALINA, OUTPUT);
    digitalWrite(BOMBAALCALINA, 0);
    pinMode(BOMBANUTRIENTES, OUTPUT);
    digitalWrite(BOMBANUTRIENTES, 0);
    pinMode(BOMBAMEZCLA, OUTPUT);
    digitalWrite(BOMBAMEZCLA, 0);
    pinMode(BOMBAMEZCLARES, OUTPUT);
    digitalWrite(BOMBAMEZCLARES, 0);
    pinMode(BOMBARES, OUTPUT);
    digitalWrite(BOMBARES, 0);

    // Conectamos el calentador y el ventilador a sus PINES correspondientes y los apagamos
    pinMode(FAN, OUTPUT);
    pinMode(HEATER, OUTPUT);
    analogWrite(FAN, 0);    // OFF
    analogWrite(HEATER, 0); // OFF

    // Establecemos como automatico el control de las bombas al inicio
    pinMode(LED, OUTPUT);
    ledcSetup(0, 1000, 8);
    ledcAttachPin(LED, 0);
    ledcWrite(0, 0); // OFF
    pinMode(RESISTOR, INPUT);
    Wire.begin(SDA_PIN, SCL_PIN);
    manualORauto = 0;
    lastChronoOn = getTime();
    delay(1000);

    // Inicializamos la conexión WIFI
    WiFi.begin(WIFI_SSID, WIFI_PSWD);
    Serial.print("Conectando a WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConectado a WiFi!");

    // Enviamos el identificador y la IP al servidor
    sendIP();

    // Configuramos las rutas del servidor web local para recibir los inputs del usuario
    // Checkboxes de las bombas
    server.on("/bombaAcida", HTTP_POST, handleBombaAcida);
    server.on("/bombaAlcalina", HTTP_POST, handleBombaAlcalina);
    server.on("/bombaMezcla", HTTP_POST, handleBombaMezcla);
    server.on("/bombaNutrientes", HTTP_POST, handleBombaNutrientes);
    server.on("/bombaMezclaResiduos", HTTP_POST, handleBombaMezclaResiduos);
    server.on("/bombaResiduos", HTTP_POST, handleBombaMezclaResiduos);
    // Botón activar spray manualmente
    server.on("/spray", HTTP_POST, handleSpray);
    // Botón para poner el control de las bombas en manual o automático
    server.on("/selectControlMode", HTTP_POST, handleSelectControlMode);
    // Input intervalo de activación de las bombas en modo automático
    server.on("/ajustePumpAutoUse", HTTP_POST, handleAjustePumpAutoUse);
    // Input intervalo de activación del spray en modo automático
    server.on("/ajusteSprayUse", HTTP_POST, handleAjusteSprayUse);
    // Inputs de valores y rangos optimos de los sensores 
    server.on("/ajusteLum", HTTP_POST, handleAjusteLum);
    server.on("/ajusteLumHours", HTTP_POST, handleAjusteLumHours);
    server.on("/ajustePh", HTTP_POST, handleAjustePh);
    server.on("/ajusteTemp", HTTP_POST, handleAjusteTemp);
    server.on("/ajusteHum", HTTP_POST, handleAjusteHum);
    // Peticiones para actualización en tiempo real
    server.on("/horasLuz", HTTP_GET, handleHorasLuz);
    server.on("/tiempoActBombaAuto", HTTP_GET, handleTiempoActBombaAuto);
    server.on("/tiempoUltimoSpray", HTTP_GET, handleTiempoUltimoSpray);
       
    // Inicializamos el servidor
    server.begin();
    Serial.println("Servidor web inicializado");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    // Iniciamos el control del sensor de temperatura y humedad
    dht.begin();
    tsl.begin();
    tsl.enableAutoRange(true);                             // Habilita el rango automatico
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_100MS); // Configura el tiempo de integracion

    // Obtenemos el tiempo actual
    startLight = getTime();

    // Adquirimos los valores de rangos , intervalos y cronos guardados previamente en memoria flash
    preferences.end();
    preferences.begin("PumpInterval", false);
    pumpUseInterval = preferences.getFloat("pumpUseIntervalRange", 3600);
    preferences.end();
    preferences.begin("startDayData", false);
    startDay = preferences.getFloat("startDayData", getActualDay());
    if (isnan(startDay))
    {
        startDay = getActualDay();
    }
    preferences.end();
    preferences.begin("lHrsData", false);
    lightHours = preferences.getULong("lHrsData", 0);
    if (isnan(lightHours) || lightHours > 24 * 3600)
    {
        lightHours = 0;
    }
    preferences.end();
    preferences.begin("SprayInterval", false);
    sprayUseInterval = preferences.getFloat("sprayUseIntervalRange", 60);
    preferences.end();
    preferences.begin("PHMin", false);
    pHOptMin = preferences.getFloat("phMinData", 5.5);
    preferences.end();
    preferences.begin("PHMax", false);
    pHOptMax = preferences.getFloat("phMaxData", 6.5);
    preferences.end();
    preferences.begin("TDSMin", false);
    TDSOptMin = preferences.getFloat("TDSMinData", 800.0);
    preferences.end();
    preferences.begin("TDSMax", false);
    TDSOptMax = preferences.getFloat("TDSMaxData", 1200.0);
    preferences.end();
    preferences.begin("HUMMin", false);
    HumOptMin = preferences.getFloat("humMinData", 60);
    preferences.end();
    preferences.begin("HUMMax", false);
    HumOptMax = preferences.getFloat("humMaxData", 75);
    preferences.end();
    preferences.begin("TEMPMin", false);
    TempOptMin = preferences.getFloat("tempMinData", 14);
    preferences.end();
    preferences.begin("TEMPMax", false);
    TempOptMax = preferences.getFloat("tempMaxData", 18);
    preferences.end();
    preferences.begin("LUMMin", false);
    LumOptMin = preferences.getFloat("lumMinData", 10000);
    preferences.end();
    preferences.begin("LUMMax", false);
    LumOptMax = preferences.getFloat("lumMaxData", 20000);
    preferences.end();
    preferences.begin("LUMHRSMin", false);
    LumOptHrsMin = preferences.getFloat("lumHrsMinData", 14);
    preferences.end();
    preferences.begin("LUMHRSMax", false);
    LumOptHrsMax = preferences.getFloat("lumHrsMaxData", 16);
    preferences.end();

    // Creamos los primeros sistemas difusos
    setFuzzyPHTDS();
    setFuzzyHUM();
    setFuzzyLUM();
    setFuzzyTEMP();

    // Realizamos una pulverizacion inicial
    checkSprayUse(0);
}

//----------------
//LOOP del sistema
// ---------------


bool areAllFloats(float arr[], int size) {
    for (int i = 0; i < size; i++) {
        if (isnan(arr[i]) || isinf(arr[i])) {  // Si el valor es NaN o infinito
            return false;  // No es un float válido
        }
    }
    return true;  // Todos los elementos son floats válidos
}


void loop()
{
    server.handleClient();  

    // Lectura de los niveles de agua en el sistema
    leerSensorMinPlan();            // dataSensorsInfo[5]   |
    leerSensorMaxPlan();            // dataSensorsInfo[10]  |
    leerSensorMinMez();             // dataSensorsInfo[7]   |-- Water lvl
    leerSensorMaxMez();             // dataSensorsInfo[8]   |
    leerSensorRes();                // dataSensorsInfo[9]   |
    // Lecturas de luz
    lightResistorReading();         // dataSensorsInfo[4] (LIGHT RESISTOR)
    lightSensorReading();           // dataSensorsInfo[6] (LIGHT)
    // Lecturas de temperatura
    tempReading();                  // dataSensorsInfo[2] (TEMP)
    // Lecturas de humedad
    humidityReading();              // dataSensorsInfo[3] (HUM)
    // Lecturas de ph y waterQuality junto con el control de bombas
    controlLiquidos();              // dataSensorsInfo[0] (PH) y dataSensorsInfo[1] (TDS)

    sendData();

    // Lectura y evaluación de los sistemas de lógica difusa
    checkTEMP();
    checkHUM();
    checkLUM();
    ifDayChanged();
    
    delay(5000);
}