#include <SPI.h>
#include <stdint.h>
//#include <BLEPeripheral.h>
#include <bluefruit.h>
#include <Adafruit_GFX.h>
#include <nrf_nvic.h>//interrupt controller stuff
//#include <nrf_sdm.h>
//#include <nrf_soc.h>
#include <WInterrupts.h>
//#include <Adafruit_SSD1306.h>
#include "ds6_SSD1306.h"
#include <TimeLib.h>
#include <nrf.h>
#include <Wire.h>
//  sd_nvic_SystemReset();

//#define wdt_reset() NRF_WDT->RR[0] = WDT_RR_RR_Reload
//#define wdt_enable(timeout) \


//NRF_WDT->CONFIG = NRF_WDT->CONFIG = (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos) | ( WDT_CONFIG_SLEEP_Pause << WDT_CONFIG_SLEEP_Pos); \
//		  NRF_WDT->CRV = (32768*timeout)/1000; \
//		  NRF_WDT->RREN |= WDT_RREN_RR0_Msk;  \
//		  NRF_WDT->TASKS_START = 1

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, PIN_SPI_MOSI, PIN_SPI_SCK, OLED_DC, OLED_RST, OLED_CS);

//Adafruit_SSD1306 display(128, 32, &SPI, 28, 4, 29);

#define BUTTON_PIN              30
#define BATTERY_PIN             3


#define sleepDelay 10000
#define sleepDelay1 50000
#define refreshRate 100

bool fingerin;
int menu;
byte wert[65];
volatile byte posi = 64;
long startbutton;

uint8_t bps = 0;

BLEService        tijd = BLEService(0x6666);
//BLEService        hrms = BLEService(UUID16_SVC_HEART_RATE);
BLECharacteristic hrmc = BLECharacteristic(UUID16_CHR_HEART_RATE_MEASUREMENT);
BLECharacteristic bslc = BLECharacteristic(UUID16_CHR_BODY_SENSOR_LOCATION);

BLECharacteristic uur = BLECharacteristic(0x1234);

BLEDis bledis;    // DIS (Device Information Service) helper class instance
BLEBas blebas;    // BAS (Battery Service) helper class instance

//BLE Client Current Time Service

//BLEClientCts  bleCTime;

volatile bool buttonPressed = false;
volatile int16_t level;
volatile uint8_t batt;

unsigned long sleepTime, displayRefreshTime, rpmTime;
volatile bool sleeping = false;

volatile int before1, before2, before3;
volatile float accX, accY, accZ;
volatile char reson;
volatile bool readacc = true, afeoff, foot;
byte sds;

void powerUp();

void PrintHex8();


void buttonHandler() {
	if (!sleeping) buttonPressed = true;
	powerUp();
}

void charge() {
	powerUp();
}
void acchandler() {
	readacc = true;
}

void updateBatteryLevel() {
	level = analogRead(BATTERY_PIN);
	batt = map(level, 370, 420, 0, 100);
	blebas.write(batt); 
	//	batteryLevelChar.setValue(batt); hier moet bluetoothkoppeling komen jj
}

float reads[4], sum;
long int now1, ptr;
float last, rawRate, start;
float first, second1, third, before, print_value;
bool rising;
int rise_count;
int n;
long int last_beat;
int lastHigh;

