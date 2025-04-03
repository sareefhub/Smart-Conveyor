#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

// กำหนดขาพินต่างๆ
#define GREEN_SERVO_PIN 13  
#define RED_SERVO_PIN 5     
#define PUSH_GREEN_SERVO_PIN 23
#define PUSH_RED_SERVO_PIN 19
#define IR_GREEN_PIN 26
#define IR_RED_PIN 25

Servo greenServo, redServo, pushGreenServo, pushRedServo;

int lastIrGreen = -1;
int lastIrRed = -1;
unsigned long lastDebounceTimeGreen = 0;
unsigned long lastDebounceTimeRed = 0;
const unsigned long debounceDelay = 100;

// ตั้งค่าการเชื่อมต่อ Wi-Fi และ MQTT
const char* ssid = "Yindee";
const char* password = "phoorin3799";
const char* mqtt_server = "172.20.10.3";  // IP ของ MQTT Broker
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/color";  // Topic สำหรับรับคำสั่ง

WiFiClient espClient;
PubSubClient client(espClient);

bool connectedOnce = false;  // ตัวแปรเพื่อตรวจสอบการเชื่อมต่อครั้งแรก
bool wifiConnectedOnce = false;  // ตัวแปรเพื่อตรวจสอบการเชื่อมต่อ Wi-Fi ครั้งแรก

void setup() {
    Serial.begin(115200);
    greenServo.attach(GREEN_SERVO_PIN);
    redServo.attach(RED_SERVO_PIN);
    pushGreenServo.attach(PUSH_GREEN_SERVO_PIN);
    pushRedServo.attach(PUSH_RED_SERVO_PIN);

    pinMode(IR_GREEN_PIN, INPUT_PULLUP);
    pinMode(IR_RED_PIN, INPUT_PULLUP);    

    resetServos();

    // เชื่อมต่อ Wi-Fi
    setup_wifi();

    // เชื่อมต่อกับ MQTT Broker
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void resetServos() {
    greenServo.write(120);
    redServo.write(90);
    pushGreenServo.write(90);
    pushRedServo.write(90);
}

void rotatePushGreenServo() {
    pushGreenServo.write(30);
    delay(3800);
    pushGreenServo.write(90);
    delay(1000);
    pushGreenServo.write(150);
    delay(4000);
    pushGreenServo.write(90);
    delay(1000);
}

void rotatePushRedServo() {
    pushRedServo.write(30);
    delay(3800);
    pushRedServo.write(90);
    delay(1000);
    pushRedServo.write(150);
    delay(4000);
    pushRedServo.write(90);
    delay(1000);
}

void setup_wifi() {
    if (!wifiConnectedOnce) {
        Serial.println("Connecting to WiFi...");
    }
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if (!wifiConnectedOnce) {
            Serial.print(".");
        }
    }
    if (!wifiConnectedOnce) {
        Serial.println("\nWiFi connected.");
        wifiConnectedOnce = true;
    }
}

void reconnect() {
    while (!client.connected()) {
        if (!connectedOnce) {
            Serial.println("Attempting MQTT connection...");
        }
        if (client.connect("ESP32ClientColor")) {
            Serial.println("Connected to MQTT Broker.");
            client.subscribe(mqtt_topic);
            // ส่งข้อความไปที่ MQTT Broker เฉพาะครั้งแรก
            if (!connectedOnce) {
                client.publish("esp32/status", "Connected to MQTT Broker.", 1);
                connectedOnce = true;  // ตั้งค่าเป็น true เพื่อไม่ให้ส่งซ้ำ
            }
        } else {
            if (!connectedOnce) {
                Serial.print("Failed, rc=");
                Serial.print(client.state());
                Serial.println(" Retrying in 5 seconds...");
            }
            delay(5000);
        }
    }
}

// ฟังก์ชันรับคำสั่งจาก MQTT
void callback(char* topic, byte* message, unsigned int length) {
    String color = "";
    for (int i = 0; i < length; i++) {
        color += (char)message[i];
    }

    Serial.println("Received color: " + color);

    if (color == "Green") {
        Serial.println("Green Servo Activated");
        greenServo.write(25);
        delay(500);

        Serial.println("Waiting for IR Green detection...");
        while (digitalRead(IR_GREEN_PIN) != 0);

        Serial.println("IR Green Detected. Pushing...");
        rotatePushGreenServo();

        resetServos();
        Serial.println("Green Process Completed");
        client.publish("esp32/status", "Green Process Completed");
    }
    else if (color == "Red") {
        Serial.println("Red Servo Activated");
        redServo.write(0);
        delay(500);

        Serial.println("Waiting for IR Red detection...");
        while (digitalRead(IR_RED_PIN) != 0);

        Serial.println("IR Red Detected. Pushing...");
        rotatePushRedServo();

        resetServos();
        Serial.println("Red Process Completed");
        client.publish("esp32/status", "Red Process Completed");
    }
}

// ฟังก์ชันหลัก
void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    int irGreen = digitalRead(IR_GREEN_PIN);
    int irRed = digitalRead(IR_RED_PIN);

    if (irGreen != lastIrGreen && millis() - lastDebounceTimeGreen > debounceDelay) {
        lastIrGreen = irGreen;
        lastDebounceTimeGreen = millis();
    }

    if (irRed != lastIrRed && millis() - lastDebounceTimeRed > debounceDelay) {
        lastIrRed = irRed;
        lastDebounceTimeRed = millis();
    }

    delay(500);
}
