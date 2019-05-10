#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

#define DHTPIN 2     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT11     // DHT 11
#define BLERX       8
#define BLETX       9
#define OUTPUT_GATE           4

DHT_Unified dht(DHTPIN, DHTTYPE);
SoftwareSerial ble(BLERX, BLETX);

uint32_t delayMS = 1000;

typedef struct metric_s {
	int current;
	int maximum;
	int minimum;
} metric_t;

typedef struct measure_s {
	metric_t temp;
	metric_t humid;
} measure_t;

measure_t measure = {0};



void setup() {
	pinMode(OUTPUT_GATE, OUTPUT);
	Serial.begin(9600);
	digitalWrite(OUTPUT_GATE, HIGH);
	ble.begin(9600);
	ble.write(F("AT"));
	Serial.println(ble.read());
	ble.write(F("AT+NAMEFRIGO"));
	Serial.println(ble.read());
	ble.write(F("AT+PIN060818"));
	Serial.println(ble.read());
	dht.begin();
	sensor_t sensor;
	unsigned char *data = (unsigned char *)&measure;
	for(uint32_t i=0; i<sizeof(measure); i++) {
		data[i] = EEPROM.read(i);
	}
}

void refreshmeasure(void)
{
	sensors_event_t event;
	dht.temperature().getEvent(&event);
	if (isnan(event.temperature)) {
		measure.temp.current = -1;
	} else {
		measure.temp.current = event.temperature;
	}
	dht.humidity().getEvent(&event);
	if (isnan(event.relative_humidity)) {
		measure.humid.current = -1;
	} else {
		measure.humid.current = event.relative_humidity;
	}
}

void parsecmd(void)
{
	String command;
	while (ble.available() >0) {
		char c = ble.read();
		command += c;
	}
	if(command.length() == 0) {
		return;
	}
	if(command.startsWith(F("g"))) {
		ble.print(F("t="));
		ble.println(measure.temp.current);
		ble.print(F("h="));
		ble.println(measure.humid.current);
		ble.print(F("tmax="));
		ble.println(measure.temp.maximum);
		ble.print(F("tmin="));
		ble.println(measure.temp.minimum);
		ble.print(F("hmax="));
		ble.println(measure.humid.maximum);
		ble.print(F("hmin="));
		ble.println(measure.humid.minimum);
	} else if (command.startsWith(F("tmin="))) {
		int val = command.substring(5).toInt();
		if(val > 0) {
			measure.temp.minimum = val;
		}
	} else if (command.startsWith(F("tmax="))) {
		int val = command.substring(5).toInt();
		if(val > 0) {
			measure.temp.maximum = val;
		}
	} else if (command.startsWith(F("hmin="))) {
		int val = command.substring(5).toInt();
		if(val > 0) {
			measure.humid.minimum = val;
		}
	} else if (command.startsWith(F("hmax="))) {
		int val = command.substring(5).toInt();
		if(val > 0) {
			measure.humid.maximum = val;
		}
	} else {
		ble.println(F("Unknown cmd: g, tmax=<>, tmin=<>, hmin=<>, hmax=<>"));
	}
	unsigned char *data = (unsigned char *)&measure;
	for(uint32_t i=0; i<sizeof(measure); i++) {
		EEPROM.update(i, data[i]);
	}
}
void computefridge(void)
{
	if(measure.temp.current >= measure.temp.maximum) {
		digitalWrite(OUTPUT_GATE, LOW);
	} else if(measure.temp.current < measure.temp.minimum) {
		digitalWrite(OUTPUT_GATE, HIGH);
	}
}
void loop() {
	delay(delayMS);
	Serial.println(F("loop"));
	refreshmeasure();
	parsecmd();
	computefridge();
}