void setup() {
	Serial.begin(38400);
	// pinMode(16, OUTPUT);
	// digitalWrite(16, LOW);
	Serial.println("Start");
	pinMode(BUTTON_PIN, INPUT);
	if (digitalRead(BUTTON_PIN) == LOW) {
		//		NRF_POWER->GPREGRET = 0x01;
		//		sd_nvic_SystemReset();
	}
	//	wdt_enable(5000);


	// Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 1
	// SRAM usage required by SoftDevice will increase dramatically with number of connections
	Bluefruit.begin();
	Serial.println("Bluefruit Start");

	Bluefruit.setName("Bluefruit52 Battery");
	// Set the connect/disconnect callback handlers
	Bluefruit.Periph.setConnectCallback(connect_callback);
	Bluefruit.Periph.setDisconnectCallback(disconnect_callback);



	// Configure and Start the Device Information Service
	Serial.println("Configuring the Device Information Service");
	bledis.setManufacturer("Adafruit Industries");
	bledis.setModel("Bluefruit Feather52");
	bledis.begin();

	// Start the BLE Battery Service and set it to 100%
	Serial.println("Configuring the Battery Service");
	blebas.begin();
	blebas.write(100);

	// Setup the Heart Rate Monitor service using
	// BLEService and BLECharacteristic classes
	Serial.println("Configuring the Heart Rate Monitor Service");
	setupHRM();
	setupTIME();

	// Configure CTS client

	//bleCTime.begin();

	// Setup the advertising packet(s)
	Serial.println("Setting up the advertising payload(s)");
	startAdv();

	Serial.println("Ready Player One!!!");
	Serial.println("\nAdvertising");

	//	pinMode(25, OUTPUT); //motor
	//	digitalWrite(25, HIGH);
	pinMode(26, OUTPUT);
	digitalWrite(26, HIGH);
	pinMode(BATTERY_PIN, INPUT);
	//attachInterrupt(digitalPinToInterrupt(22), acchandler, FALLING);
	attachInterrupt(digitalPinToInterrupt(2), charge, RISING);
	NRF_GPIO->PIN_CNF[2] &= ~((uint32_t)GPIO_PIN_CNF_SENSE_Msk);
	NRF_GPIO->PIN_CNF[2] |= ((uint32_t)GPIO_PIN_CNF_SENSE_High << GPIO_PIN_CNF_SENSE_Pos);

	attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonHandler, FALLING);

	delay(100);
	display.begin(SSD1306_SWITCHCAPVCC);
	delay(100);
	display.clearDisplay();
	display.display();

	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(10, 0);
	display.println("Start");
	display.display();
	delay(100);
	digitalWrite(25, LOW);
	Wire.begin();
	delay(100);
	Pah8001_Configure();

	for (int i = 0; i < 4; i++)reads[i] = 0;
	sum = 0;
	ptr = 0;
}










