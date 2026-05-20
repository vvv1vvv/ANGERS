#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>

const int SERVO_PIN = 18;
const int BUTTON_PIN = 4;
const int GREEN_LED = 12;
const int RED_LED = 13;
const int BUZZER_PIN = 23;

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;

const char* statusTopic = "iut/smartaccess/status";
const char* commandTopic = "iut/smartaccess/cmd";

Servo gate;
LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  gate.setPeriodHertz(50);
  gate.attach(SERVO_PIN, 500, 2400);
  gate.write(0);

  lcd.init();
  lcd.backlight();

  showLocked();

  connectWifi();

  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);

  connectMqtt();
}

void loop() {
  if (!client.connected()) {
    connectMqtt();
  }

  client.loop();

  if (digitalRead(BUTTON_PIN) == LOW) {
    openGate("LOCAL BUTTON");

    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10);
    }

    delay(200);
  }
}

void connectWifi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi connected");
  delay(1000);
}

void connectMqtt() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    String clientId = "ESP32SmartAccess-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("MQTT connected");

      client.subscribe(commandTopic);
      client.publish(statusTopic, "ONLINE");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MQTT connected");
      delay(1000);

      showLocked();
    } else {
      Serial.print("Failed, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  message.trim();
  message.toUpperCase();

  Serial.print("MQTT received on ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  if (message == "OPEN") {
    openGate("MQTT COMMAND");
  }
}

void openGate(String source) {
  showAccessGranted(source);

  client.publish(statusTopic, "ACCESS_GRANTED");
  client.publish(statusTopic, source.c_str());

  tone(BUZZER_PIN, 1000, 150);

  gate.write(90);
  client.publish(statusTopic, "GATE_OPEN");

  delay(2000);

  gate.write(0);
  client.publish(statusTopic, "LOCKED");

  showLocked();
}

void showLocked() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Access");
  lcd.setCursor(0, 1);
  lcd.print("Status: LOCKED");

  Serial.println("Status: LOCKED");
}

void showAccessGranted(String source) {
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Access granted");
  lcd.setCursor(0, 1);
  lcd.print(source);

  Serial.print("Access granted by: ");
  Serial.println(source);
}