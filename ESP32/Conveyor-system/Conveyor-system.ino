#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

const char* ssid = "";  
const char* password = "";  
const char* mqtt_server = "172.20.10.3";  
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/IR";  

WiFiClient espClient;
PubSubClient client(espClient);

#define IN1 18        
#define IN2 19        
#define IR_SENSOR 5   
#define SERVO_PIN 13  

Servo myServo; 

void setup() {
    Serial.begin(115200);
    
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IR_SENSOR, INPUT);
    
    myServo.attach(SERVO_PIN); 
    myServo.write(0);  

    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("WiFi connected");

    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(mqttCallback);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    
    int irValue = digitalRead(IR_SENSOR);

    if (irValue == LOW) { 
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        Serial.println("Object detected! Moving servo...");
        
        myServo.write(180);
        delay(1000);

        myServo.write(0);
        delay(500);
        
        client.publish(mqtt_topic, "Object detected!");
        Serial.println("Object detected!");

        delay(2000); 

        Serial.println("Resuming motor...");
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);

        Serial.println("Waiting 5 seconds before scanning again...");
        delay(5000); 
    } 
}

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

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}
