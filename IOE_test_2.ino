#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "AdafruitIO_WiFi.h"
#include <DHT.h>
#include <SoftwareSerial.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>  // Include this for HTTPClient

// DHT Sensor
#define DHTPIN D4       // Pin connected to the DHT22
#define DHTTYPE DHT22   // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

// Ultrasonic Sensor
#define TRIG_PIN D5     // Trig pin for Ultrasonic Sensor
#define ECHO_PIN D6     // Echo pin for Ultrasonic Sensor

// LDR Sensor
#define LDR_PIN A0      // LDR sensor pin (Analog input)
#define BUZZ D7
// Adafruit IO credentials
#define AIO_USERNAME    "HMQEII_AEC"
#define AIO_KEY         "aio_AAvo41CqEZmB8AJhlSuTsHLd5piL"

// NTP Client for time synchronization
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // Adjust for IST (19800 seconds = 5.5 hours)

// Variables for WiFi credentials
String ssid;
String password;

// Create an instance of the Adafruit IO client
AdafruitIO_WiFi io(AIO_USERNAME, AIO_KEY, ssid.c_str(), password.c_str());

// Create feeds for each sensor
AdafruitIO_Feed *tempFeed = io.feed("temperature");
AdafruitIO_Feed *humidityFeed = io.feed("humidity");
AdafruitIO_Feed *ldrFeed = io.feed("ldr_value");
AdafruitIO_Feed *distanceFeed = io.feed("distance");
AdafruitIO_Feed *locationFeed = io.feed("location");
AdafruitIO_Feed *timeFeed = io.feed("time");

// Global variable to store the location
String location = "Unknown";

void setup() {
  // Start Serial for debugging and user input
  Serial.begin(9600);

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
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Connect to Adafruit IO using the credentials
  io.connect();

  // Wait for Adafruit IO connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nAdafruit IO Connected!");

  // Start NTP client
  timeClient.begin();

  // Initialize sensors
  dht.begin();

  // Initialize Ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Retrieve location only once
  location = getLocationFromWiFi();

  // Send location to Adafruit IO (only once)
  locationFeed->save(location);

  // Delay to stabilize everything
  delay(2000);
}

void loop() {
  // Ensure connection to Adafruit IO
  io.run();

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

  // --- Send data to Adafruit IO ---
  
  // Send temperature and humidity
  tempFeed->save(temperature);
  humidityFeed->save(humidity);

  // Send LDR value
  ldrFeed->save(ldrValue);

  // Send ultrasonic sensor distance
  distanceFeed->save(waterLevelValue);

  // Send time (time is updated each loop cycle)
  timeFeed->save(time);

  // Print to Serial Monitor
  Serial.print("Temperature: "); Serial.println(temperature);
  Serial.print("Humidity: "); Serial.println(humidity);
  Serial.print("LDR Value: "); Serial.println(ldrValue);
  Serial.print("Distance: "); Serial.println(distance); Serial.println(waterLevelValue);
  Serial.print("Location: "); Serial.println(location);  // Location retrieved once and used repeatedly
  Serial.print("Time: "); Serial.println(time);

  // Delay between sending data (adjust as needed)
  tone(BUZZ, 100, 1);
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
