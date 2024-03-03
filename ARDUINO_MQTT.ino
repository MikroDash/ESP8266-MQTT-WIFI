/*
 Ejemplo del canal de Youtube "MicroTutoriales DC"

 https://www.youtube.com/c/MicroTutorialesDCareaga/videos
 
 Utilizando el broker MQTT de mqtt.mikrodash.com
 y la plataforma de https://app.mikrodash.com
 para crear una dashboard personalizada
*/
#include <PubSubClient.h> // Biblioteca para el cliente MQTT

// Incluye las bibliotecas WiFi para ESP8266 o ESP32
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif

// Definición de pines para ESP8266 y ESP32
#if defined(ESP8266)
  const int buttonPin = 5; // Un pin de entrada digital típico en ESP8266
  const int ledPin = 16; // Un pin de salida digital típico en ESP8266
  const int sensorPin = A0; // Pin analógico en ESP8266
  const int pwmPin = 2; // Un pin de PWM en ESP8266
#elif defined(ESP32)
  const int buttonPin = 5; // Un pin de entrada digital típico en ESP32
  const int ledPin = 22; // Un pin de salida digital típico en ESP32
  const int sensorPin = 34; // Pin analógico en ESP32 (sólo lectura)
  const int pwmPin = 4; // Un pin de PWM en ESP32
#endif

// Variables para el estado del botón y el valor del sensor
bool statusBtn = false;
bool prevButtonState = false; 
int sensorValue = 0;
int percentValue = 0;
int lastPercentValue = 0;

// Tópicos MQTT (debes reemplazar "your_mikrodash_id" con tu ID real de MikroDash)
const char* topic_sensor = "your_mikrodash_id/sensor/value";
const char* topic_button = "your_mikrodash_id/button/value";
const char* topic_pwm = "your_mikrodash_id/pwm/percent";
const char* topic_led = "your_mikrodash_id/led/value";

// Configuración de la red WiFi y MQTT
const char* ssid = "WIFI-MIKRODASH";  // Nombre de tu red WiFi
const char* password = "contra12345"; // Contraseña de tu red WiFi
const char* mqtt_server = "mqtt.mikrodash.com"; // Servidor MQTT
const int mqtt_port = 1883;           // Puerto MQTT
const char* mqtt_user = "tu_usuario_mqtt"; // Usuario para autenticación MQTT
const char* mqtt_password = "tu_contraseña_mqtt"; // Contraseña para autenticación MQTT

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  Serial.println("Conectando a la red WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.println("Dirección IP: " + WiFi.localIP().toString());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String incomingMessage = "";
  for (int i = 0; i < length; i++) {
    incomingMessage += (char)payload[i];
  }

  if (strcmp(topic, topic_led) == 0) {
    digitalWrite(ledPin, incomingMessage.toInt() ? HIGH : LOW);
  } else if (strcmp(topic, topic_pwm) == 0) {
    int pwmValue = map(incomingMessage.toInt(), 0, 100, 0, 255);
    #if defined(ESP8266)
      analogWrite(pwmPin, pwmValue);
    #elif defined(ESP32)
      ledcWrite(0, pwmValue); // Usando canal 0 para ESP32 PWM
    #endif
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect("ESPClient", mqtt_user, mqtt_password)) {
      Serial.println("Conectado");
      client.subscribe(topic_led);
      client.subscribe(topic_pwm);
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  
  #if defined(ESP32)
    // Configuración PWM para ESP32
    ledcAttachPin(pwmPin, 0); // Asigna el pin PWM al canal 0
    ledcSetup(0, 5000, 8); // Configura el canal 0, 5000 Hz, 8 bits
  #endif
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Leer estado del botón
  bool currentStatus = digitalRead(buttonPin);
  if (prevButtonState != currentStatus) {
    prevButtonState = currentStatus;
    client.publish(topic_button, currentStatus ? "1" : "0", true);
  }

  // Leer y publicar valor del sensor
  sensorValue = analogRead(sensorPin);
  percentValue = map(sensorValue, 0, 1023, 0, 100); // Ajustar rango si es ESP32
  if (percentValue != lastPercentValue) {
    lastPercentValue = percentValue;
    String msg = String(percentValue);
    client.publish(topic_sensor, msg.c_str(), true);
  }

  delay(100);
}
