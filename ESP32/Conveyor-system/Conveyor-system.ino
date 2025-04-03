#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// 🔹 ตั้งค่า WiFi และ MQTT
const char* ssid = "Yindee";  
const char* password = "phoorin3799";  
const char* mqtt_server = "172.20.10.3";  // IP ของคอมที่รัน MQTT Broker
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/IR";  // MQTT Topic

WiFiClient espClient;
PubSubClient client(espClient);

#define IN1 18         // ขาควบคุมมอเตอร์
#define IN2 19         // ขาควบคุมมอเตอร์
#define IR_SENSOR 5    // ขาเชื่อมต่อ IR Sensor
#define SERVO_PIN 13   // ขาเชื่อมต่อเซอร์โว

Servo myServo; // สร้างอ็อบเจ็กต์เซอร์โว

void setup() {
    Serial.begin(115200);
    
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IR_SENSOR, INPUT);
    
    myServo.attach(SERVO_PIN); // เชื่อมต่อเซอร์โว
    myServo.write(0);  // ตั้งค่าเริ่มต้นให้เซอร์โวอยู่ที่ 0 องศา

    // เริ่มต้นให้มอเตอร์หมุนไปข้างหน้า
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);

    // เชื่อมต่อ WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("WiFi connected");

    // ตั้งค่า MQTT
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(mqttCallback);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    
    int irValue = digitalRead(IR_SENSOR); // อ่านค่าจากเซนเซอร์ IR

    if (irValue == LOW) { 
        // ถ้าตรวจพบวัตถุ (IR ส่งค่า LOW)
        digitalWrite(IN1, LOW); // หยุดมอเตอร์
        digitalWrite(IN2, LOW);
        Serial.println("Object detected! Moving servo...");
        
        myServo.write(180); // หมุนเซอร์โวไปที่ 180 องศา
        delay(1000); // หน่วงเวลาให้เซอร์โวทำงาน

        myServo.write(0); // กลับมาที่ตำแหน่งเริ่มต้น
        delay(500); // หน่วงเวลาให้เซอร์โวกลับตำแหน่ง
        
        // ส่งข้อมูลผ่าน MQTT
        client.publish(mqtt_topic, "Object detected!");
        Serial.println("Object detected!");

        delay(2000); 

        // มอเตอร์เดินหน้าต่อ
        Serial.println("Resuming motor...");
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);

        // ดีเลย์ 5 วินาทีก่อนให้ IR Sensor สแกนใหม่
        Serial.println("Waiting 5 seconds before scanning again...");
        delay(5000); 
    
    } 
}

// ฟังก์ชันสำหรับการเชื่อมต่อ MQTT
void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("ESP32Client")) {
            Serial.println("connected");
            client.subscribe(mqtt_topic);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

// ฟังก์ชันที่เรียกใช้เมื่อรับข้อมูลจาก MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}
