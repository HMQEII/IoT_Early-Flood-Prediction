#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "AdafruitIO_WiFi.h"
#include <DHT.h>
#include <SoftwareSerial.h>

// DHT Sensor
#define DHTPIN D4       // Pin connected to the DHT22
#define DHTTYPE DHT22   // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

// Ultrasonic Sensor
#define TRIG_PIN D5     // Trig pin for Ultrasonic Sensor
#define ECHO_PIN D6     // Echo pin for Ultrasonic Sensor

// LDR Sensor
#define LDR_PIN A0      // LDR sensor pin (Analog input)

// SIM900A (Software Serial)
SoftwareSerial sim900(2, 3);  // RX, TX

// WiFi credentials
#define WIFI_SSID       "Panasonic"
#define WIFI_PASS       "Agnes1940"

// Adafruit IO credentials
#define AIO_USERNAME    "HMQEII_AEC"
#define AIO_KEY         "aio_ejkH307thwwBkqlgSLlM8tsfI98a"

// Create an instance of the Adafruit IO client
AdafruitIO_WiFi io(AIO_USERNAME, AIO_KEY, WIFI_SSID, WIFI_PASS);

// Create feeds for each sensor
AdafruitIO_Feed *tempFeed = io.feed("temperature");
AdafruitIO_Feed *humidityFeed = io.feed("humidity");
AdafruitIO_Feed *ldrFeed = io.feed("ldr_value");
AdafruitIO_Feed *distanceFeed = io.feed("distance");
AdafruitIO_Feed *locationFeed = io.feed("location");
AdafruitIO_Feed *timeFeed = io.feed("time");

void setup() {
  // Start Serial for debugging
  Serial.begin(9600);

  // Initialize SIM900
  sim900.begin(9600);
  delay(1000);
  Serial.println("SIM900A Initialized!");

  // Connect to WiFi
  Serial.print("Connecting to WiFi...");
  io.connect();

  // Wait for connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nAdafruit IO Connected!");

  // Initialize sensors
  dht.begin();

  // Initialize Ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

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
  // long duration, distance;
  // digitalWrite(TRIG_PIN, LOW);
  // delayMicroseconds(2);
  // digitalWrite(TRIG_PIN, HIGH);
  // delayMicroseconds(10);
  // digitalWrite(TRIG_PIN, LOW);
  // duration = pulseIn(ECHO_PIN, HIGH);
  // distance = (duration / 2) / 29.1;  // Convert to cm

  long duration, distance;
  int waterLevelValue; // This will store the reversed water level value

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
    // If distance is 100 cm or less, water level is high
    waterLevelValue = map(distance, 0, 200, 200, 0);  // Reverse mapping: 0 to 100 cm becomes 100 to 0
  } else {
    // If distance is more than 100 cm, water level is low
    waterLevelValue = 100;  // Beyond 100 cm, set the value to 0
  }

  // 4. Get location and time from SIM900A
  String location = getLocationFromSIM900();
  String time = getTimeFromSIM900();

  // --- Send data to Adafruit IO ---
  
  // Send temperature and humidity
  tempFeed->save(temperature);
  humidityFeed->save(humidity);

  // Send LDR value
  ldrFeed->save(ldrValue);

  // Send ultrasonic sensor distance
  distanceFeed->save(waterLevelValue);

  // Send location and time from SIM900A
  locationFeed->save(location);
  timeFeed->save(time);

  // Print to Serial Monitor
  Serial.print("Temperature: "); Serial.println(temperature);
  Serial.print("Humidity: "); Serial.println(humidity);
  Serial.print("LDR Value: "); Serial.println(ldrValue);
  Serial.print("Distance: "); Serial.println(distance); Serial.println(waterLevelValue);
  Serial.print("Location: "); Serial.println(location);
  Serial.print("Time: "); Serial.println(time);

  // Delay between sending data (adjust as needed)
  delay(12000);
}

// --- Helper function to get location from SIM900A ---
String getLocationFromSIM900() {
  sim900.println("AT+CIPGSMLOC=1,1");  // Command to get location
  delay(2000);                         // Wait for response
  String loc = "";
  while (sim900.available()) {
    loc += char(sim900.read());
  }
  return loc;
}

// --- Helper function to get time from SIM900A ---
String getTimeFromSIM900() {
  sim900.println("AT+CCLK?");  // Command to get time
  delay(1000);                 // Wait for response
  String time = "";
  while (sim900.available()) {
    time += char(sim900.read());
  }
  return time;
}
