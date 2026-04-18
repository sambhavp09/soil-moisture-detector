#include <Arduino.h>

// -------------------------
// Hardware configuration
// -------------------------
const uint8_t SOIL_SENSOR_DO_PIN = D1; // GPIO5 on NodeMCU (sensor DO)
const uint8_t RELAY_PIN = D2;          // GPIO4 on NodeMCU (relay IN)
const bool RELAY_ACTIVE_LOW = true;   // Most relay modules are active LOW
const int SENSOR_DRY_STATE = HIGH;    // Change to LOW if your module logic is inverted

// -------------------------
// Watering logic
// -------------------------
const unsigned long SENSOR_READ_INTERVAL_MS = 1000;  // Read sensor every 1s
const unsigned long PUMP_COOLDOWN_MS = 20000;        // Minimum gap between watering cycles
const unsigned long MAX_PUMP_RUN_MS = 7000;          // Safety timeout per watering cycle

const uint8_t SENSOR_SAMPLES = 7;   // Majority vote to reduce digital noise

unsigned long lastSensorReadAt = 0;
unsigned long pumpStartedAt = 0;
unsigned long lastPumpStoppedAt = 0;

bool pumpIsOn = false;
bool lastSoilDry = false;
int lastMoisturePctApprox = 100;

bool readSoilDryState() {
  uint8_t dryVotes = 0;

  for (uint8_t i = 0; i < SENSOR_SAMPLES; i++) {
    if (digitalRead(SOIL_SENSOR_DO_PIN) == SENSOR_DRY_STATE) {
      dryVotes++;
    }
    delay(5);
  }

  return dryVotes > (SENSOR_SAMPLES / 2);
}

int dryStateToMoisturePercent(bool isDry) {
  // DO is binary, so this is an approximate percentage for telemetry.
  return isDry ? 0 : 100;
}

void setPump(bool turnOn) {
  int relaySignal = turnOn ? (RELAY_ACTIVE_LOW ? LOW : HIGH)
                           : (RELAY_ACTIVE_LOW ? HIGH : LOW);

  digitalWrite(RELAY_PIN, relaySignal);
  pumpIsOn = turnOn;
}

void printTelemetry(unsigned long nowMs, bool isDry, int moisturePctApprox) {
  Serial.print("{");
  Serial.print("\"uptimeMs\":");
  Serial.print(nowMs);
  Serial.print(",\"sensorDry\":");
  Serial.print(isDry ? 1 : 0);
  Serial.print(",\"moisturePctApprox\":");
  Serial.print(moisturePctApprox);
  Serial.print(",\"pump\":");
  Serial.print(pumpIsOn ? 1 : 0);
  Serial.println("}");
}

void evaluateWateringDecision(unsigned long nowMs, bool isDry) {
  if (pumpIsOn) {
    bool reachedTarget = !isDry;
    bool safetyTimeout = (nowMs - pumpStartedAt) >= MAX_PUMP_RUN_MS;

    if (reachedTarget || safetyTimeout) {
      setPump(false);
      lastPumpStoppedAt = nowMs;

      Serial.print("Pump OFF | reason=");
      Serial.println(reachedTarget ? "soil_wet" : "safety_timeout");
    }
    return;
  }

  bool cooldownComplete = (nowMs - lastPumpStoppedAt) >= PUMP_COOLDOWN_MS;

  if (isDry && cooldownComplete) {
    setPump(true);
    pumpStartedAt = nowMs;
    Serial.println("Pump ON | reason=soil_dry");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  setPump(false); // Keep pump off at boot.

  pinMode(SOIL_SENSOR_DO_PIN, INPUT);

  Serial.println("NodeMCU soil moisture controller started (DO mode)");
  Serial.println("If dry/wet is inverted, set SENSOR_DRY_STATE to LOW");
  Serial.print("SENSOR_DRY_STATE is set to: ");
  Serial.println(SENSOR_DRY_STATE == HIGH ? "HIGH" : "LOW");
  Serial.print("Current pin D1 state: ");
  Serial.println(digitalRead(SOIL_SENSOR_DO_PIN) == HIGH ? "HIGH (dry)" : "LOW (wet)");
}

void loop() {
  unsigned long nowMs = millis();

  if ((nowMs - lastSensorReadAt) < SENSOR_READ_INTERVAL_MS) {
    return;
  }

  lastSensorReadAt = nowMs;

  lastSoilDry = readSoilDryState();
  lastMoisturePctApprox = dryStateToMoisturePercent(lastSoilDry);

  evaluateWateringDecision(nowMs, lastSoilDry);
  printTelemetry(nowMs, lastSoilDry, lastMoisturePctApprox);
}
