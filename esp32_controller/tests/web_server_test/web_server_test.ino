#include <WiFi.h>
#include <HTTPClient.h> 
#include <WebServer.h> 
#include <ArduinoJson.h>

// Identificador del microcontrolador
#define CONTROLLER_ID "1"


// Configuración de conexión WiFi
#define WIFI_SSID ""
#define WIFI_PSWD ""

// Variable para guardar la IP que se le asignará al microcontrolador al conectarse a la red wifi
String ip_value;

// URL del servidor principal
const char* serverUrl = "http://192.168.73.200:3000";

// Creamos un pequeño servidor web local en el puerto 80
WebServer server(80);

// Función que se ejecuta en el setup para enviar el ID y la IP asignada al microcontrolador al servidor principal
void sendIP(){
    if(WiFi.status() == WL_CONNECTED) {
        HTTPClient http; // Crear el objeto HTTP
        String fullUrl = String(serverUrl) + "/ipAssignment";
        http.begin(fullUrl); // Establecer conexión
        http.addHeader("Content-Type", "application/json"); // Tipo de contenido JSON
        ip_value = WiFi.localIP().toString();
        String jsonData = "{";
        jsonData += "\"" + String(CONTROLLER_ID) + "\": \"" + String(ip_value) + "\"";
        jsonData += "}";
        Serial.println(jsonData);
        int httpResponseCode = http.POST(jsonData); // Enviar la petición POST

        if (httpResponseCode > 0) {
            Serial.print("Respuesta del servidor: ");
            Serial.println(http.getString()); // Mostrar la respuesta del servidor
        } else {
            Serial.print("Error en la petición: sendIP");
            Serial.println(httpResponseCode);
        }

        http.end(); // Finalizar conexión
    } else {
        Serial.println("Error: No conectado a WiFi");
    }
}

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

void handlePetition() {
    String route = server.uri();
    String range = extractValues();

    Serial.println("Petición recibida: " + route + "\t valor_recibido: " + range);
    
    String response = "{\"mensaje\":\"Petición recibida correctamente\"}";
    server.send(200, "application/json", response);
}

// SETUP inicial del sistema
void setup()
{
    Serial.begin(9600);

    // Inicializamos la conexión WIFI
    unsigned long startAttemptTime = millis();
    WiFi.begin(WIFI_SSID, WIFI_PSWD);
    Serial.print("Conectando a WiFi...");
    // Intentamos conectar al WiFi durante 20 segundos, si no se conecta, se detiene el proceso
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000) {
        delay(500);
        Serial.print(".");
    }
    Serial.print("IP asignada al ESP32: ");
    Serial.println(WiFi.localIP());
    Serial.println("\nConectado a WiFi!");

    // Enviamos el identificador y la IP al servidor
    sendIP();

    // Configuramos las rutas del servidor web local para recibir los inputs del usuario
    // Checkboxes de las bombas
    server.on("/bombaAcida", HTTP_POST, handlePetition);
    server.on("/bombaAlcalina", HTTP_POST, handlePetition);
    server.on("/bombaMezcla", HTTP_POST, handlePetition);
    server.on("/bombaNutrientes", HTTP_POST, handlePetition);
    server.on("/bombaMezclaResiduos", HTTP_POST, handlePetition);
    server.on("/bombaResiduos", HTTP_POST, handlePetition);
    // Botón activar spray manualmente
    server.on("/spray", HTTP_POST, handlePetition);
    // Botón para poner el control de las bombas en manual o automático
    server.on("/selectControlMode", HTTP_POST, handlePetition);
    // Input intervalo de activación de las bombas en modo automático
    server.on("/ajustePumpAutoUse", HTTP_POST, handlePetition);
    // Input intervalo de activación del spray en modo automático
    server.on("/ajusteSprayUse", HTTP_POST, handlePetition);
    // Inputs de valores y rangos optimos de los sensores 
    server.on("/ajusteLum", HTTP_POST, handlePetition);
    server.on("/ajusteLumHours", HTTP_POST, handlePetition);
    server.on("/ajustePh", HTTP_POST, handlePetition);
    server.on("/ajusteTemp", HTTP_POST, handlePetition);
    server.on("/ajusteHum", HTTP_POST, handlePetition);
    // Inputs de activación de los actuadores (led, ventilador y calentador)
    server.on("/modeLed", HTTP_POST, handlePetition);
    server.on("/modeFan", HTTP_POST, handlePetition);
    server.on("/modeHeater", HTTP_POST, handlePetition);
    server.on("/powerLed", HTTP_POST, handlePetition);
    server.on("/powerFan", HTTP_POST, handlePetition);
    server.on("/powerHeater", HTTP_POST, handlePetition);
    // Peticiones para actualización en tiempo real
    server.on("/horasLuz", HTTP_GET, handlePetition);
    server.on("/tiempoActBombaAuto", HTTP_GET, handlePetition);
    server.on("/tiempoUltimoSpray", HTTP_GET, handlePetition);
       
    // Inicializamos el servidor
    server.begin();
    Serial.println("Servidor web inicializado");
}

void loop()
{
    server.handleClient();  
    delay(1000);
}
