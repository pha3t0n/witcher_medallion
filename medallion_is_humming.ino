#include <WiFi.h>
#include <driver/adc.h>

// Pin assignments
#define MOTOR_PIN 6     // D4 pad on XIAO ESP32-C3
// *** MODIFICATION: Changed from 10 (onboard) to 5 (external pin D1) ***
#define LED_PIN   5     // External LED pin (D4/GPIO5)
#define VBAT_PIN  A0    // Battery voltage sense pin

// Settings
#define SCAN_INTERVAL_SEC 60    // How often to scan for Wi-Fi (seconds)
#define LOW_BATTERY_VOLTAGE 3.4 // Voltage threshold for low battery (V)

// Longer, rumblier vibration pattern (about twice the total time)
int vibrationPattern[] = {100, 100, 50, 150, 100, 150, 200, 200, 100, 100, 150, 200, 300, 150, 150, 250, 100, 150, 150, 250, 100};
#define VIB_PATTERN_LEN (sizeof(vibrationPattern)/sizeof(vibrationPattern[0]))

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  // This will now control your new external LED on pin 5
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Setup battery ADC
  analogReadResolution(12); // 12-bit ADC (0–4095)
  analogSetAttenuation(ADC_11db); // Set 11dB attenuation (provides ~2.5V range on ESP32-C3)

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  float vbat = readBatteryVoltage();
  Serial.printf("Battery voltage: %.2f V\n", vbat);

  if (vbat < LOW_BATTERY_VOLTAGE) {
    Serial.println("⚠️  Battery low! Flashing LED...");
    flashLowBatteryLED(5); // This will now flash your external LED
  }

  Serial.println("Witcher Medallion - Setup complete");
}

void loop() {
  Serial.println("Scanning for Wi-Fi...");

  bool foundOpen = false;
  
  // Scan for visible networks only (false, false) for faster scan
  int n = WiFi.scanNetworks(false, false);

  Serial.printf("Found %d networks\n", n);

  for (int i = 0; i < n; ++i) {
    Serial.printf("%s : %d\n", WiFi.SSID(i).c_str(), WiFi.encryptionType(i));
    if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) {
      foundOpen = true;
      break;
    }
  }

  if (foundOpen) {
    Serial.println("⚡ Open network found! Vibrating...");
    vibratePattern(); // This will now flash your external LED
  } else {
    Serial.println("No open networks found.");
  }

  WiFi.scanDelete();

  float vbat = readBatteryVoltage();
  Serial.printf("Battery: %.2f V\n", vbat);

  if (vbat < LOW_BATTERY_VOLTAGE) {
    flashLowBatteryLED(3); // This will also flash your external LED
  }

  Serial.println("Sleeping...");
  esp_sleep_enable_timer_wakeup(SCAN_INTERVAL_SEC * 1000000ULL);
  esp_deep_sleep_start();  // full deep sleep for best battery life
}

// Vibrate using the defined pattern with LED flash
void vibratePattern() {
  for (int i = 0; i < VIB_PATTERN_LEN; i++) {
    digitalWrite(MOTOR_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);   // Your external LED on
    delay(vibrationPattern[i]);
    digitalWrite(MOTOR_PIN, LOW);
    digitalWrite(LED_PIN, LOW);    // Your external LED off
    delay(50);  // short pause between pulses
  }
}

// Read battery voltage
float readBatteryVoltage() {
  int raw = analogRead(VBAT_PIN);
  
  // 1. Calculate the voltage at the ADC pin (which has a ~2.5V max range)
  float pinVoltage = (raw / 4095.0) * 2.5; 
  
  // 2. Multiply by 2.0 to account for the 2:1 voltage divider on the XIAO
  float batteryVoltage = pinVoltage * 2.0; 
  
  return batteryVoltage;
}

// Flash LED to signal low battery
void flashLowBatteryLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH); // Your external LED on
    delay(200);
    digitalWrite(LED_PIN, LOW);  // Your external LED off
    delay(200);
  }
}
