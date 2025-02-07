/* DHT library 
 *
 * MIT license
 * written by Adafruit Industries
 * modified for Spark Core by RussGrue
 * */

#include "Adafruit_DHT/Adafruit_DHT.h"

DHT::DHT(uint8_t pin, uint8_t type, uint8_t count) {
	_pin = pin;
	_type = type;
	_count = count;
	firstreading = true;
}

void DHT::begin(void) {
// set up the pins!
	pinMode(_pin, INPUT);
	digitalWrite(_pin, HIGH);
	_lastreadtime = 0;
}

float DHT::readTemperature() {
	float f;

	if (read()) {
		switch (_type) {
			case DHT11:
				f = data[2];
				return f;
			case DHT22:
			case DHT21:
				f = data[2] & 0x7F;
				f *= 256;
				f += data[3];
				f /= 10;
				if (data[2] & 0x80)
					f *= -1;
				return f;
		}
	}
	return NAN;
}

float DHT::getHumidity() {
	return readHumidity();
}

float DHT::getTempCelcius() {
	return readTemperature();
}

float DHT::getTempFarenheit() {
	return convertCtoF(readTemperature());
}

float DHT::getTempKelvin() {
	return convertCtoK(readTemperature());
}

float DHT::getHeatIndex() {
	return convertFtoC(computeHeatIndex(convertCtoF(readTemperature()), readHumidity()));
}

float DHT::getDewPoint() {
	return computeDewPoint(readTemperature(), readHumidity());
}

float DHT::convertFtoC(float f) {
	return (f - 32) * 5 / 9;
}

float DHT::convertCtoF(float c) {
	return c * 9 / 5 + 32;
}

float DHT::convertCtoK(float c) {
	return c + 273.15;
}

float DHT::readHumidity(void) {
	float f;

	if (read()) {
		switch (_type) {
			case DHT11:
				f = data[0];
				return f;
			case DHT22:
			case DHT21:
				f = data[0];
				f *= 256;
				f += data[1];
				f /= 10;
				return f;
		}
	}
	return NAN;
}

float DHT::computeHeatIndex(float tempFahrenheit, float percentHumidity) {
// Adapted from equation at: https://github.com/adafruit/DHT-sensor-library/issues/9 and
// Wikipedia: http://en.wikipedia.org/wiki/Heat_index
	return -42.379 + 
		 2.04901523 * tempFahrenheit + 
		10.14333127 * percentHumidity +
		-0.22475541 * tempFahrenheit * percentHumidity +
		-0.00683783 * pow(tempFahrenheit, 2) +
		-0.05481717 * pow(percentHumidity, 2) + 
		 0.00122874 * pow(tempFahrenheit, 2) * percentHumidity + 
		 0.00085282 * tempFahrenheit * pow(percentHumidity, 2) +
		-0.00000199 * pow(tempFahrenheit, 2) * pow(percentHumidity, 2);
}

float DHT::computeDewPoint(float tempCelcius, float percentHumidity) {
	double a = 17.271;
	double b = 237.7;
	double tC = (a * (float) tempCelcius) / (b + (float) tempCelcius) + log( (float) percentHumidity / 100);
	double Td = (b * tC) / (a - tC);
	return Td;
}


boolean DHT::read(void) {
	uint8_t laststate = HIGH;
	uint8_t counter = 0;
	uint8_t j = 0, i;
	unsigned long currenttime;

// Check if sensor was read less than two seconds ago and return early
// to use last reading.
	currenttime = millis();
	if (currenttime < _lastreadtime) {
// ie there was a rollover
		_lastreadtime = 0;
	}
	if (!firstreading && ((currenttime - _lastreadtime) < 2000)) {
		return true; // return last correct measurement
//		delay(2000 - (currenttime - _lastreadtime));
	}
	firstreading = false;
/*
	Serial.print("Currtime: "); Serial.print(currenttime);
	Serial.print(" Lasttime: "); Serial.print(_lastreadtime);
*/
	_lastreadtime = millis();

	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
  
// pull the pin high and wait 250 milliseconds
	digitalWrite(_pin, HIGH);
	delay(250);

// now pull it low for ~20 milliseconds
	pinMode(_pin, OUTPUT);
	digitalWrite(_pin, LOW);
	delay(20);
	noInterrupts();
	digitalWrite(_pin, HIGH);
	delayMicroseconds(40);
	pinMode(_pin, INPUT);

// read in timings
	for ( i=0; i< MAXTIMINGS; i++) {
		counter = 0;
		while (digitalRead(_pin) == laststate) {
			counter++;
			delayMicroseconds(1);
			if (counter == 255) {
				break;
			}
		}
		laststate = digitalRead(_pin);

		if (counter == 255) break;

// ignore first 3 transitions
		if ((i >= 4) && (i%2 == 0)) {
// shove each bit into the storage bytes
			data[j/8] <<= 1;
			if (counter > _count)
				data[j/8] |= 1;
			j++;
		}

	}

	interrupts();

/*
	Serial.println(j, DEC);
	Serial.print(data[0], HEX); Serial.print(", ");
	Serial.print(data[1], HEX); Serial.print(", ");
	Serial.print(data[2], HEX); Serial.print(", ");
	Serial.print(data[3], HEX); Serial.print(", ");
	Serial.print(data[4], HEX); Serial.print(" =? ");
	Serial.println(data[0] + data[1] + data[2] + data[3], HEX);
*/

// check we read 40 bits and that the checksum matches
	if ((j >= 40) && 
	   (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) ) {
		return true;
	}
 
	return false;

}
