#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <ESP32Servo.h>

const char* ssid = "";  
const char* password = "";  
const char* mqtt_server = "172.20.10.3";  
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/robot";  

WiFiClient espClient;
PubSubClient client(espClient);

#define BASE_SERVO_PIN 25  
#define RIGHT_ARM_SERVO_PIN 17  
#define CLAW_SERVO_PIN 27  

Servo baseServo;
Servo rightArmServo;
Servo clawServo;

const int BASE_CENTER = 90;  
const int BASE_RIGHT = 180;  
const int BASE_LEFT = 0;  
const int RIGHT_ARM_UP = 0;  
const int RIGHT_ARM_DOWN = 60;  
const int CLAW_OPEN = 70;  
const int CLAW_GRAB = 160;  

void moveServo(Servo &servo, int startAngle, int targetAngle, bool isBaseOrClaw, int stepDelay = 15) {
    int step = isBaseOrClaw ? 4 : 2; 
    if (startAngle > targetAngle) step = -step;

    for (int angle = startAngle; (step > 0) ? (angle <= targetAngle) : (angle >= targetAngle); angle += step) {
        servo.write(angle);
        delay(stepDelay);
    }
    servo.write(targetAngle);
}

void processMovement(int targetBaseAngle) {
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
}

void callback(char* topic, byte* message, unsigned int length) {
    String command;
    
    for (int i = 0; i < length; i++) {
        command += (char)message[i];
    }

    if (command == "LEFT") {
        processMovement(BASE_LEFT);
    } else if (command == "RIGHT") {
        processMovement(BASE_RIGHT);
    }
}

void setup_wifi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

void reconnect() {
    while (!client.connected()) {
        if (client.connect("ESP32Client")) {
            client.subscribe(mqtt_topic);
        } else {
            delay(5000);
        }
    }
}

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

void drawButtons() {
    drawButton(BTN_LEFT_X, BTN_LEFT_Y, "LEFT", TFT_BLUE);
    drawButton(BTN_RIGHT_X, BTN_RIGHT_Y, "RIGHT", TFT_RED);
}

void drawButton(int x, int y, String label, uint16_t color) {
    tft.fillRect(x, y, BTN_WIDTH, BTN_HEIGHT, color);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(x + 40, y + 20);
    tft.print(label);
}

bool isPressed(uint16_t x, uint16_t y, int btnX, int btnY) {
    return (x >= btnX && x <= btnX + BTN_WIDTH && y >= btnY && y <= btnY + BTN_HEIGHT);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
        if (isPressed(x, y, BTN_LEFT_X, BTN_LEFT_Y)) {
            drawButton(BTN_LEFT_X, BTN_LEFT_Y, "LEFT", TFT_DARKGREY);
            delay(200);
            drawButton(BTN_LEFT_X, BTN_LEFT_Y, "LEFT", TFT_BLUE);
            processMovement(BASE_LEFT);
        }
        if (isPressed(x, y, BTN_RIGHT_X, BTN_RIGHT_Y)) {
            drawButton(BTN_RIGHT_X, BTN_RIGHT_Y, "RIGHT", TFT_DARKGREY);
            delay(200);
            drawButton(BTN_RIGHT_X, BTN_RIGHT_Y, "RIGHT", TFT_RED);
            processMovement(BASE_RIGHT);
        }
    }
}