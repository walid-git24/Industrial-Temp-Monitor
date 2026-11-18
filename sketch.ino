#include <WiFi.h>
#include <PubSubClient.h>
#include <dhtESP32-rmt.h>

// ---------- Pin Configuration ----------
#define DHT_PIN     4
#define BUTTON_PIN  27
#define FAN_PIN     26

// ---------- Wi-Fi ----------
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// ---------- MQTT ----------
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char* MQTT_TOPIC =
    "asl/national-arena/challenge05/aryanaritro/temperature";

// ---------- Network Clients ----------
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ---------- Sensor Data ----------
float temperature = 0.0;
float humidity = 0.0;

float latestTemperature = 0.0;
float latestHumidity = 0.0;

volatile bool newReadingAvailable = false;

portMUX_TYPE sensorMux = portMUX_INITIALIZER_UNLOCKED;

// ---------- Timing ----------
const unsigned long SENSOR_INTERVAL = 2000;
const unsigned long DEBOUNCE_TIME = 25;
const unsigned long RECONNECT_INTERVAL = 5000;

unsigned long lastSensorRead = 0;

// ---------- Fan Control ----------
bool fanState = false;

int lastButtonReading = HIGH;
int stableButtonState = HIGH;

unsigned long lastDebounceTime = 0;


// =================================================
// NETWORK TASK
// =================================================

void networkTask(void* parameter) {

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.println("Connecting to Wi-Fi...");

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setSocketTimeout(2);
  mqttClient.setKeepAlive(15);

  unsigned long lastWifiAttempt = millis();
  unsigned long lastMqttAttempt = 0;

  for (;;) {

    unsigned long now = millis();

    // ---------- Wi-Fi Management ----------
    if (WiFi.status() != WL_CONNECTED) {

      if (mqttClient.connected()) {
        mqttClient.disconnect();
      }

      if (now - lastWifiAttempt >= RECONNECT_INTERVAL) {

        Serial.println("Retrying Wi-Fi...");

        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASS);

        lastWifiAttempt = now;
      }
    }

    // ---------- MQTT Management ----------
    else {

      if (!mqttClient.connected()) {

        if (now - lastMqttAttempt >= RECONNECT_INTERVAL) {

          lastMqttAttempt = now;

          char clientID[40];

          snprintf(
            clientID,
            sizeof(clientID),
            "ASL-CH05-%08X",
            (unsigned int)esp_random()
          );

          Serial.println("Connecting to MQTT...");

          if (mqttClient.connect(clientID)) {
            Serial.println("MQTT Connected!");
          } else {
            Serial.print("MQTT Failed. State: ");
            Serial.println(mqttClient.state());
          }
        }
      }

      // ---------- MQTT Publishing ----------
      if (mqttClient.connected()) {

        mqttClient.loop();

        bool available = false;
        float t = 0.0;
        float h = 0.0;

        portENTER_CRITICAL(&sensorMux);

        if (newReadingAvailable) {

          t = latestTemperature;
          h = latestHumidity;

          newReadingAvailable = false;
          available = true;
        }

        portEXIT_CRITICAL(&sensorMux);

        if (available) {

          char payload[100];

          snprintf(
            payload,
            sizeof(payload),
            "{\"temperature\":%.2f,\"humidity\":%.2f}",
            t,
            h
          );

          if (mqttClient.publish(MQTT_TOPIC, payload)) {
            Serial.print("MQTT Published: ");
            Serial.println(payload);
          } else {
            Serial.println("MQTT Publish Failed");
          }
        }
      }
    }

    // Network task yields independently of button control.
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


// =================================================
// SETUP
// =================================================

void setup() {

  Serial.begin(115200);
  delay(500);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(FAN_PIN, OUTPUT);

  digitalWrite(FAN_PIN, LOW);

  Serial.println("\nASL Challenge 05");
  Serial.println("System Initializing...");

  // Network runs independently on Core 0.
  xTaskCreatePinnedToCore(
    networkTask,
    "NetworkTask",
    8192,
    NULL,
    1,
    NULL,
    0
  );

  Serial.println("System Ready");
}


// =================================================
// MAIN LOOP — BUTTON + SENSOR
// =================================================

void loop() {

  unsigned long now = millis();

  // ---------- Emergency Button ----------
  int buttonReading = digitalRead(BUTTON_PIN);

  if (buttonReading != lastButtonReading) {
    lastDebounceTime = now;
  }

  if ((now - lastDebounceTime) >= DEBOUNCE_TIME) {

    if (buttonReading != stableButtonState) {

      stableButtonState = buttonReading;

      // Toggle fan only when button is pressed.
      if (stableButtonState == LOW) {

        fanState = !fanState;

        digitalWrite(
          FAN_PIN,
          fanState ? HIGH : LOW
        );

        Serial.print("Emergency Fan: ");
        Serial.println(fanState ? "ON" : "OFF");
      }
    }
  }

  lastButtonReading = buttonReading;


  // ---------- DHT22 Reading ----------
  if (now - lastSensorRead >= SENSOR_INTERVAL) {

    lastSensorRead = now;

    uint8_t error = read_dht(
      temperature,
      humidity,
      DHT_PIN,
      DHT22
    );

    if (error == 0) {

      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" C");

      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");

      portENTER_CRITICAL(&sensorMux);

      latestTemperature = temperature;
      latestHumidity = humidity;

      newReadingAvailable = true;

      portEXIT_CRITICAL(&sensorMux);

    } else {

      Serial.print("DHT Error Code: ");
      Serial.println(error);
    }
  }

  delay(1);
}
