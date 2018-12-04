//Version 2.6 -- 04/12/2018
//2.6 - Added SQL to remote display
//2.5 - Added No APRS timeout after PTT
//2.4 - Added turn off Pre/de-emphasis, Highpass, Lowpass filter
//2.3 - Inbouw smart beaconing en Mice by Frank CNO

#include <RDKAPRS.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <TimerOne.h>
#include <Wire.h>
#include <EEPROM.h>
#include <LiquidCrystal_SSD1306.h>

#define button  2   //button to transmit package
#define PTT     3   //PTT pin. This is active low.
#define MicPwr  4   //Power for microphone
#define ledPin  16  //=A2 led for GPS fix
#define bfPin   5   //afsk output must be PD5
#define sqlPin  6   //Squelsh pin
#define PTTPin  7   //PTT switch pin
#define rxPin   8   //rx pin into TX GPS connection
#define txPin   9   //tx pin into RX GPS connection

#define offsetEEPROM 0x0    //offset config

TinyGPSPlus gps;
SoftwareSerial gps_dra(rxPin, txPin);  // RX from GPS (blue), TX to DRA (pin 8)

#define OLED_RESET 0
LiquidCrystal_SSD1306 lcd(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS, OLED_RESET);

long lastUpdate = 0;
int Updatedelay = 600; //delay between packets in seconds.
bool validGPS = 1;
bool pttPressed = 0;
bool lastPttPressed = 0;
bool isInverted = 0;
long NoAPRSAfterPTTTime = 0;
int pttOffValue = 0;

long startTimeOutMillis = 0;
long currentTimeOutMillis = 0;
long timeOutTimerMillis = 0;
bool buttonPressed = 0;
char receivedString[28];
char chkGS[3] = "GS";
static char conv_buf[16];

int SB = 0;
unsigned long old_course;
unsigned long sbCourse;
unsigned long sbStart = 0;


struct StoreStruct {
	byte chkDigit;
	byte aprsChannel;
	byte rxChannel;
	byte txChannel;
	byte rxTone;
	byte txTone;
	char dest[8];
	byte dest_ssid;
	char call[8];
	byte ssid;
	char comment[16];
	byte symbool;  // = auto.
	char path1[8];
	byte path1_ssid;
	char path2[8];
	byte path2_ssid;
	byte interval;
	byte multiplier;
	byte power;
	byte height;
	byte gain;
	byte directivity;
	byte preAmble;
	byte tail;
	byte doTX;
	byte BcnAfterTX;
	byte NoAPRSAfterPTT;
	byte txTimeOut;
	byte isDebug;
};

StoreStruct storage = {
		"#",
		64,
		140,
		92,
		8,
		8,
		"APZRAZ",
		0,
		"PA2RDK",
		7,
		"73 de Robert",
		'>',
		"WIDE1",
		1,
		"WIDE2",
		2,
		30,
		20,
		5,
		0,
		0,
		0,
		90,
		10,
		1,
		0,
		120,
		120,
		0
};



