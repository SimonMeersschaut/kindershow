#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>

// --- Hardware & LED Configuration ---
#define LED_PIN     13
#define AUX_PIN     14      // Pin G14 definition
#define NUM_LEDS    8      // Adjust to the number of LEDs on your strip
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

// --- Network Configuration ---
const char* ssid     = "Meersschaut Smart";
const char* password = "Mjcmss2112";

WebServer server(8000); // Listening on port 8000

// Helper: Fill entire strip with a specific color
void setStripColor(CRGB color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
}

// Dynamic GET Route Handler for /scene/<scene_name>
void handleSceneRequest() {
  // Extract URI path (e.g., "/scene/BLUE")
  String uri = server.uri();
  
  // Extract the target scene after "/scene/"
  String sceneName = uri.substring(7);
  sceneName.toUpperCase();
  sceneName.trim();

  Serial.print("Received Scene Request: ");
  Serial.println(sceneName);

  // Parse color/scene actions
  if (sceneName == "RED") {
    setStripColor(CRGB::Red);
    digitalWrite(AUX_PIN, HIGH);
  } else if (sceneName == "GREEN") {
    setStripColor(CRGB::Green);
    digitalWrite(AUX_PIN, HIGH);
  } else if (sceneName == "BLUE") {
    setStripColor(CRGB::Blue);
    digitalWrite(AUX_PIN, HIGH);
  } else if (sceneName == "WHITE") {
    setStripColor(CRGB::White);
    digitalWrite(AUX_PIN, HIGH);
  } else if (sceneName == "OFF") {
    setStripColor(CRGB::Black);
    digitalWrite(AUX_PIN, LOW); // Set G14 LOW specifically for the OFF scene
  } else {
    server.send(400, "text/plain", "Unknown Scene: " + sceneName);
    return;
  }

  // Acknowledge request (visible in web browser)
  server.send(200, "text/plain", "OK: Scene set to " + sceneName);
}

void setup() {
  Serial.begin(115200);

  // Initialize GPIO 14 as an OUTPUT and set to default LOW
  pinMode(AUX_PIN, OUTPUT);
  digitalWrite(AUX_PIN, LOW);

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
}