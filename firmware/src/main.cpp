#include "Arduino.h"
#include "BME688.h"
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include <vector>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

static AsyncWebServer server(80);
// Create a global JSON document for sensor data
DynamicJsonDocument sensorDataDoc(512);
DynamicJsonDocument configDoc(4096); // For configuration parsing

BME688 sensor;

// Structure to define temperature-time vector
struct TempTimeVector {
  int temperature;
  int duration;
};

// Structure to hold heater profile configuration
struct HeaterProfile {
  String id;
  int timeBase;
  std::vector<TempTimeVector> vectors;  // Vector of temperature and time pairs
};

// Structure to hold duty cycle configuration
struct DutyCycleProfile {
  String id;
  int numberScanningCycles;
  int numberSleepingCycles;
};

// Structure to hold sensor configuration
struct SensorConfig {
  int sensorIndex;
  String heaterProfile;
  String dutyCycleProfile;
};

// Default JSON configuration as a string
const char* defaultConfig = R"({
  "configHeader": {
    "dateCreated": "2025-04-25T21:42:13.628Z",
    "appVersion": "2.0.0",
    "boardType": "board_8",
    "boardMode": "burn_in",
    "boardLayout": "grouped"
  },
  "configBody": {
    "heaterProfiles": [
      {
        "id": "heater_354",
        "timeBase": 140,
        "temperatureTimeVectors": [
          [320, 5],
          [100, 2],
          [100, 10],
          [100, 30],
          [200, 5],
          [200, 5],
          [200, 5],
          [320, 5],
          [320, 5],
          [320, 5]
        ]
      }
    ],
    "dutyCycleProfiles": [
      {
        "id": "duty_5_10",
        "numberScanningCycles": 5,
        "numberSleepingCycles": 10
      }
    ],
    "sensorConfigurations": [
      {
        "sensorIndex": 0,
        "heaterProfile": "heater_354",
        "dutyCycleProfile": "duty_5_10"
      },
      {
        "sensorIndex": 1,
        "heaterProfile": "heater_354",
        "dutyCycleProfile": "duty_5_10"
      },
      {
        "sensorIndex": 2,
        "heaterProfile": "heater_354",
        "dutyCycleProfile": "duty_5_10"
      },
      {
        "sensorIndex": 3,
        "heaterProfile": "heater_354",
        "dutyCycleProfile": "duty_5_10"
      },
      {
        "sensorIndex": 4,
        "heaterProfile": "heater_354",
        "dutyCycleProfile": "duty_5_10"
      },
      {
        "sensorIndex": 5,
        "heaterProfile": "heater_354",
        "dutyCycleProfile": "duty_5_10"
      },
      {
        "sensorIndex": 6,
        "heaterProfile": "heater_354",
        "dutyCycleProfile": "duty_5_10"
      },
      {
        "sensorIndex": 7,
        "heaterProfile": "heater_354",
        "dutyCycleProfile": "duty_5_10"
      }
    ]
  }
})";

// Function to initialize the file system and create the config file if it doesn't exist
bool initFileSystem() {
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return false;
  }
  
  // Check if the config file exists
  if (!SPIFFS.exists("/bmeconfig.json")) {
    Serial.println("Config file not found, creating default config...");
    
    // Create and write the default configuration
    File configFile = SPIFFS.open("/bmeconfig.json", "w");
    if (!configFile) {
      Serial.println("Failed to create config file");
      return false;
    }
    
    configFile.print(defaultConfig);
    configFile.close();
    
    Serial.println("Default config file created successfully");
  } else {
    Serial.println("Config file found");
  }
  
  return true;
}