void loop() {
	float flat, flon;
	unsigned long age;
	unsigned long gps_speed;
	unsigned int FlexibleDelay = Updatedelay;
	unsigned long gps_kspeed;
	unsigned long gps_course;

	printInt(gps.satellites.value(), gps.satellites.isValid(), 5, 0);
	printInt(gps.hdop.value(), gps.hdop.isValid(), 5, 0);

	flat = gps.location.lat();
	flon = gps.location.lng();
	age = gps.location.age();
	gps_speed = gps.speed.kmph();
	gps_kspeed = gps.speed.knots();
	gps_course = gps.course.deg();

	if (!gps.location.isValid()){
		if (storage.isDebug == 1) {
			flat = 52.074066;
			flon = 4.504167;
			//flat = 46.116744;
			//flon = 14.547144;
		}
	}
	printFloat(flat, gps.location.isValid(), 11, 6, 0);
	printFloat(flon, gps.location.isValid(), 12, 6, 0);

	printInt(age, gps.location.isValid(), 5, 0);
	printDateTime(gps.date, gps.time, 0);
	printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2, 0);
	printFloat(gps.course.deg(), gps.course.isValid(), 7, 2, 0);
	printFloat(gps_speed, gps.speed.isValid(), 6, 2, 0);
	printStr(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.value()) : "*** ", 6);

	if ((gps_speed > 0) && (gps_speed < 20000)) {
		FlexibleDelay = Updatedelay / gps_speed ;
		if (FlexibleDelay < storage.interval) FlexibleDelay = storage.interval;
	} else {
		FlexibleDelay = Updatedelay;
	}

	Serial.print(FlexibleDelay);

	// Smart Beaconing
	if ((millis() - sbStart > 5000) && (gps_speed > 5)) {
		sbCourse = (abs(gps_course - old_course));
		if (sbCourse > 180) sbCourse = 360 - sbCourse;
		if (sbCourse > 27) SB = 1;
		sbStart = millis();
		old_course = gps_course;
	}

	if (NoAPRSAfterPTTTime>0 && millis()>NoAPRSAfterPTTTime){
		NoAPRSAfterPTTTime=0;
		Serial.println();
		Serial.println(F("Re-enable APRS after PTT"));
	}

	if (((!gps.location.isValid()) || (age > 3000)) && (storage.isDebug == 0)) {
		Serial.print(F(" Invalid position"));
		validGPS = 0;
	} else {
		validGPS = 1;
		if (lastPttPressed == 0 && NoAPRSAfterPTTTime == 0)
			if ((buttonPressed == 1) || ((millis() - lastUpdate) / 1000 > FlexibleDelay) || (SB == 1))
			{
				lastUpdate = millis();
				showDisplay(flat, flon, 1);
				printRemote(flat, flon, 1);
				Serial.println("");
				Serial.print(F("Send beacon:"));
				setDra(storage.aprsChannel, storage.aprsChannel, 0, 0);
				delay(100);
				locationUpdate(flat, flon, gps_kspeed, gps_course);
				delay(500);
				setDra(storage.rxChannel, storage.txChannel, storage.rxTone, storage.txTone);
				if (SB == 1) SB = 0;
			}
	}
	buttonPressed = 0;
	showDisplay(flat, flon, 0);
	printRemote(flat, flon, 0);

	smartDelay(2000);
	if (lastPttPressed != pttPressed) {
		Serial.print(F(" PTT Swapped"));
		lastPttPressed = pttPressed;
		if (lastPttPressed == 1) {
			startTimeOutMillis = millis();
			digitalWrite(PTT, HIGH);                           //PTT on
			digitalWrite(MicPwr, HIGH);                        //Mike on
			digitalWrite(ledPin, HIGH);
		}
		else
		{
			digitalWrite(PTT, LOW);                            //PTT off
			digitalWrite(MicPwr, LOW);                         //Mike off
			digitalWrite(ledPin, LOW);
			if (storage.BcnAfterTX==1) buttonPressed = 1;
			if (storage.NoAPRSAfterPTT>0){
				NoAPRSAfterPTTTime=millis()+long(storage.NoAPRSAfterPTT)*1000;
				buttonPressed=0;
				Serial.println();
				Serial.print(F("Disable APRS after PTT until:"));
				Serial.println(NoAPRSAfterPTTTime);
			}
		}
	}
	currentTimeOutMillis = millis();
	if (((currentTimeOutMillis - startTimeOutMillis) > timeOutTimerMillis) && (storage.txTimeOut > 0) && (lastPttPressed == 1)) {
		isInverted = !isInverted;
		invertLCD(isInverted);
		digitalWrite(PTT, LOW);                            //PTT off
		digitalWrite(MicPwr, LOW);                         //Mike off
		digitalWrite(ledPin, LOW);
	} else invertLCD(false);

	Serial.print(F(" ")); Serial.print(analogRead(0));
	if (lastPttPressed == 1) Serial.println(F(" TX")); else Serial.println(F(" RX"));
}

