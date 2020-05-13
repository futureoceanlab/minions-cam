#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "DS3231.h"
#include "utility.cpp"


#define CLOCK_ADDRESS 0x68


// Constructor
DS3231::DS3231() 
{
	bus = 0;
}

// Constructor
DS3231::DS3231(int i2c_bus) 
{
	bus = i2c_bus;
}


void DS3231::init() 
{
    char buf[16];

	sprintf(buf, "/dev/i2c-%d", bus);
	
	if ((fd = open(buf, O_RDWR)) < 0)
	{
		fprintf(stderr, "Failed to open i2c bus /dev/i2c-%d\n", bus);
		exit(1);
	}

	// Initialize Keller LD
	selectDevice(CLOCK_ADDRESS);
}

void DS3231::setA1Time(uint8_t A1Day, uint8_t A1Hour, uint8_t A1Minute, 
                       uint8_t A1Second, uint8_t AlarmBits, bool A1Dy, 
                       bool A1h12, bool A1PM) 
{
	//	Sets the alarm-1 date and time on the DS3231, using A1* information
	uint8_t temp_buffer;
    char buf[6];
	buf[0] = 0x07;
    buf[1] = decToBcd(A1Second) | ((AlarmBits & 0b00000001) << 7);
    buf[2] = decToBcd(A1Minute) | ((AlarmBits & 0b00000010) << 6);
	// Figure out A1 hour 
	if (A1h12) {
		// Start by converting existing time to h12 if it was given in 24h.
		if (A1Hour > 12) {
			// well, then, this obviously isn't a h12 time, is it?
			A1Hour = A1Hour - 12;
			A1PM = true;
		}
		if (A1PM) {
			// Afternoon
			// Convert the hour to BCD and add appropriate flags.
			temp_buffer = decToBcd(A1Hour) | 0b01100000;
		} else {
			// Morning
			// Convert the hour to BCD and add appropriate flags.
			temp_buffer = decToBcd(A1Hour) | 0b01000000;
		}
	} else {
		// Now for 24h
		temp_buffer = decToBcd(A1Hour); 
	}
	temp_buffer = temp_buffer | ((AlarmBits & 0b00000100)<<5);
    
	// A1 hour is figured out, send it
    buf[3] = temp_buffer;
	// Figure out A1 day/date and A1M4
	temp_buffer = ((AlarmBits & 0b00001000)<<4) | decToBcd(A1Day);
	if (A1Dy) {
		// Set A1 Day/Date flag (Otherwise it's zero)
		temp_buffer = temp_buffer | 0b01000000;
	}
    buf[4] = temp_buffer;

    if ((write(fd, buf, 5)) != 5)
	{
		fprintf(stderr, "Error writing to Keller LD\n");
		exit(1);
	}

}


void DS3231::turnOnAlarm(uint8_t Alarm) {
	// turns on alarm number "Alarm". Defaults to 2 if Alarm is not 1.
	uint8_t temp_buffer = readControlByte(0);
	// modify control byte
	if (Alarm == 1) {
		temp_buffer = temp_buffer | 0b00000101;
	} else {
		temp_buffer = temp_buffer | 0b00000110;
	}
	writeControlByte(temp_buffer, 0);
}

void DS3231::turnOffAlarm(uint8_t Alarm) {
	// turns off alarm number "Alarm". Defaults to 2 if Alarm is not 1.
	// Leaves interrupt pin alone.
	uint8_t temp_buffer = readControlByte(0);
	// modify control byte
	if (Alarm == 1) {
		temp_buffer = temp_buffer & 0b11111110;
	} else {
		temp_buffer = temp_buffer & 0b11111101;
	}
	writeControlByte(temp_buffer, 0);
}

bool DS3231::checkAlarmEnabled(uint8_t Alarm) {
	// Checks whether the given alarm is enabled.
	uint8_t result = 0x0;
	uint8_t temp_buffer = readControlByte(0);
	if (Alarm == 1) {
		result = temp_buffer & 0b00000001;
	} else {
		result = temp_buffer & 0b00000010;
	}
	return result;
}


/***************************************** 
	Private Functions
 *****************************************/

uint8_t DS3231::decToBcd(uint8_t val) {
// Convert normal decimal numbers to binary coded decimal
	return ( (val/10*16) + (val%10) );
}

uint8_t DS3231::bcdToDec(uint8_t val) {
// Convert binary coded decimal to normal decimal numbers
	return ( (val/16*10) + (val%16) );
}

uint8_t DS3231::readControlByte(bool which) {
	// Read selected control byte
	// first byte (0) is 0x0e, second (1) is 0x0f
    char buf[1];

	if (which) {
		// second control byte
		buf[0] = 0x0f;
	} else {
		// first control byte
		buf[0] = 0x0e;
	}

    if ((write(fd, buf, 1)) != 1)
	{
		fprintf(stderr, "Error writing to Keller LD\n");
		exit(1);
	}
    
    // Read control byte
	if (read(fd, buf, 1) != 1)
	{
		fprintf(stderr, "Error reading from Keller LD\n");
		exit(1);
	}
	return buf[0];	
}

void DS3231::writeControlByte(uint8_t control, bool which) {
	// Write the selected control byte.
	// which=false -> 0x0e, true->0x0f.
    char buf[2];

	if (which) {
		// second control byte
		buf[0] = 0x0f;
	} else {
		// first control byte
		buf[0] = 0x0e;
	}
    buf[1] = control;
    if ((write(fd, buf, 2)) != 2)
	{
		fprintf(stderr, "Error writing to Keller LD\n");
		exit(1);
	}
}
