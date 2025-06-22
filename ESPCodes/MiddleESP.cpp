#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <Arduino.h>

#define TRIG D1  // Ultrasonic Trigger Pin
#define ECHO D0  // Ultrasonic Echo Pin

const char *ssid = "Sheikho";       
const char *password = "21112004";
const char* udpServerIP = "192.168.4.1"; // Onshore ESP's SoftAP IP
const int udpPort = 1503; // UDP Port

WiFiUDP udp;

bool sendEmptyPacket();
float readDistance();

float distance;

void setup() {
    Serial.begin(115200);
    
    WiFi.mode(WIFI_STA); // Set ESP8266 to station mode
    WiFi.begin(ssid, password);

    Serial.print("Connecting to Onshore ESP AP");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Onshore ESP!");
    Serial.print("ESP IP Address: ");
    Serial.println(WiFi.localIP());

    udp.begin(udpPort);

    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);
}

void loop() {
    if (!sendEmptyPacket()) {
        Serial.println("Handshake failed. Retrying...");
        delay(1000);
        return;
    }

    distance = readDistance(); 
    delay(2000);
    udp.beginPacket(udpServerIP, udpPort);
    udp.print(String(distance)); 
    udp.endPacket();

    Serial.println("UDP packet sent: " + String(distance) + " cm");

    delay(60000); // Wait before next reading
}

bool sendEmptyPacket() {
    udp.beginPacket(udpServerIP, udpPort);
    udp.print("Hello");  // Send an empty packet
    udp.endPacket();

    Serial.println("Sent empty packet. Waiting for acknowledgment...");

    delay(1000);

    int packetSize = udp.parsePacket();
    if (packetSize) {
        char ack[10];
        int len = udp.read(ack, sizeof(ack) - 1);
        ack[len] = '\0';
        
        if (strcmp(ack, "ACK") == 0) {
            Serial.println("Received ACK. Proceeding to send data...");
            return true;  // Handshake successful
        }
    }
    return false;  // Handshake failed
}

float readDistance() {
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    long duration = pulseIn(ECHO, HIGH, 100000); // Timeout after 100ms
    if (duration == 0) {
        Serial.println("Ultrasonic sensor failed to read");
        return -1;
    }

    float sound_wave = 0.034; // Speed of sound in cm/us
    return (duration * sound_wave) / 2.0; 
}