void printRemote(float flat, float flon, bool isTX){
	Wire.beginTransmission(9);
	int error = Wire.endTransmission();
	if (error == 0)
	{
		if (isTX==0){
			Wire.requestFrom(9,5,1);
			int i = 0;
			bool reWrite = 0;
			char freqString[6];
			while (Wire.available()){
				freqString[i] = Wire.read();
				i++;
			}
			freqString[i]=0;
			if (freqString[0] == '#'){

				if (storage.txChannel != (byte)freqString[1]){
					storage.txChannel = (byte)freqString[1];
					reWrite = 1;
				}
				if (storage.rxChannel != (byte)freqString[2]){
					storage.rxChannel = (byte)freqString[2];
					reWrite = 1;
				}
				if (storage.txTone != (byte)freqString[3]){
					storage.txTone = (byte)freqString[3];
					reWrite = 1;
				}

				if (storage.rxTone != (byte)freqString[4]){
					storage.rxTone = (byte)freqString[4];
					reWrite = 1;
				}
				if (reWrite==1){
					setDra(storage.rxChannel, storage.txChannel, storage.rxTone, storage.txTone);
					reWrite=0;
				}
			}
		}
	}
	Wire.beginTransmission(9);
	Wire.print(F("#"));
	Wire.write(storage.aprsChannel);
	Wire.write(storage.txChannel);
	Wire.write(storage.rxChannel);
	Wire.write(storage.txTone);
	Wire.write(storage.rxTone);
	Wire.write(byte(gps.time.hour()));
	Wire.write(byte(gps.time.minute()));
	Wire.write(byte(gps.time.second()));
	Wire.write(byte(gps.satellites.value()));
	byte p1 = flat;
	float f1 = float(flat-p1)*100;
	byte p2 = f1;
	byte p3 = float(f1-p2)*100;
	Wire.write(p1);
	Wire.write(p2);
	Wire.write(p3);
	p1 = flon;
	f1 = float(flon-p1)*100;
	p2 = f1;
	p3 = float(f1-p2)*100;
	Wire.write(p1);
	Wire.write(p2);
	Wire.write(p3);
	if (isTX==1) {
		Wire.write(3);	//Beacon
	}
	else if (lastPttPressed == 1) {
		Wire.write(2);	//TX
	}
	else {
		Wire.write(1);	//RX
	}
	long temp=gps.altitude.meters()*100;
	if (!gps.altitude.isValid()){
		if (storage.isDebug == 1) {
			temp=123456;
		}
	}

	byte tempbuffer[4];
	tempbuffer[0] = byte(temp >> 24);
	tempbuffer[1] = byte(temp >> 16);
	tempbuffer[2] = byte(temp >> 8);
	tempbuffer[3] = byte(temp);

	Wire.write(tempbuffer[0]);
	Wire.write(tempbuffer[1]);
	Wire.write(tempbuffer[2]);
	Wire.write(tempbuffer[3]);
	Wire.write(byte(storage.ssid));
	Wire.write(byte(storage.symbool));
	Wire.write("#");
	Wire.write(byte(digitalRead(sqlPin)));
	Wire.print(storage.call);
	Wire.endTransmission();
}

