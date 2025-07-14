#include <Arduino.h>
#include <NimBLEDevice.h>
#include "NuPacket.hpp"
#include "NuSerial.hpp"
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <SPI.h>
#include "bme68xLibrary.h"

#define NEW_GAS_MEAS (BME68X_GASM_VALID_MSK | BME68X_HEAT_STAB_MSK | BME68X_NEW_DATA_MSK)
#define MEAS_DUR 140

#define BME_SCK 12
#define BME_MISO 13
#define BME_MOSI 11
#define SEN1_CS 45

Bme68x sen1;

#define SEALEVELPRESSURE_HPA (1013.25)

#define DELAYVAL 1000
// Which pin on the Arduino is connected to the NeoPixels?
#define NEOPIXEL_PIN	0 // On Trinket or Gemma, suggest changing this to be not 0.
#define NUMPIXELS	 1 // How many NeoPixels are attached to the Arduino?
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Set BLE device name
#define DEVICE_NAME "NonHumanScent"

void setup()
{
    SPI.begin();
    sen1.begin(SEN1_CS, SPI);

    // Initialize BLE stack and Nordic UART service
    NimBLEDevice::init(DEVICE_NAME);
    NimBLEDevice::getAdvertising()->setName(DEVICE_NAME);
    NuPacket.start();
    NuPacket.send("--Initializing--");

    /*
    if(sen1.checkStatus())
	{
		if (sen1.checkStatus() == BME68X_ERROR)
		{
			NuPacket.send("Sensor error:");
			NuPakcet.send(sen1.statusString());
			return;
		}
		else if (sen1.checkStatus() == BME68X_WARNING)
		{
			NuPacket.send("Sensor Warning:");
			NuPacket.send(sen1.statusString());
		}
	}
	*/

    /* Set the default configuration for temperature, pressure and humidity */
	sen1.setTPH();

	/* Heater temperature in degree Celsius */
	uint16_t tempProf[10] = { 320, 100, 100, 100, 200, 200, 200, 320, 320,
			320 };
	/* Multiplier to the shared heater duration */
	uint16_t mulProf[10] = { 5, 2, 10, 30, 5, 5, 5, 5, 5, 5 };
	/* Shared heating duration in milliseconds */
	uint16_t sharedHeatrDur = MEAS_DUR - (sen1.getMeasDur(BME68X_PARALLEL_MODE) / 1000);

	sen1.setHeaterProf(tempProf, mulProf, sharedHeatrDur, 10);
	sen1.setOpMode(BME68X_PARALLEL_MODE);

        pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
        pixels.clear();

    if (!sen1.begin()) {
	NuPacket.send("Could not find a valid BME680 sensor, check wiring!"); 
     }
    // Set up oversampling and filter initialization
    sen1.setTemperatureOversampling(BME680_OS_8X);
    sen1.setHumidityOversampling(BME680_OS_2X);
    sen1.setPressureOversampling(BME680_OS_4X);
    sen1.setIIRFilterSize(BME680_FILTER_SIZE_3);
    sen1.setGasHeater(320, 150); // 320*C for 150 ms
    NuPacket.send("TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%), Gas resistance(ohm), Status, Gas index");

}

void loop()
{
	if (NuPacket.connect()) {
	sen168xData data;
	uint8_t nFieldsLeft = 0;

	/* data being fetched for every 140ms */
	delay(MEAS_DUR);

	if (sen1.fetchData())
	{
		do
		{
			nFieldsLeft = sen1.getData(data);
			if (data.status == NEW_GAS_MEAS)
			{
				NuPacket.send(std::to_string(millis()));
				NuPacket.send(std::to_string(data.temperature));
				NuPacket.send(std::to_string(data.pressure));
				NuPacket.send(std::to_string(data.humidity));
				NuPacket.send(std::to_string(data.gas_resistance));
				// NuPacket.send(std::to_string(data.status, HEX) + ", ");
				NuPacket.send(std::to_string(data.gas_index);
			}
		} while (nFieldsLeft);
	}
	}

}

/*
if (NuPacket.connect()) {
   pixels.setPixelColor(0, pixels.Color(0, 150, 0));
   pixels.show();
   delay(DELAYVAL);

   int temperature_reading = sen1.temperature;
   NuPacket.send("Temperature = ");
   NuPacket.send(std::to_string(temperature_reading).c_str());
   NuPacket.send(" *C");

   int pressure_reading = sen1.pressure / 100.0;
   NuPacket.send("Pressure = ");
   NuPacket.send(std::to_string(pressure_reading).c_str());
   NuPacket.send(" hPa");

   int humidity_reading = sen1.humidity;
   NuPacket.send("Humidity = ");
   NuPacket.send(std::to_string(humidity_reading).c_str());
   NuPacket.send(" %");

   int gas_reading = sen1.gas_resistance / 1000.0;
   NuPacket.send("Gas = ");
   NuPacket.send(std::to_string(gas_reading).c_str());
   NuPacket.send(" KOhms");

   int altitude_reading = sen1.readAltitude(SEALEVELPRESSURE_HPA);
   NuPacket.send("Approx. Altitude = ");
   NuPacket.send(std::to_string(altitude_reading).c_str());
   NuPacket.send(" m");

   NuPacket.send("\n");
}
else
   {
    pixels.clear();
    pixels.setPixelColor(0, pixels.Color(150, 0, 0));
    pixels.show();
    delay(DELAYVAL);
}
*/
