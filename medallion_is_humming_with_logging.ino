#include <WiFi.h>
#include <driver/adc.h>
#include <FS.h>
#include <LittleFS.h>

// Pin assignments
#define MOTOR_PIN 6     // D4 pad on XIAO ESP32-C3 (GPIO6 is actually D6 on XIAO C3)
#define LED_PIN   5     // External LED pin (D5/GPIO5)
#define VBAT_PIN  A0    // Battery voltage sense pin

// Settings
#define SCAN_INTERVAL_SEC 60    // How often to scan for Wi-Fi (seconds)
#define LOW_BATTERY_VOLTAGE 3.4 // Voltage threshold for low battery (V)
#define LOG_FILE "/wifi_log.txt"

// Longer, rumblier vibration pattern
int vibrationPattern[] = {100, 100, 50, 150, 100, 150, 200, 200, 100, 100, 150, 200, 300, 150, 150, 250, 100, 150, 150, 250, 100};
#define VIB_PATTERN_LEN (sizeof(vibrationPattern)/sizeof(vibrationPattern[0]))

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to wake up

  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Setup battery ADC
  analogReadResolution(12); 
  analogSetAttenuation(ADC_11db); 

  // Initialize File System
  if(!LittleFS.begin(true)){ // true = format if failed
    Serial.println("LittleFS Mount Failed");
    return;
  }
  
  // Print existing logs to Serial Monitor on boot
  printLogs();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  float vbat = readBatteryVoltage();
  Serial.printf("Battery voltage: %.2f V\n", vbat);

  if (vbat < LOW_BATTERY_VOLTAGE) {
    Serial.println("⚠️  Battery low! Flashing LED...");
    flashLowBatteryLED(5);
  }

  Serial.println("Witcher Medallion - Setup complete");
}

void loop() {
  Serial.println("Scanning for Wi-Fi...");

  bool foundOpen = false;
  String foundSSID = ""; // Store the name of the open network

  // Scan for visible networks only
  int n = WiFi.scanNetworks(false, false);
  Serial.printf("Found %d networks\n", n);

  for (int i = 0; i < n; ++i) {
    Serial.printf("%s : %d\n", WiFi.SSID(i).c_str(), WiFi.encryptionType(i));
    if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) {
      foundOpen = true;
      foundSSID = WiFi.SSID(i);
      // Log the find immediately
      logNetwork(foundSSID); 
      break; // Stop after finding the first open one
    }
  }

  if (foundOpen) {
    Serial.printf("⚡ Open network found: %s! Vibrating...\n", foundSSID.c_str());
    vibratePattern();
  } else {
    Serial.println("No open networks found.");
  }

  WiFi.scanDelete();

  float vbat = readBatteryVoltage();
  
  if (vbat < LOW_BATTERY_VOLTAGE) {
    flashLowBatteryLED(3);
  }

  Serial.println("Sleeping...");
  esp_sleep_enable_timer_wakeup(SCAN_INTERVAL_SEC * 1000000ULL);
  esp_deep_sleep_start(); 
}

// --- Helper Functions ---

// Append a network name to the log file
void logNetwork(String ssid) {
  File file = LittleFS.open(LOG_FILE, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open log file for appending");
    return;
  }
  // We don't have a Real Time Clock (RTC), so we can't log the real date/time.
  // We will just log the SSID found.
  file.println(ssid);
  file.close();
  Serial.println("Saved to log.");
}

// Read and print the entire log file
void printLogs() {
  Serial.println("--- READING SAVED LOGS ---");
  
  if(!LittleFS.exists(LOG_FILE)){
    Serial.println("No log file found.");
    return;
  }

  File file = LittleFS.open(LOG_FILE, FILE_READ);
  if(!file){
    Serial.println("Failed to open log file for reading");
    return;
  }

  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
  Serial.println("\n--- END OF LOGS ---");
}

void vibratePattern() {
  for (int i = 0; i < VIB_PATTERN_LEN; i++) {
    digitalWrite(MOTOR_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);   
    delay(vibrationPattern[i]);
    digitalWrite(MOTOR_PIN, LOW);
    digitalWrite(LED_PIN, LOW);    
    delay(50); 
  }
}

float readBatteryVoltage() {
  int raw = analogRead(VBAT_PIN);
  float pinVoltage = (raw / 4095.0) * 2.5; 
  float batteryVoltage = pinVoltage * 2.0; 
  return batteryVoltage;
}

void flashLowBatteryLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}