// Function to load and parse the configuration file
bool loadSensorConfig() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return false;
  }

  // Check if the config file exists
  if (!SPIFFS.exists("/bmeconfig.json")) {
    Serial.println("Config file not found");
    return false;
  }

  // Open the config file
  File configFile = SPIFFS.open("/bmeconfig.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  // Clear and use the global configDoc
  configDoc.clear();

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(configDoc, configFile);
  configFile.close();

  if (error) {
    Serial.print("Failed to parse config file: ");
    Serial.println(error.c_str());
    return false;
  }

  // Apply configuration to the sensor
  Serial.println("Applying sensor configuration...");
  
  // Extract board configuration
  JsonObject configHeader = configDoc["configHeader"];
  String boardType = configHeader["boardType"].as<String>();
  String boardMode = configHeader["boardMode"].as<String>();
  
  Serial.print("Board Type: ");
  Serial.println(boardType);
  Serial.print("Board Mode: ");
  Serial.println(boardMode);
  
  // Extract heater profiles
  HeaterProfile heaterProfile;
  if (configDoc["configBody"]["heaterProfiles"][0]["id"]) {
    heaterProfile.id = configDoc["configBody"]["heaterProfiles"][0]["id"].as<String>();
    heaterProfile.timeBase = configDoc["configBody"]["heaterProfiles"][0]["timeBase"].as<int>();
    
    Serial.print("Heater Profile: ");
    Serial.println(heaterProfile.id);
    Serial.print("Time Base: ");
    Serial.println(heaterProfile.timeBase);
    
    // Extract temperature-time vectors
    JsonArray tempTimeVectors = configDoc["configBody"]["heaterProfiles"][0]["temperatureTimeVectors"];
    for (JsonArray vector : tempTimeVectors) {
      TempTimeVector ttv;
      ttv.temperature = vector[0].as<int>();
      ttv.duration = vector[1].as<int>();
      heaterProfile.vectors.push_back(ttv);
      
      Serial.print("Temperature: ");
      Serial.print(ttv.temperature);
      Serial.print(", Duration: ");
      Serial.println(ttv.duration);
    }
  }

  // Extract duty cycle profile
  DutyCycleProfile dutyCycle;
  if (configDoc["configBody"]["dutyCycleProfiles"][0]["id"]) {
    dutyCycle.id = configDoc["configBody"]["dutyCycleProfiles"][0]["id"].as<String>();
    dutyCycle.numberScanningCycles = configDoc["configBody"]["dutyCycleProfiles"][0]["numberScanningCycles"].as<int>();
    dutyCycle.numberSleepingCycles = configDoc["configBody"]["dutyCycleProfiles"][0]["numberSleepingCycles"].as<int>();
    
    Serial.print("Duty Cycle Profile: ");
    Serial.println(dutyCycle.id);
    Serial.print("Scanning Cycles: ");
    Serial.println(dutyCycle.numberScanningCycles);
    Serial.print("Sleeping Cycles: ");
    Serial.println(dutyCycle.numberSleepingCycles);
  }
  
  // Extract sensor configurations
  for (JsonObject sensorConfig : configDoc["configBody"]["sensorConfigurations"].as<JsonArray>()) {
    int sensorIndex = sensorConfig["sensorIndex"].as<int>();
    String heaterProfileId = sensorConfig["heaterProfile"].as<String>();
    String dutyCycleProfileId = sensorConfig["dutyCycleProfile"].as<String>();
    
    if (sensorIndex == 0) {  // We're only using one sensor in this example
      Serial.print("Sensor Index: ");
      Serial.println(sensorIndex);
      Serial.print("Using Heater Profile: ");
      Serial.println(heaterProfileId);
      Serial.print("Using Duty Cycle Profile: ");
      Serial.println(dutyCycleProfileId);
    }
  }

  Serial.println("Configuration applied successfully");
  return true;
}