void showDisplay(float flat, float flon, bool isTX) {
	if (lastPttPressed == 1) {
		lcd.setCursor(0, 0);
		setBigSize(true);
		setLCDReverse(true);
		lcd.print(F("  "));
		lcd.print(storage.call);
		lcd.print(F("    "));
		setBigSize(false);
		setLCDReverse(false);
		lcd.setCursor(0, 2);
		lcd.print(F("RX:"));
		long frx = 40000+(storage.rxChannel*125);
		frx=(frx/10)+140000;
		sprintf(conv_buf,"%03d.%03d",int(frx/1000),frx-((int(frx/1000))*1000));
		lcd.print(conv_buf);
		lcd.print(F(" / ")); lcd.print(storage.rxTone); lcd.print(F("         "));
		lcd.setCursor(0, 3);
		lcd.print(F("TX:"));
		frx = 40000+(storage.txChannel*125);
		frx=(frx/10)+140000;
		sprintf(conv_buf,"%03d.%03d",int(frx/1000),frx-((int(frx/1000))*1000));
		lcd.print(conv_buf);
		lcd.print(F(" / ")); lcd.print(storage.txTone); lcd.print(F("         "));
	}
	else
	{
		lcd.setCursor(0, 0);
		setLCDReverse(1);
		lcd.print(F(" Tme:"));
		setLCDReverse(0);
		printDateTime(gps.date, gps.time, 1);
		lcd.print(F("       "));
		lcd.setCursor(18, 0);
		if (isTX == 1) lcd.print(F("TX")); else lcd.print(F("--"));
		lcd.setCursor(0, 1);
		setLCDReverse(1);
		lcd.print(F(" Lat:"));
		setLCDReverse(0);
		if (validGPS) printFloat(flat, gps.location.isValid(), 11, 6, 1); else lcd.print(F("********"));
		lcd.print(F("       "));
		lcd.setCursor(0, 2);
		setLCDReverse(1);
		lcd.print(F(" Lon:"));
		setLCDReverse(0);
		if (validGPS) printFloat(flon, gps.location.isValid(), 12, 6, 1); else lcd.print(F("********"));
		lcd.print(F("       "));
		lcd.setCursor(14, 2);
		setLCDReverse(1);
		lcd.print(F(" #:"));
		setLCDReverse(0);
		lcd.setCursor(17, 2);
		if (validGPS) printInt(gps.satellites.value(), gps.satellites.isValid(), 5, 1); else lcd.print(F("**"));

		lcd.setCursor(0, 3);
		setLCDReverse(1);
		lcd.print(F(" Alt:"));
		setLCDReverse(0);
		lcd.setCursor(5, 3);
		if (validGPS) {
			lcd.setCursor(5, 3);
			printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2, 1);
		} else lcd.print(F("*****"));

		lcd.print(F("M "));
		lcd.setCursor(14, 3);
		setLCDReverse(1);
		if (validGPS) lcd.print(F("GPSFix")); else lcd.print(F("No Fix"));
		setLCDReverse(0);
	}
}

void locationUpdate(float flat, float flon, unsigned long kspeed, unsigned long kcourse) {
	gps_dra.end();

	Beacon.setLat(deg_to_nmea(flat,true));
	Beacon.setLon(deg_to_nmea(flon,false));
	Beacon.setKspeed(kspeed);
	Beacon.setCourse(kcourse);

	// We can optionally set power/height/gain/directivity
	// information. These functions accept ranges
	// from 0 to 10, directivity 0 to 9.
	// See this site for a calculator:
	// http://www.aprsfl.net/phgr.php
	// LibAPRS will only add PHG info if all four variables
	// are defined!
	//Beacon.setPower(2);
	//Beacon.setHeight(4);
	//Beacon.setGain(7);
	//Beacon.setDirectivity(0);

	txing();
	gps_dra.begin(9600);
}

char* deg_to_nmea(float fdeg, boolean is_lat) {
	long deg = fdeg*1000000;
	bool is_negative=0;
	if (deg < 0) is_negative=1;

	// Use the absolute number for calculation and update the buffer at the end
	deg = labs(deg);

	unsigned long b = (deg % 1000000UL) * 60UL;
	unsigned long a = (deg / 1000000UL) * 100UL + b / 1000000UL;
	b = (b % 1000000UL) / 10000UL;

	conv_buf[0] = '0';
	// in case latitude is a 3 digit number (degrees in long format)
	if( a > 9999) { snprintf(conv_buf , 6, "%04u", a);} else snprintf(conv_buf + 1, 5, "%04u", a);

	conv_buf[5] = '.';
	snprintf(conv_buf + 6, 3, "%02u", b);
	conv_buf[9] = '\0';
	if (is_lat) {
		if (is_negative) {conv_buf[8]='S';}
		else conv_buf[8]='N';
		return conv_buf+1;
		// conv_buf +1 because we want to omit the leading zero
	}
	else {
		if (is_negative) {conv_buf[8]='W';}
		else conv_buf[8]='E';
		return conv_buf;
	}
}

void invertLCD(boolean i) {
	lcd.invert(i);
}

