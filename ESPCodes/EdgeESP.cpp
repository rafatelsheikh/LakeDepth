#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <SoftwareSerial.h>


#define TRIG D1
#define ECHO D0
#define SIM800_TX D4
#define SIM800_RX D3

SoftwareSerial sim800(SIM800_TX, SIM800_RX);

bool isReading = 1;
float edgeSensor;
char midSensor[255];
char incomingPacket[255];
const char *number = ""; // enter your phone number here

const char *ssidSensor = "Sheikho";       
const char *passwordSensor = "21112004";

// UDP Settings
const int udpPort = 1503;
WiFiUDP udp;

void sendToServer(String sensor_1, String sensor_2);
float readDistance();
void receiveData();
bool waitForEmptyPacket();

void setup() {
    Serial.begin(115200);
    // Start SIM800L communication
    sim800.begin(9600);

    // Initialize SIM800L
    initializeSIM800L();

    // Set ESP to both AP mode
    WiFi.mode(WIFI_AP);

    // Start SoftAP for Remote ESP to connect
    WiFi.softAP(ssidSensor, passwordSensor);
    Serial.print("SoftAP IP Address: ");
    Serial.println(WiFi.softAPIP());

    // Start listening for UDP packets from the Remote ESP
    udp.begin(udpPort);

    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);
}

void loop() {
    if (!waitForEmptyPacket()) {
        Serial.println("Handshake failed. Retrying...");
        delay(1000);
        return;
    }

    Serial.println("Handshake successful. Getting real data...");

    delay(2000);

    receiveData();

    delay(2000);

    if (strlen(incomingPacket) == 0 || strcmp(incomingPacket, "Hello") == 0) {
        return;
    }

    edgeSensor = readDistance();
    
    delay(2000);

    sendToServer(String(edgeSensor), String(midSensor));

    delay(60000);
}

void sendToServer(String sensor_1, String sensor_2) {
String message= "first reading = " + sensor_1 + " second reading = " + sensor_2;
//Sets the SMS mode to Text Mode
    sim800.println("AT+CMGF=1");
    delay(1000);
sim800.print("AT+CMGS=\"");
sim800.print(number);
sim800.println("\"");
delay(1000);
sim800.print(message);
delay(500);
sim800.write(26); // CTRL+Z
delay(3000);
Serial.println("SMS Sent!");
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

void receiveData() {
    int packetSize = 0;
    int len = 0;
    int i = 0;
    while (true && i < 20) {
        packetSize = udp.parsePacket();
        len = udp.read(incomingPacket, sizeof(incomingPacket) - 1);
        Serial.println("inside the loop");
        if (strlen(incomingPacket) != 0 && strcmp(incomingPacket, "Hello") != 0) {
            Serial.println("inside the if");
            break;
        }

        if (i >= 20) {
            return;
        }
        i++;
        delay(1000);
    }

    if (len > 0 && i < 20) {
        incomingPacket[len] = '\0';
    }
    Serial.print("Received Water Level: ");
    Serial.println(incomingPacket);
    strcpy(midSensor, incomingPacket);
}


bool waitForEmptyPacket() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        int len = udp.read(incomingPacket, sizeof(incomingPacket) - 1);
        incomingPacket[len] = '\0';

        if (strcmp(incomingPacket, "Hello") == 0) {  // Empty packet received
            Serial.println("Received empty packet. Sending acknowledgment.");
            udp.beginPacket(udp.remoteIP(), udpPort);
            udp.print("ACK");
            udp.endPacket();
            return true;
        }
    }
    
    return false;  
}
void initializeSIM800L() {
    Serial.println("Initializing SIM800L...");
  
    for (int i = 0; i < 5; i++) {
      sim800.println("AT");
      delay(1000);
      if (sim800.find("OK")) {
        Serial.println("SIM800L is OK");
        break;
      } else {
        Serial.println("Attempt " + String(i + 1) + ": SIM800L is not responding");
        if (i == 4) {
          Serial.println("Failed to initialize SIM800L.");
          return;
        }
      }
    }
    //Checks if the sim is ready 
    sim800.println("AT+CPIN?");
    delay(1000);
    if (sim800.find("READY")) {
      Serial.println("SIM800L is ready");
    } else {
      Serial.println("SIM800L is not ready");
      return;
    }
  }