// Function to handle updating the configuration
void handleUpdateConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  // If this is the first chunk
  if (index == 0) {
    // Create a buffer to hold all the data
    request->_tempObject = malloc(total);
    if (request->_tempObject == NULL) {
      request->send(500, "text/plain", "Not enough memory");
      return;
    }
  }
  
  // Copy the data to the buffer
  memcpy((uint8_t*)request->_tempObject + index, data, len);
  
  // If this is the last chunk
  if (index + len == total) {
    // Process the complete JSON
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, (uint8_t*)request->_tempObject, total);
    free(request->_tempObject);
    request->_tempObject = NULL;
    
    if (error) {
      request->send(400, "text/plain", "Invalid JSON");
      return;
    }
    
    // Save the JSON to the config file
    File configFile = SPIFFS.open("/bmeconfig.json", "w");
    if (!configFile) {
      request->send(500, "text/plain", "Failed to open config file for writing");
      return;
    }
    
    // Write the JSON to the file
    serializeJson(doc, configFile);
    configFile.close();
    
    // Apply the new configuration
    if (loadSensorConfig()) {
      // Create a response JSON
      DynamicJsonDocument responseDoc(256);
      responseDoc["status"] = "success";
      responseDoc["message"] = "Configuration updated and applied";
      
      String responseStr;
      serializeJson(responseDoc, responseStr);
      
      request->send(200, "application/json", responseStr);
    } else {
      request->send(500, "text/plain", "Failed to apply updated configuration");
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize the sensor
  if (sensor.begin()) {
    Serial.println("BME688 Initialized Successfully!");
    
    // Initialize file system and create config file if needed
    if (initFileSystem()) {
      Serial.println("File system initialized successfully");
      
      // Load and apply configuration
      if (loadSensorConfig()) {
        Serial.println("Sensor configuration loaded successfully");
      } else {
        Serial.println("Failed to load sensor configuration");
      }
    } else {
      Serial.println("Failed to initialize file system");
    }
  } else {
    Serial.println("Failed to initialize BME688!");
  }

  // Setup WiFi Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP("BME688-Sensor");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Route for root / web page - serve a simple HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><head><title>BME688 Sensor</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body { font-family: Arial; margin: 20px; }</style>";
    html += "<script>";
    html += "function updateSensorData() {";
    html += "  fetch('/sensor').then(response => response.json())";
    html += "  .then(data => {";
    html += "    document.getElementById('temp').innerHTML = data.temperature;";
    html += "    document.getElementById('humidity').innerHTML = data.humidity;";
    html += "    document.getElementById('gas').innerHTML = data.gas_resistance;";
    html += "  });";
    html += "  setTimeout(updateSensorData, 3000);"; // Update every 3 seconds
    html += "}";
    html += "document.addEventListener('DOMContentLoaded', updateSensorData);";
    html += "</script></head><body>";
    html += "<h1>BME688 Sensor Dashboard</h1>";
    html += "<p>Temperature: <span id='temp'>-</span> °C</p>";
    html += "<p>Humidity: <span id='humidity'>-</span> %</p>";
    html += "<p>Gas Resistance: <span id='gas'>-</span> Ω</p>";
    html += "<p><a href='/config'>View Configuration</a></p>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Route to get current sensor readings as JSON
  server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    
    // Clear and prepare JSON document
    sensorDataDoc.clear();
    JsonObject root = sensorDataDoc.to<JsonObject>();
    
    // Add sensor data to JSON
    root["temperature"] = sensor.readTemperature();
    root["humidity"] = sensor.readHumidity();
    root["gas_resistance"] = sensor.readGas(0);
    root["timestamp"] = millis();
    
    // Serialize JSON to response
    serializeJson(sensorDataDoc, *response);
    request->send(response);
  });

  // Route to get sensor configuration
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/bmeconfig.json", "application/json");
  });

  // Route to update the configuration
  server.on("/update-config", HTTP_POST, 
    // The first callback is called after the body is received
    [](AsyncWebServerRequest *request) {
      // We don't need to do anything here, the actual handling is done in the body handler
    },
    // The second callback is the upload handler
    NULL,
    // The third callback is the body handler
    handleUpdateConfig
  );

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Just for debugging
  Serial.print("Gas Resistance: ");
  Serial.print(sensor.readGas(0));
  Serial.println(" Ω");
  Serial.print("Temperature: ");
  Serial.print(sensor.readTemperature()); 
  Serial.println(" °C");
  Serial.print("Humidity: ");
  Serial.print(sensor.readHumidity());
  Serial.println(" %");
  
  delay(3000); // Wait 3 seconds before next reading
}