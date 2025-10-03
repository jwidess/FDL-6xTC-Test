/*
 * Freezer Data Logger (FDL) - 6x Thermocouple Test Mk1
 * 
 * Hardware:
 *   - ESP32-C6-DevKitC-1
 *   - SEN-30003-T (4-channel MAX31855T breakout) - TC0 to TC3
 *   - 2x Adafruit MAX31856 breakouts - TC4 and TC5
 * 
 * Author: Justin W.
 * 
 * Pin Connections:
 * | GPIO # | SPI Line | SEN-30003-T Connections | MAX31856 |
 * | ------ | -------- | ----------------------- | -------- |
 * | GPIO6  | SPI_CLK  | SDK                     | SCK      |
 * | GPIO7  | SPI_D    |                         | SDI      |
 * | GPIO2  | SPI_Q    | SDO                     | SDO      |
 * | GPIO23 | SPICS_0  |                         | CS_0     |
 * | GPIO22 | SPICS_1  |                         | CS_1     |
 * | GPIO21 | SPICS_5  | CS3                     |          |
 * | GPIO20 | SPICS_4  | CS2                     |          |
 * | GPIO19 | SPICS_3  | CS1                     |          |
 * | GPIO18 | SPICS_2  | CS0                     |          |
 */

#include <SPI.h>
#include <Adafruit_MAX31855.h>
#include <Adafruit_MAX31856.h>
#include <Adafruit_NeoPixel.h>

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// SPI Interface Pins (shared by all devices)
#define PIN_SPI_CLK   6   // FSPI_CLK -> SDK/SCK on all devices
#define PIN_SPI_MISO  2   // FSPI_Q -> SDO on all devices
#define PIN_SPI_MOSI  7   // FSPI_D -> SDI on MAX31856 only

// CS Pins
#define PIN_CS0  18   // MAX31855 Channel 0 (SEN-30003-T)
#define PIN_CS1  19   // MAX31855 Channel 1 (SEN-30003-T)
#define PIN_CS2  20   // MAX31855 Channel 2 (SEN-30003-T)
#define PIN_CS3  21   // MAX31855 Channel 3 (SEN-30003-T)
#define PIN_CS4  22   // MAX31856 Channel 4 (Eval Board 0)
#define PIN_CS5  23   // MAX31856 Channel 5 (Eval Board 1)

// Status LED (WS2812B)
#define PIN_LED           8
#define NUM_LEDS          1
#define LED_BRIGHTNESS   30   // 0-255

// ============================================================================
// LED STATUS COLORS
// ============================================================================
#define COLOR_OFF         0x000000
#define COLOR_INIT        0x0000FF  // Blue - Init
#define COLOR_OK          0x00FF00  // Green - All OK
#define COLOR_WARNING     0xFFFF00  // Yellow - Some faults
#define COLOR_ERROR       0xFF0000  // Red - Critical error

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

// MAX31855 TCs (TC0-TC3)
Adafruit_MAX31855 tc0(PIN_SPI_CLK, PIN_CS0, PIN_SPI_MISO);
Adafruit_MAX31855 tc1(PIN_SPI_CLK, PIN_CS1, PIN_SPI_MISO);
Adafruit_MAX31855 tc2(PIN_SPI_CLK, PIN_CS2, PIN_SPI_MISO);
Adafruit_MAX31855 tc3(PIN_SPI_CLK, PIN_CS3, PIN_SPI_MISO);

// MAX31856 TCs (TC4-TC5)
Adafruit_MAX31856 tc4(PIN_CS4, PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_CLK);
Adafruit_MAX31856 tc5(PIN_CS5, PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_CLK);

