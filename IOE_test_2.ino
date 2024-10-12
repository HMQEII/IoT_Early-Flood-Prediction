#include <ESP8266WiFi.h>
#include <DHT.h>
#include <SoftwareSerial.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

// DHT Sensor
#define DHTPIN D4       // Pin connected to the DHT22
#define DHTTYPE DHT22   // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

// Ultrasonic Sensor
#define TRIG_PIN D5     // Trig pin for Ultrasonic Sensor
#define ECHO_PIN D6     // Echo pin for Ultrasonic Sensor

// LDR Sensor
#define LDR_PIN A0      // LDR sensor pin (Analog input)
#define BUZZ D7         // Buzzer Pin
#define LEDIND D8       // LED indicator

// NTP Client for time synchronization
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // Adjust for IST (19800 seconds = 5.5 hours)

// Variables for WiFi credentials
String ssid;
String password;

// Global variable to store the location
String location = "Mira Road";

// Flask API endpoint
const char* serverName = "https://io-t-connectivity-api.vercel.app/api/data";  // Replace with your Flask API URL

// HTTP Client instance (move this outside of loop)
HTTPClient http;

void setup() {
  // Start Serial for debugging and user input
  Serial.begin(9600);
  pinMode(LEDIND, OUTPUT);

  // Request WiFi credentials from the user
  Serial.println("Please enter SSID: ");
  while (ssid == "") {
    if (Serial.available()) {
      ssid = Serial.readStringUntil('\n');  // Read SSID from user input
      ssid.trim();  // Remove any extra spaces
    }
  }

  Serial.println("Please enter Password: ");
  while (password == "") {
    if (Serial.available()) {
      password = Serial.readStringUntil('\n');  // Read password from user input
      password.trim();  // Remove any extra spaces
    }
  }

  // Connect to WiFi with user-provided credentials
  WiFi.begin(ssid.c_str(), password.c_str());

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LEDIND, HIGH);   // LED ON
    delay(300);                    // Short delay
    digitalWrite(LEDIND, LOW);    // LED OFF
    delay(300);
    delay(500);
    Serial.print("...");
  }
  Serial.println("\nWiFi Connected!");

  // Start NTP client
  timeClient.begin();

  // Initialize sensors
  dht.begin();

  // Initialize Ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Retrieve location only once
  location = getLocationFromWiFi();
}

void loop() {
  // --- Read sensor values ---
  
  // 1. Read temperature and humidity from DHT22
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // 2. Read LDR value
  int ldrValue = analogRead(LDR_PIN);

  // 3. Read distance from ultrasonic sensor
  long duration, distance;
  int waterLevelValue;

  // Trigger the ultrasonic sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure the time taken for the echo
  duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate the distance in cm
  distance = (duration / 2) / 29.1;

  // Reverse logic: low distance -> high value, high distance -> low value
  if (distance <= 200) {
    waterLevelValue = map(distance, 0, 100, 100, 0);  // Reverse mapping
  } else {
    waterLevelValue = 100;  // Beyond 100 cm, set the value to 100
  }

  // 4. Get time from NTP client
  timeClient.update();
  String time = timeClient.getFormattedTime();

  // --- Prepare JSON payload ---
  String postData;
  DynamicJsonDocument doc(1024);
  
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["ldr_value"] = ldrValue;
  doc["distance"] = waterLevelValue;
  doc["location"] = location;
  doc["time"] = time;
  
  serializeJson(doc, postData);

  // --- Send data to Flask API ---
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;  // Create a secure WiFiClient
    client.setInsecure();  // If you don't have a certificate, disable SSL validation (insecure)
    http.begin(client, serverName);  // Begin with the secure client

    // WiFiClient client;  // Create a WiFiClient instance
    // http.begin(client, serverName);  // Specify your Flask API URL with the WiFiClient object
    http.addHeader("Content-Type", "application/json");  // Specify content type header
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // Follow redirects automatically

    int httpResponseCode = http.POST(postData);  // Send POST request
    if (httpResponseCode > 0) {
      String response = http.getString();  // Get the response to the request
      Serial.println("HTTP Response Code: " + String(httpResponseCode));  // Print return code
      Serial.println("Response: " + response);  // Print response
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();  // Free resources
  }

  // Delay between sending data (adjust as needed)
  tone(BUZZ, 100, 1);
  digitalWrite(LEDIND, HIGH);   // LED ON (Indicating successful operation)
  delay(100);                    // Short delay for the blink
  digitalWrite(LEDIND, LOW);
  delay(12000);
}

// --- Helper function to get location from Wi-Fi ---
String getLocationFromWiFi() {
  WiFiClientSecure client;
  client.setInsecure();  // Disable SSL certificate validation

  HTTPClient http;  // Use HTTPClient after including ESP8266HTTPClient.h
  http.begin(client, "https://ipapi.co/json");  // Use HTTPS URL

  int httpCode = http.GET();

  String city = "Unknown";
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Location Data: " + payload);

    // Parse JSON to get the city name
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    city = doc["city"].as<String>();  // Extract the city
  } else {
    Serial.println("Error in HTTP request");
  }

  http.end();
  return city;
}
