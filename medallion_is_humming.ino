#include <WiFi.h>
#include <driver/adc.h>

// Pin assignments
#define MOTOR_PIN 6        // D4 pad on XIAO ESP32-C3
#define LED_PIN   10       // Built-in user LED (GPIO10)
#define VBAT_PIN  A0       // Battery voltage sense pin

// Settings
#define SCAN_INTERVAL_SEC 60     // How often to scan for Wi-Fi (seconds)
#define LOW_BATTERY_VOLTAGE 3.4  // Voltage threshold for low battery (V)

// Longer, rumblier vibration pattern (about twice the total time)
int vibrationPattern[] = {100, 100, 50, 150, 100, 150, 200, 200, 100, 100, 150, 200, 300, 150, 150, 250, 100, 150, 150, 250, 100};
#define VIB_PATTERN_LEN (sizeof(vibrationPattern)/sizeof(vibrationPattern[0]))

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Setup battery ADC
  analogReadResolution(12); // 12-bit ADC (0–4095)
  analogSetAttenuation(ADC_11db); // Allows up to ~3.9V measurement range

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
  int n = WiFi.scanNetworks(false, true);

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
    vibratePattern();
  } else {
    Serial.println("No open networks found.");
  }

  WiFi.scanDelete();

  float vbat = readBatteryVoltage();
  Serial.printf("Battery: %.2f V\n", vbat);

  if (vbat < LOW_BATTERY_VOLTAGE) {
    flashLowBatteryLED(3);
  }

  Serial.println("Sleeping...");
  esp_sleep_enable_timer_wakeup(SCAN_INTERVAL_SEC * 1000000ULL);
  esp_deep_sleep_start();  // full deep sleep for best battery life
}

// Vibrate using the defined pattern with LED flash
void vibratePattern() {
  for (int i = 0; i < VIB_PATTERN_LEN; i++) {
    digitalWrite(MOTOR_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);   // LED on during motor pulse
    delay(vibrationPattern[i]);
    digitalWrite(MOTOR_PIN, LOW);
    digitalWrite(LED_PIN, LOW);    // LED off between pulses
    delay(50);  // short pause between pulses
  }
}

// Read battery voltage (adjust divider ratio if needed)
float readBatteryVoltage() {
  int raw = analogRead(VBAT_PIN);
  float voltage = (raw / 4095.0) * 3.3 * 2.0; 
  // Multiply by 2.0 if using 2:1 voltage divider (typical for Seeed XIAO ESP32C3)
  return voltage;
}

// Flash LED to signal low battery
void flashLowBatteryLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}