// Status LED
Adafruit_NeoPixel led(NUM_LEDS, PIN_LED, NEO_GRB + NEO_KHZ800);

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  while (!Serial) delay(1);
  
  // Initialize LED
  led.begin();
  led.setBrightness(LED_BRIGHTNESS);
  setStatusLED(COLOR_INIT);
  
  Serial.println("\n================================");
  Serial.println("FDL 6x TC Test Mk1");
  Serial.println("================================");
  
  // Wait for sensors to stabilize
  delay(500);
  
  // Initialize MAX31855 sensors (TC0-TC3)
  Serial.println("\nInitializing MAX31855 sensors...");
  if (!initMAX31855Sensors()) {
    Serial.println("\n*** FATAL ERROR: MAX31855 initialization failed! ***");
    setStatusLED(COLOR_ERROR);
    while (1) delay(10);  // Halt
  }
  
  // Initialize MAX31856 sensors (TC4-TC5)
  Serial.println("\nInitializing MAX31856 sensors...");
  if (!initMAX31856Sensors()) {
    Serial.println("\n*** FATAL ERROR: MAX31856 initialization failed! ***");
    setStatusLED(COLOR_ERROR);
    while (1) delay(10);  // Halt
  }
  
  Serial.println("\n*** All sensors initialized successfully! ***");
  Serial.println("================================\n");
  setStatusLED(COLOR_OK);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  bool anyFaults = false;
  bool criticalError = false;  // Flag for severe errors
  
  Serial.println("\n========== Thermocouple Readings ==========");
  
  // Read MAX31855 channels (TC0-TC3)
  Serial.println("\nMAX31855 Channels:");
  anyFaults |= readMAX31855(tc0, 0, criticalError);
  anyFaults |= readMAX31855(tc1, 1, criticalError);
  anyFaults |= readMAX31855(tc2, 2, criticalError);
  anyFaults |= readMAX31855(tc3, 3, criticalError);
  
  // Read MAX31856 channels (TC4-TC5)
  Serial.println("\nMAX31856 Channels:");
  anyFaults |= readMAX31856(tc4, 4, criticalError);
  anyFaults |= readMAX31856(tc5, 5, criticalError);
  
  Serial.println("===========================================\n");
  
  // Update LED color based on error
  if (criticalError) {
    setStatusLED(COLOR_ERROR);  // Red for critical errors
  } else if (anyFaults) {
    setStatusLED(COLOR_WARNING);  // Yellow for normal faults (disconnected TCs, etc.)
  } else {
    setStatusLED(COLOR_OK);
  }
  
  delay(1000);
}

// ============================================================================
// INIT FUNCTIONS
// ============================================================================

bool initMAX31855Sensors() {
  bool allOK = true;
  
  if (!tc0.begin()) {
    Serial.println(" TC0 (CS=GPIO18) **FAILED**");
    allOK = false;
  } else {
    Serial.println(" TC0 (CS=GPIO18) OK");
  }
  
  if (!tc1.begin()) {
    Serial.println(" TC1 (CS=GPIO19) **FAILED**");
    allOK = false;
  } else {
    Serial.println(" TC1 (CS=GPIO19) OK");
  }
  
  if (!tc2.begin()) {
    Serial.println(" TC2 (CS=GPIO20) **FAILED**");
    allOK = false;
  } else {
    Serial.println(" TC2 (CS=GPIO20) OK");
  }
  
  if (!tc3.begin()) {
    Serial.println(" TC3 (CS=GPIO21) **FAILED**");
    allOK = false;
  } else {
    Serial.println(" TC3 (CS=GPIO21) OK");
  }
  
  return allOK;
}

bool initMAX31856Sensors() {
  bool allOK = true;
  
  if (!tc4.begin()) {
    Serial.println(" TC4 (CS=GPIO22) **FAILED**");
    allOK = false;
  } else {
    Serial.println(" TC4 (CS=GPIO22) OK");
    tc4.setThermocoupleType(MAX31856_TCTYPE_T);
    tc4.setConversionMode(MAX31856_CONTINUOUS);
  }
  
  if (!tc5.begin()) {
    Serial.println(" TC5 (CS=GPIO23) **FAILED**");
    allOK = false;
  } else {
    Serial.println(" TC5 (CS=GPIO23) OK");
    tc5.setThermocoupleType(MAX31856_TCTYPE_T);
    tc5.setConversionMode(MAX31856_CONTINUOUS);
  }
  
  return allOK;
}

