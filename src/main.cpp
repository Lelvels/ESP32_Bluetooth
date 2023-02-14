#include <Arduino.h>
#include "BluetoothSerial.h"
#include "ArduinoJson.h"
#include "WiFi.h"
#include "iot_configs.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
  #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define LED 2
BluetoothSerial SerialBT;

static String message = "";
char incomingChar;
bool receiveStringEnd = false;
bool ledState = LOW;
DynamicJsonDocument doc(200);     

static void connectToWiFi(String ssid, String password)
{
  WiFi.disconnect();
  Serial.println();
  Serial.print("Connecting to WIFI SSID ");
  Serial.println(ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  long previousTime = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    long currentTime = millis();
    if(currentTime - previousTime >= CONNECT_WIFI_TIMEOUT)
      break;
    delay(500);
    Serial.print(".");
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.print("WiFi connected, IP address: ");
    Serial.println(WiFi.localIP());
    SerialBT.print("WiFi connected, IP address: ");
    SerialBT.println(WiFi.localIP());
  } else {
    Serial.println("Wifi not connected!");
    SerialBT.println("Wifi not connected!");
  }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32-AQUA2");
  Serial.println("The device started, now you can pair it with bluetooth!");
  pinMode(LED, OUTPUT);
}

void loop() {
  if (SerialBT.available()){
    char incomingChar = SerialBT.read();
    if (incomingChar != '\n'){
      message += String(incomingChar);
    } else {
      receiveStringEnd = true;
    }
    Serial.write(incomingChar);  
  }
  if(!message.isEmpty() && receiveStringEnd){
    Serial.print("[+] Processing string: ");
    Serial.print(message.c_str());
    deserializeJson(doc, message);
    JsonObject object = doc.as<JsonObject>();
    bool hasMethod = object.containsKey("method");
    if(hasMethod){
      String method = object["method"];
      if(method.compareTo("ToggleLed") == 0)
      {
        if(ledState == HIGH)
          ledState = LOW;
        else
          ledState = HIGH;
        digitalWrite(LED, ledState);
        SerialBT.print("[+] Toggle LED to ");
        SerialBT.println(ledState);
      }
      else if(method.compareTo("ConnectToWifi") == 0)
      {
        if(object.containsKey("SSID") && object.containsKey("Password")){
          String ssid = object["SSID"];
          String password = object["Password"];
          connectToWiFi(ssid, password);
        } else {
          SerialBT.println("[+] Wifi not connected, Missing SSID or Password!");
        }
      }
      else 
      {
        SerialBT.println("[+] Wifi not connected, Invalid method name!");
      }
    } else {
      SerialBT.println("[+] Wifi not connected, Invalid string!");
    }
    receiveStringEnd = false;
    message.clear();
  }
  delay(20);
}
