#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <ESP32Servo.h> // Make sure to install "ESP32Servo" by Kevin Harrington via Library Manager

// --- Hardware & LED Configuration ---
#define LED_PIN     13
#define AUX_PIN     14      // Pin G14 definition
#define SERVO_PIN   27      // Servo connected to GPIO 27
#define NUM_LEDS    8       // Adjust to the number of LEDs on your strip
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
Servo headServo;

// --- Network Configuration ---
const char* ssid     = "Meersschaut Smart";
const char* password = "Mjcmss2112";

WebServer server(8000); // Listening on port 8000

// --- Scene State Management ---
enum SceneState {
  STATE_OFF,
  STATE_SPEAKING,
  STATE_IDLE,
  STATE_RED_SOLID,
  STATE_GREEN_SOLID,
  STATE_BLUE_SOLID
};

SceneState currentScene = STATE_OFF;

// Helper: Fill entire strip with a specific color
void setStripColor(CRGB color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
}

// Dynamic GET Route Handler for /scene/<scene_name>
void handleSceneRequest() {
  String uri = server.uri();
  
  // Extract the target scene after "/scene/"
  String sceneName = uri.substring(7);
  sceneName.toUpperCase();
  sceneName.trim();

  Serial.print("Received Scene Request: ");
  Serial.println(sceneName);

  if (sceneName == "OFF") {
    digitalWrite(AUX_PIN, LOW);
    currentScene = STATE_OFF;
    setStripColor(CRGB::Black);
    headServo.write(90); // Reset servo to center position when OFF
  }
  else {
    digitalWrite(AUX_PIN, HIGH);
    
    if (sceneName == "SPEAKING") {
      currentScene = STATE_SPEAKING;
    } else if (sceneName == "IDLE") {
      currentScene = STATE_IDLE;
      headServo.write(90); // Reset servo to center when IDLE
    } else if (sceneName == "RED-SOLID") {
      currentScene = STATE_RED_SOLID;
      setStripColor(CRGB::Red);
    } else if (sceneName == "GREEN-SOLID") {
      currentScene = STATE_GREEN_SOLID;
      setStripColor(CRGB::Green);
    } else if (sceneName == "BLUE-SOLID") {
      currentScene = STATE_BLUE_SOLID;
      setStripColor(CRGB::Blue);
    } else {
      server.send(400, "text/plain", "Unknown Scene: " + sceneName);
      return;
    }
  }

  // Acknowledge request
  server.send(200, "text/plain", "OK: Scene set to " + sceneName);
}

// Non-blocking Animation Logic for Active Scenes
void updateAnimations() {
  static uint8_t pulseIndex = 0;

  if (currentScene == STATE_SPEAKING) {
    // 1. LED Paarse pulsanimatie
    EVERY_N_MILLISECONDS(15) {
      pulseIndex += 4;
      uint8_t brightness = beatsin8(60, 40, 255, 0, pulseIndex);
      fill_solid(leds, NUM_LEDS, CHSV(192, 255, brightness)); // HSV 192 = Paars
      FastLED.show();
    }

    // 2. Servo sweeping back and forth (40° to 140° angle range)
    EVERY_N_MILLISECONDS(20) {
      // beatsin16(BPM, min_val, max_val) generates smooth sinusoidal motion
      // 15 BPM gives a smooth, slow left-right swing
      uint16_t servoPos = beatsin16(15, 40, 140); 
      headServo.write(servoPos);
    }
  } 
  else if (currentScene == STATE_IDLE) {
    // Rustige paarse fade (langzamer, subtielere gloed)
    EVERY_N_MILLISECONDS(30) {
      uint8_t brightness = beatsin8(15, 20, 100);
      fill_solid(leds, NUM_LEDS, CHSV(192, 255, brightness));
      FastLED.show();
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize GPIO 14 as an OUTPUT and set to default LOW
  pinMode(AUX_PIN, OUTPUT);
  digitalWrite(AUX_PIN, LOW);

  // Initialize Servo on GPIO 27
  ESP32PWM::allocateTimer(0);
  headServo.setPeriodHertz(50);          // Standard 50Hz Servo
  headServo.attach(SERVO_PIN, 500, 2400); // Standard pulse width limits (500us to 2400us)
  headServo.write(90);                   // Start at center position (90 degrees)

  // Initialize FastLED on GPIO 13
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(128); // Set overall brightness (0-255)
  setStripColor(CRGB::Black);  // Default to off

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // Handle wildcards starting with /scene/
  server.onNotFound([]() {
    if (server.uri().startsWith("/scene/")) {
      handleSceneRequest();
    } else {
      server.send(404, "text/plain", "Not Found");
    }
  });

  // Root endpoint for simple browser status checks
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/plain", "ESP32 LED Controller Active. Use /scene/<name>");
  });

  server.begin();
  Serial.println("HTTP Server started on port 8000");
}

void loop() {
  server.handleClient();
  updateAnimations();
}