void txing()
{
	Serial.println("");
	byte save_TIMSK0;
	byte save_PCICR;
	if (storage.doTX != 0) {
		invertLCD(true);
		digitalWrite(PTT, HIGH);         //ptt on
		digitalWrite(ledPin, HIGH);
	}
	delay(400);                                       //delay before sending data
	TCCR0B = (TCCR0B & 0b11111000) | 1;               //switch to 62500 HZ PWM frequency
	save_TIMSK0 = TIMSK0;                             //save Timer 0 register
	TIMSK0 = 0;                                       //disable Timer 0
	save_PCICR = PCICR;                               //save external pin interrupt register
	PCICR = 0;                                        //disable external pin interrupt
	Timer1.attachInterrupt(sinus_irq);                //warp interrupt in library
	Beacon.sendMessage();             				 //Beacon.sendPacket(track2, 72);

	digitalWrite(PTT, LOW);                           //PTT off
	digitalWrite(ledPin, LOW);
	invertLCD(false);
	Timer1.detachInterrupt();                         //disable timer1 interrupt
	analogWrite(bfPin, 0);                            //PWM at 0
	TCCR0B = (TCCR0B & 0b11111000) | 3;                 //register return to normal
	TIMSK0 = save_TIMSK0;
	PCICR = save_PCICR;
	Beacon.ptrStartNmea = 0;
	Serial.println("");
}

void sinus_irq()    //warp timer1 irq into DRAPRS lib
{
	Beacon.sinus();
}

void saveConfig() {
	for (unsigned int t = 0; t < sizeof(storage); t++)
		EEPROM.write(offsetEEPROM + t, *((char*)&storage + t));
}

void loadConfig() {
	if (EEPROM.read(offsetEEPROM + 0) == storage.chkDigit)
		for (unsigned int t = 0; t < sizeof(storage); t++)
			*((char*)&storage + t) = EEPROM.read(offsetEEPROM + t);
}

void printConfig() {
	if (EEPROM.read(offsetEEPROM + 0) == storage.chkDigit)
		for (unsigned int t = 0; t < sizeof(storage); t++)
			Serial.write(EEPROM.read(offsetEEPROM + t));
	Serial.println();
	setSettings(0);
}

