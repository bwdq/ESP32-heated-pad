#include <WiFi.h>
#include <WebServer.h>

//////////////////////
// CONFIGURABLE DATA
//////////////////////
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

const int pinIntensity = D3;
const int pinDuration  = D4;

//////////////////////
// STATE VARIABLES
//////////////////////
int intensityLevels[] = {1, 2, 3, 4, 5, 6};
int durationOptions[] = {30, 60, 90, 120}; // in minutes

int intensityIndex = 2; // default intensity = 3 (index 2)
int durationIndex  = 1; // default duration = 60 min (index 1)

unsigned long durationStart = 0; 
unsigned long durationMillis = durationOptions[durationIndex] * 60UL * 1000UL;

//////////////////////
// SERVER
//////////////////////
WebServer server(80);

void updateDurationTime() {
  unsigned long elapsed = millis() - durationStart;
  if (elapsed >= durationMillis) {
    durationMillis = 0;  // time’s up
  }
}

void handleState() {
  Serial.println("[HTTP GET] /state requested");

  updateDurationTime();
  unsigned long elapsed = millis() - durationStart;
  unsigned long remaining = (elapsed < durationMillis) ? (durationMillis - elapsed) : 0;
  
  String json = "{";
  json += "\"intensity\":" + String(intensityLevels[intensityIndex]) + ",";
  json += "\"duration\":" + String(durationOptions[durationIndex]) + ",";
  json += "\"time_remaining_ms\":" + String(remaining);
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleSet() {
  Serial.print("[HTTP POST] /set requested with args: ");  // <-- Debug message
  for (uint8_t i = 0; i < server.args(); i++) {
    Serial.print(server.argName(i));
    Serial.print("=");
    Serial.print(server.arg(i));
    if (i < server.args() - 1) Serial.print(", ");
  }
  Serial.println();

  if (server.hasArg("intensity")) {
    int val = server.arg("intensity").toInt();
    if (val >= 1 && val <= 6) {
      //calculate number of button presses with wraparound
      int desired = val - 1;
      int presses = ((desired + 6) - intensityIndex) % 6;
      for (int i = 0; i < presses; i++) {
        incrementIntensity();
      }
    }
  }
  if (server.hasArg("duration")) {
    int val = server.arg("duration").toInt();
    val = (val / 30);
    if (val >= 1 && val <= 4) {
      //calculate number of button presses with wraparound
      int desired = val - 1;
      int presses = ((desired + 4) - durationIndex) % 4;
      for (int i = 0; i < presses; i++) {
        incrementDuration();
      }
    }
  }
  
  handleState();
}

//////////////////////
// OUTPUT CONTROL
//////////////////////
void incrementIntensity() {
  intensityIndex = (intensityIndex + 1) % 6;
  digitalWrite(pinIntensity, HIGH);
  delay(100);
  digitalWrite(pinIntensity, LOW);
  delay(1000);
  Serial.printf("Intensity incremented to %d\n", intensityLevels[intensityIndex]);
}

void incrementDuration() {
  durationIndex = (durationIndex + 1) % 4;
  durationMillis = durationOptions[durationIndex] * 60UL * 1000UL;
  durationStart = millis();
  digitalWrite(pinDuration, HIGH);
  delay(100);
  digitalWrite(pinDuration, LOW);
  delay(1000);
  Serial.printf("Duration incremented to %d min\n", durationOptions[durationIndex]);
}

//////////////////////
// SETUP
//////////////////////
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(pinIntensity, OUTPUT);
  pinMode(pinDuration, OUTPUT);
  digitalWrite(pinIntensity, LOW);
  digitalWrite(pinDuration, LOW);

  Serial.printf("Connecting to WiFi: %s\n", ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n WiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Setup routes
  server.on("/state", HTTP_GET, handleState);
  server.on("/set", HTTP_POST, handleSet);
  server.begin();

  Serial.println("HTTP server started.");
  Serial.println("Access the device at:");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
}

//////////////////////
// LOOP
//////////////////////
void loop() {
  server.handleClient();
  updateDurationTime();
}