void disableWakeupByInterrupt(uint32_t pin) {
	detachInterrupt(digitalPinToInterrupt(pin));
	NRF_GPIO->PIN_CNF[pin] &= ~((uint32_t)GPIO_PIN_CNF_SENSE_Msk);
	NRF_GPIO->PIN_CNF[pin] |= ((uint32_t)GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
}



void startAdv(void)
{
	// Advertising packet
	Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
	Bluefruit.Advertising.addTxPower();

	// Include HRM Service UUID
	//	Bluefruit.Advertising.addService(hrms);
	Bluefruit.Advertising.addService(tijd);
	// Include CTS client UUID

	//Bluefruit.Advertising.addService(bleCTime);


	// Include Name
	Bluefruit.Advertising.addName();

	/* Start Advertising
	 * - Enable auto advertising if disconnected
	 * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
	 * - Timeout for fast mode is 30 seconds
	 * - Start(timeout) with timeout = 0 will advertise forever (until connected)
	 * 
	 * For recommended advertising interval
	 * https://developer.apple.com/library/content/qa/qa1931/_index.html   
	 */
	Bluefruit.Advertising.restartOnDisconnect(true);
	Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
	Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
	Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

void setupHRM(void)
{
	// Configure the Heart Rate Monitor service
	// See: https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.heart_rate.xml
	// Supported Characteristics:
	// Name                         UUID    Requirement Properties
	// ---------------------------- ------  ----------- ----------
	// Heart Rate Measurement       0x2A37  Mandatory   Notify
	// Body Sensor Location         0x2A38  Optional    Read
	// Heart Rate Control Point     0x2A39  Conditional Write       <-- Not used here
	//	hrms.begin();

	// Note: You must call .begin() on the BLEService before calling .begin() on
	// any characteristic(s) within that service definition.. Calling .begin() on
	// a BLECharacteristic will cause it to be added to the last BLEService that
	// was 'begin()'ed!

	// Configure the Heart Rate Measurement characteristic
	// See: https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.heart_rate_measurement.xml
	// Properties = Notify
	// Min Len    = 1
	// Max Len    = 8
	//    B0      = UINT8  - Flag (MANDATORY)
	//      b5:7  = Reserved
	//      b4    = RR-Internal (0 = Not present, 1 = Present)
	//      b3    = Energy expended status (0 = Not present, 1 = Present)
	//      b1:2  = Sensor contact status (0+1 = Not supported, 2 = Supported but contact not detected, 3 = Supported and detected)
	//      b0    = Value format (0 = UINT8, 1 = UINT16)
	//    B1      = UINT8  - 8-bit heart rate measurement value in BPM
	//    B2:3    = UINT16 - 16-bit heart rate measurement value in BPM
	//    B4:5    = UINT16 - Energy expended in joules
	//    B6:7    = UINT16 - RR Internal (1/1024 second resolution)
	hrmc.setProperties(CHR_PROPS_NOTIFY);
	hrmc.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
	hrmc.setFixedLen(2);
	hrmc.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
	hrmc.begin();
	uint8_t hrmdata[2] = { 0b00000110, 0x40 }; // Set the characteristic to use 8-bit values, with the sensor connected and detected
	hrmc.write(hrmdata, 2);

	// Configure the Body Sensor Location characteristic
	// See: https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.body_sensor_location.xml
	// Properties = Read
	// Min Len    = 1
	// Max Len    = 1
	//    B0      = UINT8 - Body Sensor Location
	//      0     = Other
	//      1     = Chest
	//      2     = Wrist
	//      3     = Finger
	//      4     = Hand
	//      5     = Ear Lobe
	//      6     = Foot
	//      7:255 = Reserved
	bslc.setProperties(CHR_PROPS_READ);
	bslc.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
	bslc.setFixedLen(1);
	bslc.begin();
	bslc.write8(2);    // Set the characteristic to 'Wrist' (2)
}



void setupTIME(void)
{
	tijd.begin();

	uur.setProperties(CHR_PROPS_WRITE);
	//uur.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
	uur.setPermission(SECMODE_OPEN, SECMODE_OPEN);
	uur.setFixedLen(5);
	//	uur.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
	uur.begin();
	//uint8_t uurdata[2] = { 0x02, 0x23 }; // Set the characteristic to use 8-bit values, with the sensor connected and detected
	//uur.write(uurdata, 2);

}


void connect_callback(uint16_t conn_handle)
{
	// Get the reference to current connection
	BLEConnection* connection = Bluefruit.Connection(conn_handle);

	char central_name[32] = { 0 };
	connection->getPeerName(central_name, sizeof(central_name));

	Serial.print("Connected to ");
	Serial.println(central_name);
	/*	if ( bleCTime.discover(conn_handle) )
		{
		bleCTime.enableAdjust();
		Serial.println("receiving time");
		bleCTime.getCurrentTime();
		bleCTime.getLocalTimeInfo();
		Serial.println("weekday");
		Serial.print(bleCTime.Time.weekday);
		}	
		else
		Serial.println("not receiving time");
	 */
}


/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
	(void) conn_handle;
	(void) reason;

	Serial.println("Disconnected");
	Serial.println("Advertising!");
}

void cccd_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint16_t cccd_value)
{
	// Display the raw request packet
	Serial.print("CCCD Updated: ");
	//Serial.printBuffer(request->data, request->len);
	Serial.print(cccd_value);
	Serial.println("");

	// Check the characteristic this CCCD update is associated with in case
	// this handler is used for multiple CCCD records.
	if (chr->uuid == hrmc.uuid) {
		if (chr->notifyEnabled(conn_hdl)) {
			Serial.println("Heart Rate Measurement 'Notify' enabled");
		} else {
			Serial.println("Heart Rate Measurement 'Notify' disabled");
		}
	}
}





uint8_t hrmdata[2] = { 0b00000110, bps++ };           // Sensor connected, increment BPS value
uint8_t hourdata[5] ; // Set the characteristic to use 8-bit values, with the sensor connected and detected