void setSettings(bool doSet) {
	int i = 0;
	byte b;
	receivedString[0] = 'X';

	Serial.print(F("APRS Channel ("));
	Serial.print(storage.aprsChannel);
	Serial.print(F("):"));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.aprsChannel = i;
	}
	Serial.println();

	Serial.print(F("RX Channel ("));
	Serial.print(storage.rxChannel);
	Serial.print(F("):"));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.rxChannel = i;
	}
	Serial.println();

	Serial.print(F("TX Channel ("));
	Serial.print(storage.txChannel);
	Serial.print(F("):"));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.txChannel = i;
	}
	Serial.println();

	Serial.print(F("RX Tone ("));
	Serial.print(storage.rxTone);
	Serial.print(F("):"));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.rxTone = i;
	}
	Serial.println();

	Serial.print(F("TX Tone ("));
	Serial.print(storage.txTone);
	Serial.print(F("):"));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.txTone = i;
	}
	Serial.println();

	Serial.print(F("Dest. ("));
	Serial.print(storage.dest);
	Serial.print(F("):"));
	if (doSet == 1) {
		getStringValue(7,true);
		if (receivedString[0] != 0) {
			storage.dest[0] = 0;
			strcat(storage.dest, receivedString);
		}
	}
	Serial.println();

	Serial.print(F("Dest. SSID ("));
	Serial.print(storage.dest_ssid);
	Serial.print(F("):"));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.dest_ssid = i;
	}
	Serial.println();

	Serial.print(F("Call ("));
	Serial.print(storage.call);
	Serial.print(F("):"));
	if (doSet == 1) {
		getStringValue(7,true);
		if (receivedString[0] != 0) {
			storage.call[0] = 0;
			strcat(storage.call, receivedString);
		}
	}
	Serial.println();

	Serial.print(F("SSID ("));
	Serial.print(storage.ssid);
	Serial.print(F("):"));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.ssid = i;
	}
	Serial.println();

	Serial.print(F("Comment ("));
	Serial.print(storage.comment);
	Serial.print(F("):"));
	if (doSet == 1) {
		getStringValue(15,false);
		if (receivedString[0] != 0) {
			storage.comment[0] = 0;
			strcat(storage.comment, receivedString);
		}
	}
	Serial.println();

	Serial.print(F("Symbol >Car,<Motor ("));
	Serial.print(char(storage.symbool));
	Serial.print(F("):"));
	if (doSet == 1) {
		b = getCharValue();
		if (receivedString[0] != 0) storage.symbool = b;
	}
	Serial.println();

	Serial.print(F("Path1 ("));
	Serial.print(storage.path1);
	Serial.print(F("):"));
	if (doSet == 1) {
		getStringValue(7,false);
		if (receivedString[0] != 0) {
			storage.path1[0] = 0;
			strcat(storage.path1, receivedString);
		}
	}
	Serial.println();

	Serial.print(F("Path1 SSID ("));
	Serial.print(storage.path1_ssid);
	Serial.print(F("): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.path1_ssid = i;
	}
	Serial.println();

	Serial.print(F("Path2 ("));
	Serial.print(storage.path2);
	Serial.print(F("): "));
	if (doSet == 1) {
		getStringValue(7,false);
		if (receivedString[0] != 0) {
			storage.path2[0] = 0;
			strcat(storage.path2, receivedString);
		}
	}
	Serial.println();

	Serial.print(F("Path2 SSID ("));
	Serial.print(storage.path2_ssid);
	Serial.print(F("): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.path2_ssid = i;
	}
	Serial.println();

	Serial.print(F("Interval ("));
	Serial.print(storage.interval);
	Serial.print(F("): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.interval = i;
	}
	Serial.println();

	Serial.print(F("Multiplier ("));
	Serial.print(storage.multiplier);
	Serial.print(F("): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.multiplier = i;
	}
	Serial.println();

	Serial.print(F("Power ("));
	Serial.print(storage.power);
	Serial.print(F("W): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.power = i;
	}
	Serial.println();

	Serial.print(F("Height ("));
	Serial.print(storage.height);
	Serial.print(F("M): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.height = i;
	}
	Serial.println();

	Serial.print(F("Gain ("));
	Serial.print(storage.gain);
	Serial.print(F("): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.gain = i;
	}
	Serial.println();

	Serial.print(F("Directivity ("));
	Serial.print(storage.directivity);
	Serial.print(F("): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.directivity = i;
	}
	Serial.println();

	Serial.print(F("PreAmble ("));
	Serial.print(storage.preAmble);
	Serial.print(F(" chars): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.preAmble = i;
	}
	Serial.println();

	Serial.print(F("Tail ("));
	Serial.print(storage.tail);
	Serial.print(F(" chars): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.tail = i;
	}
	Serial.println();

	Serial.print(F("TX enabled ("));
	Serial.print(storage.doTX);
	Serial.print(F("): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.doTX = i;
	}
	Serial.println();

	Serial.print(F("Send beacon after TX ("));
	Serial.print(storage.BcnAfterTX);
	Serial.print(F("): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.BcnAfterTX = i;
	}
	Serial.println();

	Serial.print(F("No APRS after PTT in sec's ("));
	Serial.print(storage.NoAPRSAfterPTT);
	Serial.print(F("): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.NoAPRSAfterPTT = i;
	}
	Serial.println();

	Serial.print(F("TX Timeout ("));
	Serial.print(storage.txTimeOut);
	Serial.print(F("): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.txTimeOut = i;
	}
	Serial.println();

	Serial.print(F("Debugmode ("));
	Serial.print(storage.isDebug);
	Serial.print(F("): "));
	if (doSet == 1) {
		i = getNumericValue();
		if (receivedString[0] != 0) storage.isDebug = i;
	}
	Serial.println();

	if (doSet == 1) {
		saveConfig();
		loadConfig();
	}
}

void getStringValue(int length, bool fixedLen) {
	serialFlush();
	receivedString[0] = 0;
	int i = 0;
	while (receivedString[i] != 13 && i < length) {
		if (Serial.available() > 0) {
			receivedString[i] = Serial.read();
			if (receivedString[i] == 13 || receivedString[i] == 10) {
				i--;
			}
			else {
				Serial.write(receivedString[i]);
			}
			i++;
		}
	}
	if (fixedLen==true && i>0 ){
		while (i<length-1){
			receivedString[i]=' ';
			i++;
		}
	}
	receivedString[i] = 0;
	serialFlush();
}

byte getCharValue() {
	serialFlush();
	receivedString[0] = 0;
	int i = 0;
	while (receivedString[i] != 13 && i < 2) {
		if (Serial.available() > 0) {
			receivedString[i] = Serial.read();
			if (receivedString[i] == 13 || receivedString[i] == 10) {
				i--;
			}
			else {
				Serial.write(receivedString[i]);
			}
			i++;
		}
	}
	receivedString[i] = 0;
	serialFlush();
	return receivedString[i - 1];
}

byte getNumericValue() {
	serialFlush();
	byte myByte = 0;
	byte inChar = 0;
	bool isNegative = false;
	receivedString[0] = 0;

	int i = 0;
	while (inChar != 13) {
		if (Serial.available() > 0) {
			inChar = Serial.read();
			if (inChar > 47 && inChar < 58) {
				receivedString[i] = inChar;
				i++;
				Serial.write(inChar);
				myByte = (myByte * 10) + (inChar - 48);
			}
			if (inChar == 45) {
				Serial.write(inChar);
				isNegative = true;
			}
		}
	}
	receivedString[i] = 0;
	if (isNegative == true) myByte = myByte * -1;
	serialFlush();
	return myByte;
}

void serialFlush() {
	for (int i = 0; i < 10; i++)
	{
		while (Serial.available() > 0) {
			Serial.read();
		}
	}
}

static void smartDelay(unsigned long ms)
{
	int pttValue = 1000;
	unsigned long start = millis();
	gps_dra.flush();
	do
	{
		while (gps_dra.available()) gps.encode(gps_dra.read());
		if (ms > 500) {
			pttValue = analogRead(0);
			pttPressed = ((pttValue > pttOffValue + 250) || (digitalRead(PTTPin)==0));
			if (digitalRead(button) == 0) buttonPressed = 1;
		}
	} while (millis() - start < ms && pttPressed == lastPttPressed && buttonPressed == 0);
}

static void printFloat(float val, bool valid, int len, int prec, bool toLCD)
{
	if (!valid)
	{
		while (len-- > 1) {
			if (toLCD) lcd.print(F("*")); else Serial.print(F("*"));
		}
		if (!toLCD) Serial.print(F(" "));
	}
	else
	{
		if (toLCD) lcd.print(val, prec); else Serial.print(val, prec);
		int vi = abs((int)val);
		int flen = prec + (val < 0.0 ? 2 : 1); // . and -
		flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
		for (int i = flen; i < len; ++i) {
			if (!toLCD) Serial.print(' ');
		}
	}
	smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len, bool toLCD)
{
	char sz[32] = "*****************";
	if (valid)
		sprintf(sz, "%ld", val);
	sz[len] = 0;
	for (int i = strlen(sz); i < len; ++i)
		sz[i] = ' ';
	if (len > 0)
		sz[len - 1] = ' ';
	if (toLCD) lcd.print(sz); else Serial.print(sz);
	smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t, bool toLCD)
{
	if (!toLCD) {
		if (!d.isValid())
		{
			Serial.print(F("********** "));
		}
		else
		{
			char sz[32];
			sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
			Serial.print(sz);
		}
	}

	if (!t.isValid())
	{
		if (toLCD) lcd.print(F("********")); else Serial.print(F("********** "));
	}
	else
	{
		char sz[32];
		sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
		if (toLCD) lcd.print(sz); else Serial.print(sz);
	}

	if (!toLCD) {
		printInt(d.age(), d.isValid(), 5, 0);
		smartDelay(0);
	}
}

static void printStr(const char *str, int len)
{
	int slen = strlen(str);
	for (int i = 0; i < len; ++i)
		Serial.print(i < slen ? str[i] : ' ');
	smartDelay(0);
}

void setDra(byte rxFreq, byte txFreq, byte rxTone, byte txTone) {
	char buff[50];
	int txPart, rxPart;
	if(txFreq>79) txPart = txFreq-80; else txPart=txFreq;
	if(rxFreq>79) rxPart = rxFreq-80; else rxPart=rxFreq;

	sprintf(buff,"AT+DMOSETGROUP=0,14%01d.%04d,14%01d.%04d,%04d,1,%04d",int(txFreq/80)+4,txPart*125,int(rxFreq/80)+4,rxPart*125,txTone,rxTone);
	Serial.println();
	Serial.println(buff);
	gps_dra.println(buff);
}

void setBigSize(boolean i) {
	lcd.setDoubleSize(i);
}

void setLCDReverse(boolean i) {
	lcd.setCharReverse(i);
}

void beginLCD() {
	lcd.begin();
}

void setup() {
	Serial.begin(9600);
	Wire.begin();
	beginLCD();
	delay(1);
	lcd.setCursor(0,0);
	setBigSize(true);
	setLCDReverse(true);
	lcd.print(F("   APRS    "));
	setBigSize(false);
	setLCDReverse(false);
	lcd.setCursor(0,2);
	lcd.print(F("  TRACKER v3.2NL  "));    //intro
	pinMode(bfPin, OUTPUT);
	pinMode(PTT, OUTPUT);
	digitalWrite(PTT, LOW);
	pinMode(ledPin, OUTPUT);
	digitalWrite(ledPin,LOW);
	pinMode(MicPwr, OUTPUT);
	digitalWrite(MicPwr,LOW);
	pinMode(sqlPin, INPUT);
	pinMode(PTTPin, INPUT_PULLUP);
	pinMode(button, INPUT_PULLUP);

	Timer1.initialize(76);    //µs  fe=13200 hz so TE=76µs 13157.9 mesured

	if (EEPROM.read(offsetEEPROM) != storage.chkDigit || digitalRead(button) == 0){
		//if (EEPROM.read(offsetEEPROM) != storage.chkDigit){
		Serial.println(F("Writing defaults"));
		saveConfig();
	}

	loadConfig();
	printConfig();

	Serial.println(F("Type GS to enter setup:"));
	delay(2000);
	lcd.setCursor(0,2); lcd.print(F("Connect serial to"));
	lcd.setCursor(2,3); lcd.print(F("enter setup ..."));
	delay(3000);
	if (Serial.available()) {
		Serial.println(F("Check for setup"));
		if (Serial.find(chkGS)) {
			Serial.println(F("Setup entered..."));
			setSettings(1);
			delay(2000);
		}
	}

	//if (detectMenu() == 1) EepromMenu();
	Serial.println("");
	pttOffValue = analogRead(0);
	Serial.print(F("PTT OffValue"));  Serial.println(pttOffValue);
	Beacon.begin(bfPin, 1200, 2200, 350);   //analog pin, led pin, freq1, freq2, shift freq
	Beacon.setCallsign(storage.call, storage.ssid);
	Beacon.setDestination(storage.dest, storage.dest_ssid);
	Beacon.setSymbol(storage.symbool);
	Beacon.setComment(storage.comment);
	Beacon.setPath1(storage.path1, storage.path1_ssid);
	Beacon.setPath2(storage.path2, storage.path2_ssid);
	Beacon.setPower(storage.power);
	Beacon.setHeight(storage.height);
	Beacon.setGain(storage.gain);
	Beacon.setDirectivity(storage.directivity);
	Beacon.setPreamble(storage.preAmble);
	Beacon.setTail(storage.tail);

	Updatedelay = storage.interval * storage.multiplier;

	Beacon.printSettings();
	Serial.print(F("Free RAM:     ")); Serial.println(Beacon.freeMemory());

	gps_dra.begin(9600);
	setDra(storage.rxChannel, storage.txChannel, storage.rxTone, storage.txTone);
	gps_dra.println(F("AT+DMOSETVOLUME=8"));
	gps_dra.println(F("AT+DMOSETMIC=8,0"));
	gps_dra.println(F("AT+SETFILTER=1,1,1"));

	Serial.println(F("Sats HDOP Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Dir   Interval"));
	Serial.println(F("          (deg)      (deg)       Age                      Age  (m)    --- from GPS ---   (sec)   "));
	Serial.println(F("-------------------------------------------------------------------------------------------------"));

	lcd.clear();

	timeOutTimerMillis = (long)storage.txTimeOut*1000;
}










