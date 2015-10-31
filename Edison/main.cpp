/* Program for detecting the air quality and sending data to Parse/IoT Analytics with Intel Edison
Copyright Appollution.
*/

#include "grove.h"
#include "jhd1313m1.h"
#include "buzzer.h"

#include <climits>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <curl/curl.h>
using namespace std;

const string url = "https://api.parse.com/1/classes/Sensors/mWElcljqTJ";
const string ApplicationId = "X-Parse-Application-Id: NkLzqvxNk1AA4Pe3OF9Wz9s50SPFrACHsCrICF7z";
const string ClientId = "X-Parse-REST-API-Key: nHQzbI8QUkmpTgjCu4sBxFl3iPsiSl7b849ZoFQh";

string int_to_string(int i)
{
    stringstream ss;
    ss << i;
    return ss.str();
}

void updateDataToParse(int temperatureValue) {
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		struct curl_slist *headers=NULL;
		headers = curl_slist_append(headers, ApplicationId.c_str());
		headers = curl_slist_append(headers, ClientId.c_str());
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		string temp = int_to_string(temperatureValue) + "º";
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
		string json =  "{\"tempValue\":\""+ temp +"\"}";
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

		res = curl_easy_perform(curl);
			if(res != CURLE_OK)
				cout << "Sending measures failed " << endl;

	}
	curl_easy_cleanup(curl);
}

/*
  Update the temperature and gas values and reflect the changes on the LCD
  - change LCD backlight color based on the measured temperature,
    a cooler color for low temperatures, a warmer one for high temperatures
  - display current temperature
  - record and display MIN and MAX temperatures
  - reset MIN and MAX values if the button is being pushed
  - blink the led to show the temperature was measured and data updated
 */
void update(upm::GroveTemp* temperature_sensor, upm::GroveButton* button,
		upm::GroveLed* led_green, upm::Jhd1313m1 *lcd, upm::GroveLed* led_blue,
		upm::GroveLed* led_red) {
	// minimum and maximum temperatures registered, the initial values will be
	// replaced after the first read
	static int min_temperature = INT_MAX;
	static int max_temperature = INT_MIN;

	// the temperature range in degrees Celsius,
	// adapt to your room temperature for a nicer effect!
	const int TEMPERATURE_RANGE_MIN_VAL = 20;
	const int TEMPERATURE_RANGE_MAX_VAL = 27;

	// other helper variables
	int temperature; // temperature sensor value in degrees Celsius
	//int gasValue;
	float fade; // fade value [0.0 .. 1.0]
	uint8_t r, g, b; // resulting LCD backlight color components [0 .. 255]
	std::stringstream row_1, row_2; // LCD rows

	// update the min and max temperature values, reset them if the button is
	// being pushed
	//gasValue = gas_sensor->getSample();
	temperature = temperature_sensor->value();
	if (button->value() == 1) {
		min_temperature = temperature;
		max_temperature = temperature;
	} else {
		if (temperature < min_temperature) {
			min_temperature = temperature;
		}
		if (temperature > max_temperature) {
			max_temperature = temperature;
		}
	}

	// display the temperature values on the LCD
	row_1 << "Temp " << temperature << "    ";
	row_2 << "Min " << min_temperature << " Max " << max_temperature << "    ";
	lcd->setCursor(0,0);
	lcd->write(row_1.str());
	lcd->setCursor(1,0);
	lcd->write(row_2.str());

	// set the fade value depending on where we are in the temperature range
	if (temperature <= TEMPERATURE_RANGE_MIN_VAL) {
		fade = 0.0;
	} else if (temperature >= TEMPERATURE_RANGE_MAX_VAL) {
		fade = 1.0;
	} else {
		fade = (float)(temperature - TEMPERATURE_RANGE_MIN_VAL) /
				(TEMPERATURE_RANGE_MAX_VAL - TEMPERATURE_RANGE_MIN_VAL);
	}

	// fade the color components separately
	r = (int)(255 * fade);
	g = (int)(64 * fade);
	b = (int)(255 * (1 - fade));

	led_green->off();
	led_blue->off();
	led_red->off();

	if (temperature > TEMPERATURE_RANGE_MAX_VAL)
		led_red->on();
	else if (temperature < TEMPERATURE_RANGE_MIN_VAL)
		led_blue->on();
	else
		led_green->on();

	// apply the calculated result
	lcd->setColor(r, g, b);
	updateDataToParse(temperature);
}

int main() {

	// Set up Parse/IoT Analytics/Fiware



	// check that we are running on Galileo or Edison
	mraa_platform_t platform = mraa_get_platform_type();
	if ((platform != MRAA_INTEL_GALILEO_GEN1) &&
			(platform != MRAA_INTEL_GALILEO_GEN2) &&
			(platform != MRAA_INTEL_EDISON_FAB_C)) {
		std::cerr << "Unsupported platform, exiting" << std::endl;
		return MRAA_ERROR_INVALID_PLATFORM;
	}

	// gas sensor connected to A1(analog in)
	//upm::MQ2* gas_sensor = new upm::MQ2(1);

	// button connected to D4 (digital in)
	upm::GroveButton* button = new upm::GroveButton(4);

	// led green connected to D3 (digital out)
	upm::GroveLed* led_green = new upm::GroveLed(3);

	upm::GroveLed* led_blue = new upm::GroveLed(5);

	upm::GroveLed* led_red = new upm::GroveLed(6);

	// temperature sensor connected to A0 (analog in)
	upm::GroveTemp* temp_sensor = new upm::GroveTemp(0);

	// LCD connected to the default I2C bus
	upm::Jhd1313m1* lcd = new upm::Jhd1313m1(6, 0x3E, 0x62);

	// check if all items are created correctly
	if ((button == NULL) or (led_green == NULL) or (temp_sensor == NULL)
		or (lcd == NULL) or (led_blue == NULL) or (led_red == NULL)) {
		std::cerr << "Can't create all objects, exiting" << std::endl;
		return MRAA_ERROR_UNSPECIFIED;
	}

	// loop forever updating the temperature and gas values every 5 seconds
	for (;;) {
		update(temp_sensor, button, led_green, lcd, led_blue, led_red);
		sleep(30);
	}

	return MRAA_SUCCESS;
}