void loop() {
	//	Serial.println("LOOPING");
	if ( Bluefruit.connected() ) {
		//	uint8_t hrmdata[2] = { 0b00000110, print_value };           // Sensor connected, increment BPS value
		uur.read(hourdata, 5);
		if (hourdata[0] != 0)
		{
			Serial.println(hourdata[0]);
			Serial.println(hourdata[1]);
			Serial.println(hourdata[2]);
			Serial.println(hourdata[3]);
			Serial.println(hourdata[4]);
		//	Serial.println("uurdata");
	//		Serial.println(hourdata[1]);
		//	hourdata[2]=1;
		//setTime(14, 27, 00, 14, 12, 2015);
		setTime(hourdata[3], hourdata[4], 00, hourdata[2], hourdata[1], hourdata[0] + 2000);
			hourdata[0]=0;
	uur.write(hourdata, 5);
		}
	}
	// Get Time from iOS once per second
	// Note it is not advised to update this quickly
	// Application should use local clock and update time after 
	// a long period (e.g an hour or day)

	//bleCTime.getCurrentTime();
	//bleCTime.getLocalTimeInfo();


	//	blePeripheral.poll();
	//	wdt_reset();
	if (sleeping) {
		sd_nvic_ClearPendingIRQ(SD_EVT_IRQn);
		sd_power_mode_set(NRF_POWER_MODE_LOWPWR);
		sd_app_evt_wait();
	} else {
		if (millis() - displayRefreshTime > refreshRate) {
			displayRefreshTime = millis();
			switch (menu) {
				case 0:
					displayMenu2();
					break;
				case 1:
					displayMenu1();
					break;
				case 2:
					displayMenu0();
					break;
				case 3:
					displayMenu3();
					break;
				case 4:
					displayMenu4();
					break;
			}
		}

		if (buttonPressed) {
			buttonPressed = false;
			switch (menu) {
				case 0:
					menu = 1;
					break;
				case 1:
					menu = 2;
					break;
				case 2:
					menu = 3;
					break;
				case 3:
					menu = 4;
					break;
				case 4:
					startbutton = millis();
					while (!digitalRead(BUTTON_PIN)) {}
					if (millis() - startbutton > 1000) {
						delay(100);
						int err_code = sd_power_gpregret_set(0,0x01); //jj gpregret_id gpregret_msk
						sd_nvic_SystemReset();
						while (1) {};
						break;
					} else {
						menu = 0;
					}
			}
		}
	}

	uint8_t buffer[13];
	Pah8001_ReadRawData(buffer);
	if (buffer[0]) {
		if (buffer[8] == 128) {
			rawRate = map(buffer[6], 0, 255, 1023, 0);
			//Serial.println("rawrate");
			//Serial.print(rawRate);
			fingerin = true;
		} else fingerin = false;
	}
	if (millis() - rpmTime >= 20) {
		rpmTime = millis();

		sum -= reads[ptr];
		sum += rawRate;
		reads[ptr] = rawRate;
		last = sum / 4;

		if (last > before)
		{
			rise_count++;
			if (!rising && rise_count > 5)
			{
				if (!rising)lastHigh = rawRate;
				rising = true;
				first = millis() - last_beat;
				last_beat = millis();
				print_value = 60000. / (0.4 * first + 0.3 * second1 + 0.3 * third);
				third = second1;
				second1 = first;
			}
		}
		else
		{
			rising = false;
			rise_count = 0;
		}
		before = last;

		ptr++;
		ptr %= 4;

		//Serial.print((int)print_value);
		//Serial.print(",");
		//	Serial.print(rawRate);
		//Serial.println();
	}
}
void PrintHex8(uint32_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
	char tmp[16];
	for (int i = 0; i < length; i++) {
		sprintf(tmp, "%08X", data[i]);
		display.print(tmp); display.println(" ");
	}
}

void displayMenu0() {
	display.clearDisplay();
	display.setCursor(0, 0);
	display.println("Heartrate");
	display.println((int)rawRate);
	if (fingerin) display.print((int)print_value); else display.print("No Finger");

	if (posi <= 63)posi++; else posi = 0;
	wert[posi] = map(rawRate, lastHigh - 30, lastHigh + 10, 0, 32);
	for (int i = 64; i <= 128; i++) {
		display.drawLine( i, 32, i, 32 - wert[i - 64], WHITE);
	}
	display.drawLine( posi + 65, 0, posi + 65, 32, BLACK);
	display.display();
}

void displayMenu1() {
	updateBatteryLevel();
	display.clearDisplay();
	display.setCursor(0, 0);
	display.println("Menue 1");
	char tmp[16];
	sprintf(tmp, "%04X", NRF_FICR->DEVICEADDR[1] & 0xffff);
	String MyID = tmp;
	sprintf(tmp, "%08X", NRF_FICR->DEVICEADDR[0]);
	MyID += tmp;
	display.println(MyID);
	display.display();
}

void displayMenu2() {
	char buffer[11];
	time_t time_now = now();
	sprintf(buffer,"%02d.%02d.%04d",day(time_now),month(time_now),year(time_now));
	display.clearDisplay();
	display.setCursor(0, 0);
	display.println(buffer);
	sprintf(buffer,"%02d:%02d:%02d",hour(time_now),minute(time_now),second(time_now));
	display.println(buffer);
	display.display();
}

void displayMenu3() {
	display.clearDisplay();
	display.setCursor(0, 0);
	display.println("Menue 4");
	display.display();
}
void displayMenu4() {
	display.clearDisplay();
	display.setCursor(0, 0);
	display.println("Hello From Arduino");
	display.println("  :)");
	display.println("Hold for Bootloader");
	display.display();
}


