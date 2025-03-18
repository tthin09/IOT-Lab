#include <arduino.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>
#include "WiFi.h"
#include "freertos/FreeRTOS.h"
#include "DHT20.h"
#include <LCDI2C_Multilingual.h>
#include "project_config.h"

#define LED_A 32
#define LED_B 33

#define I2C_SDA 21
#define I2C_SCL 22

// Wifi setup
constexpr char WIFI_SSID[] = WIFI_SSID_VALUE;
constexpr char WIFI_PASSWORD[] = WIFI_PASSWORD_VALUE;

// Thingsboard setup
constexpr char TOKEN[] = THINGSBOARD_TOKEN_VALUE;
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;

constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

// Objects
WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

// LCD setup
LCDI2C_Vietnamese lcd(0x21, 16, 2);

void connectWifi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    vTaskDelay(500);
  }
  Serial.print("\nConnected to WiFi, local IP: ");
  Serial.println(WiFi.localIP());
}

void wifiTask(void *pvParameters)
{
  connectWifi();
  while (1)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Wifi disconnected, reconnecting...");
      WiFi.disconnect();
      connectWifi();
    }

    vTaskDelay(5000);
  }
}

void trafficLightTask(void *pvParameters)
{
  const char *light_colors[3] = {"Green", "Yellow", "Red"};
  int lights[3][2] = {{1, 0}, {0, 1}, {1, 1}};
  int light_time[3] = {4, 2, 3};

  while (1)
  {
    for (int i = 0; i < 3; i++)
    {
      Serial.print(light_colors[i]);
      Serial.println(" light on");
      digitalWrite(LED_A, lights[i][0]);
      digitalWrite(LED_B, lights[i][1]);

      vTaskDelay(light_time[i] * 1000);
    }
  }
}

void dht20Task(void *pvParameters)
{
  DHT20 dht;
  dht.begin();

  while (1)
  {
    vTaskDelay(telemetrySendInterval);

    int status = dht.read();
    if (status != DHT20_OK)
    {
      Serial.println("Failed to read data from DHT20!");
    }

    float temperature = dht.getTemperature();
    float humidity = dht.getHumidity();
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, ");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    tb.sendTelemetryData("temperature", temperature);
    tb.sendTelemetryData("humidity", humidity);

    lcd.clear();
    lcd.println("Nhiệt độ: " + String((int)temperature) + " °C");
    lcd.println("Độ ẩm: " + String(humidity, 1) + " %");
  }
}

void connectThingsboardTask(void *pvParameters)
{
  while (1)
  {
    vTaskDelay(5000);
    if (!tb.connected())
    {
      Serial.print("Connecting to: ");
      Serial.print(THINGSBOARD_SERVER);
      Serial.print(" with token ");
      Serial.println(TOKEN);
      const bool successful = tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
      if (!successful)
      {
        Serial.println("Failed to connect to ThingsBoard");
        continue;
      }

      tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
      tb.sendAttributeData("rssi", WiFi.RSSI());
      tb.sendAttributeData("channel", WiFi.channel());
      tb.sendAttributeData("bssid", WiFi.BSSIDstr().c_str());
      tb.sendAttributeData("localIp", WiFi.localIP().toString().c_str());
      tb.sendAttributeData("ssid", WiFi.SSID().c_str());
      Serial.println("Subscribe to ThingsBoard done");
    }
  }
}

void setup()
{
  Serial.begin(SERIAL_DEBUG_BAUD);
  Wire.begin(I2C_SDA, I2C_SCL);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.println("Hello!");

  // xTaskCreate(wifiTask, "Wifi Task", 2048, NULL, 2, NULL);
  // xTaskCreate(connectThingsboardTask, "Connect ThingsBoard Task", 8192, NULL, 2, NULL);
  xTaskCreate(dht20Task, "DHT20 Task", 2048, NULL, 1, NULL);
  // xTaskCreate(trafficLightTask, "Traffic Light Task", 2048, NULL, 1, NULL);
}

void loop()
{
  // FreeRTOS
}