// ============================================================================
// READING FUNCTIONS
// ============================================================================

bool readMAX31855(Adafruit_MAX31855 &tc, int channel, bool &criticalError) {
  bool hasFault = false;
  double tempC = tc.readCelsius();
  
  Serial.print("  TC");
  Serial.print(channel);
  Serial.print(": ");
  
  if (isnan(tempC)) {
    Serial.println("ERROR - Fault detected!");
    hasFault = true;
    
    uint8_t fault = tc.readError();
    int faultCount = 0;
    
    if (fault & MAX31855_FAULT_OPEN) {
      Serial.println("       → Open circuit (no thermocouple connected)");
      faultCount++;
    }
    if (fault & MAX31855_FAULT_SHORT_GND) {
      Serial.println("       → Short to ground");
      faultCount++;
    }
    if (fault & MAX31855_FAULT_SHORT_VCC) {
      Serial.println("       → Short to VCC");
      faultCount++;
    }
    
    // Multiple simultaneous faults
    if (faultCount > 1) {
      Serial.println("       CRITICAL: Multiple faults!");
      criticalError = true;
    }
  } else {
    Serial.print(tempC, 2);
    Serial.println(" °C");
  }
  
  return hasFault;
}

bool readMAX31856(Adafruit_MAX31856 &tc, int channel, bool &criticalError) {
  bool hasFault = false;
  
  Serial.print("  TC");
  Serial.print(channel);
  Serial.print(": ");
  
  // Check faults first
  uint8_t fault = tc.readFault();
  if (fault) {
    hasFault = true;
    Serial.println("ERROR - Fault detected!");
    Serial.print("       FAULT (0x");
    Serial.print(fault, HEX);
    Serial.println("):");
    
    int faultCount = 0;
    
    if (fault & MAX31856_FAULT_OPEN) {
      Serial.println("       → Open circuit");
      faultCount++;
    }
    if (fault & MAX31856_FAULT_OVUV) {
      Serial.println("       → Over/under voltage");
      faultCount++;
    }
    if (fault & MAX31856_FAULT_TCLOW) {
      Serial.println("       → TC temperature low");
      faultCount++;
    }
    if (fault & MAX31856_FAULT_TCHIGH) {
      Serial.println("       → TC temperature high");
      faultCount++;
    }
    if (fault & MAX31856_FAULT_CJLOW) {
      Serial.println("       → Cold junction low");
      faultCount++;
    }
    if (fault & MAX31856_FAULT_CJHIGH) {
      Serial.println("       → Cold junction high");
      faultCount++;
    }
    if (fault & MAX31856_FAULT_TCRANGE) {
      Serial.println("       → TC out of range");
      faultCount++;
    }
    if (fault & MAX31856_FAULT_CJRANGE) {
      Serial.println("       → CJ out of range");
      faultCount++;
    }
    
    // Multiple faults (0xFF = all faults) typically indicate wiring/CS issue
    // Normal disconnected TC shows 0x41 (Open + TC out of range = 2 faults)
    // Wiring/CS problem shows 0xFF (all 8 faults)
	// Not the best way of doing this, but it works :)
    if (faultCount > 2 || fault == 0xFF) {
      Serial.println("       CRITICAL: Multiple faults!");
      criticalError = true;
    }
  } else {
    // Only read and display temp if no faults
    float tempC = tc.readThermocoupleTemperature();
    Serial.print(tempC, 2);
    Serial.println(" °C");
  }
  
  return hasFault;
}

// LED Function
void setStatusLED(uint32_t color) {
  led.setPixelColor(0, color);
  led.show();
}