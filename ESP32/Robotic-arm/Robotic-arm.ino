#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <ESP32Servo.h>

// 🔹 ตั้งค่า WiFi และ MQTT
const char* ssid = "";  
const char* password = "";  
const char* mqtt_server = "172.20.10.3";  // IP ของคอมที่รัน MQTT Broker
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/robot";  // MQTT Topic

WiFiClient espClient;
PubSubClient client(espClient);

// 🔹 กำหนดขาพินของเซอร์โว
#define BASE_SERVO_PIN 25  
#define RIGHT_ARM_SERVO_PIN 17  
#define CLAW_SERVO_PIN 27  

Servo baseServo;
Servo rightArmServo;
Servo clawServo;

// 🔹 กำหนดค่ามุมเริ่มต้น
const int BASE_CENTER = 90;  
const int BASE_RIGHT = 180;  
const int BASE_LEFT = 0;  
const int RIGHT_ARM_UP = 0;  
const int RIGHT_ARM_DOWN = 60;  
const int CLAW_OPEN = 70;  
const int CLAW_GRAB = 160;  

// 🔹 ฟังก์ชันเคลื่อนที่เซอร์โว (เหมือนเดิม)
void moveServo(Servo &servo, int startAngle, int targetAngle, bool isBaseOrClaw, int stepDelay = 15) {
    int step = isBaseOrClaw ? 4 : 2; 
    if (startAngle > targetAngle) step = -step;

    for (int angle = startAngle; (step > 0) ? (angle <= targetAngle) : (angle >= targetAngle); angle += step) {
        servo.write(angle);
        delay(stepDelay);
    }
    servo.write(targetAngle);
}

// 🔹 ฟังก์ชันควบคุมการหมุน (เหมือนเดิม)
void processMovement(int targetBaseAngle) {
    Serial.println("Starting task...");
    moveServo(baseServo, BASE_CENTER, targetBaseAngle, true);
    delay(1000);
    moveServo(rightArmServo, RIGHT_ARM_UP, RIGHT_ARM_DOWN, false);
    delay(1000);
    moveServo(clawServo, CLAW_OPEN, CLAW_GRAB, true);
    delay(1000);
    moveServo(rightArmServo, RIGHT_ARM_DOWN, RIGHT_ARM_UP, false);
    delay(1000);
    moveServo(baseServo, targetBaseAngle, BASE_CENTER, true);
    delay(1000);
    moveServo(rightArmServo, RIGHT_ARM_UP, RIGHT_ARM_DOWN, false);
    delay(1000);
    moveServo(clawServo, CLAW_GRAB, CLAW_OPEN, true);
    delay(1000);
    moveServo(rightArmServo, RIGHT_ARM_DOWN, RIGHT_ARM_UP, false);
    delay(1000);
    Serial.println("Task completed.");
}

// 🔹 ฟังก์ชันรับคำสั่งจาก MQTT
void callback(char* topic, byte* message, unsigned int length) {
    Serial.print("MQTT Message received: ");
    String command;
    
    for (int i = 0; i < length; i++) {
        command += (char)message[i];
    }
    Serial.println(command);

    if (command == "LEFT") {
        Serial.println("Moving Left via MQTT");
        processMovement(BASE_LEFT);
    } else if (command == "RIGHT") {
        Serial.println("Moving Right via MQTT");
        processMovement(BASE_RIGHT);
    }
}

// 🔹 ฟังก์ชันเชื่อมต่อ WiFi
void setup_wifi() {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
}

// 🔹 ฟังก์ชันเชื่อมต่อกับ MQTT Broker
void reconnect() {
    while (!client.connected()) {
        Serial.println("Attempting MQTT connection...");
        if (client.connect("ESP32Client")) {
            Serial.println("Connected to MQTT Broker.");
            client.subscribe(mqtt_topic);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" Retrying in 5 seconds...");
            delay(5000);
        }
    }
}

// 🔹 ตั้งค่า TFT (เหมือนเดิม)
TFT_eSPI tft = TFT_eSPI();
#define BTN_WIDTH  150
#define BTN_HEIGHT 60
#define BTN_RIGHT_X  (tft.width() / 2 - BTN_WIDTH - 10)
#define BTN_RIGHT_Y  (tft.height() / 2 - BTN_HEIGHT / 2)
#define BTN_LEFT_X   (tft.width() / 2 + 10)
#define BTN_LEFT_Y   (tft.height() / 2 - BTN_HEIGHT / 2)

void setup() {
    Serial.begin(115200);
    setup_wifi();
    
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    baseServo.attach(BASE_SERVO_PIN);
    baseServo.write(BASE_CENTER);

    rightArmServo.attach(RIGHT_ARM_SERVO_PIN);
    rightArmServo.write(RIGHT_ARM_UP);

    clawServo.attach(CLAW_SERVO_PIN);
    clawServo.write(CLAW_OPEN);

    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_WHITE);
    drawButtons();
}

// 🔹 วาดปุ่มควบคุม (เหมือนเดิม)
void drawButtons() {
    drawButton(BTN_LEFT_X, BTN_LEFT_Y, "LEFT", TFT_BLUE);
    drawButton(BTN_RIGHT_X, BTN_RIGHT_Y, "RIGHT", TFT_RED);
}

// 🔹 ฟังก์ชันวาดปุ่ม
void drawButton(int x, int y, String label, uint16_t color) {
    tft.fillRect(x, y, BTN_WIDTH, BTN_HEIGHT, color);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(x + 40, y + 20);
    tft.print(label);
}

// 🔹 ฟังก์ชันตรวจสอบการกดปุ่ม
bool isPressed(uint16_t x, uint16_t y, int btnX, int btnY) {
    return (x >= btnX && x <= btnX + BTN_WIDTH && y >= btnY && y <= btnY + BTN_HEIGHT);
}

// 🔹 Loop ทำงานหลัก + MQTT
void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
        if (isPressed(x, y, BTN_LEFT_X, BTN_LEFT_Y)) {
            Serial.println("LEFT Button Pressed");
            drawButton(BTN_LEFT_X, BTN_LEFT_Y, "LEFT", TFT_DARKGREY);
            delay(200);
            drawButton(BTN_LEFT_X, BTN_LEFT_Y, "LEFT", TFT_BLUE);
            processMovement(BASE_LEFT);
        }
        if (isPressed(x, y, BTN_RIGHT_X, BTN_RIGHT_Y)) {
            Serial.println("RIGHT Button Pressed");
            drawButton(BTN_RIGHT_X, BTN_RIGHT_Y, "RIGHT", TFT_DARKGREY);
            delay(200);
            drawButton(BTN_RIGHT_X, BTN_RIGHT_Y, "RIGHT", TFT_RED);
            processMovement(BASE_RIGHT);
        }
    }
}