void powerUp() {
	if (sleeping) {
		sleeping = false;
		delay(5);
	}
	sleepTime = millis();
}

void powerDown() {
	if (!sleeping) {
		menu = 0;

		sleeping = true;

		NRF_SAADC ->ENABLE = 0; //disable ADC
		NRF_PWM0  ->ENABLE = 0; //disable all pwm instance
		NRF_PWM1  ->ENABLE = 0;
		NRF_PWM2  ->ENABLE = 0;
	}
}

void writeRegister(uint8_t addr, uint8_t data)
{
	Wire.beginTransmission(0x6b);
	Wire.write(addr);
	Wire.write(data);
	Wire.endTransmission();
}

uint8_t readRegister(uint8_t addr)
{
	Wire.beginTransmission(0x6b);
	Wire.write(addr);
	Wire.endTransmission();
	Wire.requestFrom(0x6b, 1);
	return Wire.read();
}

static const struct {
	uint8_t reg;
	uint8_t value;
} config[] =
{
	{ 0x27u, 0xFFu },
	{ 0x28u, 0xFAu },
	{ 0x29u, 0x0Au },
	{ 0x2Au, 0xC8u },
	{ 0x2Bu, 0xA0u },
	{ 0x2Cu, 0x8Cu },
	{ 0x2Du, 0x64u },
	{ 0x42u, 0x20u },
	{ 0x48u, 0x00u },
	{ 0x4Du, 0x1Au },
	{ 0x7Au, 0xB5u },
	{ 0x7Fu, 0x01u },
	{ 0x07u, 0x48u },
	{ 0x23u, 0x40u },
	{ 0x26u, 0x0Fu },
	{ 0x2Eu, 0x48u },
	{ 0x38u, 0xEAu },
	{ 0x42u, 0xA4u },
	{ 0x43u, 0x41u },
	{ 0x44u, 0x41u },
	{ 0x45u, 0x24u },
	{ 0x46u, 0xC0u },
	{ 0x52u, 0x32u },
	{ 0x53u, 0x28u },
	{ 0x56u, 0x60u },
	{ 0x57u, 0x28u },
	{ 0x6Du, 0x02u },
	{ 0x0Fu, 0xC8u },
	{ 0x7Fu, 0x00u },
	{ 0x5Du, 0x81u }
};

void Pah8001_Configure()
{
	uint8_t value;
	writeRegister(0x06, 0x82);
	delay(10);
	writeRegister(0x09,0x5A);
	writeRegister(0x05,0x99);
	value = readRegister (0x17);
	writeRegister(0x17, value | 0x80 );
	for (size_t i = 0; i < sizeof(config) / sizeof(config[0]); i++)
	{
		writeRegister(config[i].reg, config[i].value);
	}
	value = readRegister(0x00);
}


void Pah8001_ReadRawData(uint8_t buffer[9])
{
	uint8_t tmp[4];
	uint8_t value;
	writeRegister(0x7F, 0x01);

	buffer[0] = readRegister(0x68) & 0xF;

	if (buffer[0] != 0) //0 means no data, 1~15 mean have data
	{
		Wire.beginTransmission(0x6b);
		Wire.write(0x64);
		Wire.endTransmission();
		Wire.requestFrom(0x6b, 4);

		buffer[1] = Wire.read() & 0xFF;
		buffer[2] = Wire.read() & 0xFF;
		buffer[3] = Wire.read() & 0xFF;
		buffer[4] = Wire.read() & 0xFF;

		Wire.beginTransmission(0x6b);
		Wire.write(0x1a);
		Wire.endTransmission();
		Wire.requestFrom(0x6b, 4);

		buffer[5] = Wire.read() & 0xFF;
		buffer[6] = Wire.read() & 0xFF;
		buffer[7] = Wire.read() & 0xFF;

		writeRegister(0x7F, 0x00);
		buffer[8] = readRegister(0x59) & 0x80;
	}
	else
	{
		writeRegister(0x7F, 0x00);
	}
}

void Pah8001_PowerOff(void)
{
	writeRegister(0x7F, 0x00);
	writeRegister(0x06, 0x0A);
}

void Pah8001_PowerOn(void)
{
	writeRegister(0x7F, 0x00);
	writeRegister(0x06, 0x02);
	writeRegister(0x05, 0x99);
}



