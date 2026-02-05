#include "ens160.h"
#include "pico/stdlib.h"
#include <stdio.h>

static i2c_inst_t *ens160_i2c;

void ens160_init(i2c_inst_t *i2c) {
    ens160_i2c = i2c;

	//bool ping = ens160_ping(ENS160_ADDRESS_LOW);
	//printf("Gas Sensor Ping: %d\n", ping);

	uint16_t unique_id = ens160_getUniqueID();
	printf("[ens160] Gas Sensor Unique ID: %04x\n", unique_id);

	if (ens160_setOperatingMode(SFE_ENS160_RESET)) {
		printf("[ens160] Ready.\n");
	}
	sleep_ms(100);
	ens160_setOperatingMode(SFE_ENS160_IDLE);
	sleep_ms(500);
	ens160_setOperatingMode(SFE_ENS160_STANDARD);
	int ensStatus = ens160_getFlags();
	printf("[ens160] Gas Sensor Status Flag: %d\n", ensStatus);
}

bool ens160_ping(int address) {
    int ret;
    uint8_t rxdata;
    ret = i2c_read_blocking(ens160_i2c, address, &rxdata, 1, false);
    if (ret != PICO_ERROR_GENERIC)
        return true;
    return false;
}

uint16_t ens160_getUniqueID() {
    int32_t retVal;
	uint8_t tempVal[2] = {0}; 
	uint16_t id; 

    i2c_write_blocking(ens160_i2c, ENS160_ADDRESS_LOW, SFE_ENS160_PART_ID, 1, false);
	retVal = i2c_read_blocking(ens160_i2c, ENS160_ADDRESS_LOW, tempVal, 2, false);
	if (retVal != 2) {
		return 0;
	}

	id = tempVal[0];
	id |= tempVal[1] << 8;

	return id; 
}

bool ens160_isConnected() {
	uint16_t uniqueID; 
	uniqueID = ens160_getUniqueID(); 
	if (uniqueID != ENS160_DEVICE_ID) {
		return false;
	}
	return true;
}

bool ens160_setOperatingMode(uint8_t val) {
	if (val > SFE_ENS160_RESET) {
		return false;
	}

    uint8_t buf[] = {SFE_ENS160_OP_MODE, val};
    i2c_write_blocking(ens160_i2c, ENS160_ADDRESS_LOW, buf, 2, false);

	return true; 
}

uint8_t ens160_getFlags() {
	int32_t retVal;
	uint8_t tempVal;

    uint8_t buf[] = {SFE_ENS160_DEVICE_STATUS};
    i2c_write_blocking(ens160_i2c, ENS160_ADDRESS_LOW, buf, 1, false);
	retVal = i2c_read_blocking(ens160_i2c, ENS160_ADDRESS_LOW, &tempVal, 1, false);
	if (retVal != 1) {
		return 0xFF;
	}

	tempVal = (tempVal & 0x0C) >> 2;

	switch (tempVal) {
		case 0: // Normal operation
		case 1: // Warm-up phase
		case 2: // Initial Start-Up Phase
		case 3: // Invalid Output
			return tempVal;
		default:
			return 0xFF;
	}
}

uint16_t ens160_getTVOC() {
	uint16_t tvoc; 
	uint8_t tempVal[2] = {0}; 

    uint8_t buf[] = {SFE_ENS160_DATA_TVOC};
    i2c_write_blocking(ens160_i2c, ENS160_ADDRESS_LOW, buf, 1, false);
	int32_t retVal = i2c_read_blocking(ens160_i2c, ENS160_ADDRESS_LOW, tempVal, 2, false);
	if (retVal != 2) {
		return 0;
	}

	tvoc = tempVal[0];
	tvoc |= tempVal[1] << 8;

	return tvoc;
}

uint16_t ens160_getECO2() {
	uint16_t eco;
	uint8_t tempVal[2] = {0};

    uint8_t buf[] = {SFE_ENS160_DATA_ECO2};
    i2c_write_blocking(ens160_i2c, ENS160_ADDRESS_LOW, buf, 1, false);
	i2c_read_blocking(ens160_i2c, ENS160_ADDRESS_LOW, tempVal, 2, false);

	eco = tempVal[0];
	eco |= tempVal[1]  << 8;

	return eco;
}

uint8_t ens160_checkDataStatus() {
	uint8_t tempVal;

    uint8_t buf[] = {SFE_ENS160_DEVICE_STATUS};
    i2c_write_blocking(ens160_i2c, ENS160_ADDRESS_LOW, buf, 1, false);
	i2c_read_blocking(ens160_i2c, ENS160_ADDRESS_LOW, &tempVal, 1, false);

	tempVal &= 0x02 >> 1;

	return tempVal;
}

// Does not return actual temperature
// It gives the compensation value fed to him
float ens160_getTempKelvin() {
	float temperature; 
	int16_t tempConversion; 
	uint8_t tempVal[2] = {0};

    uint8_t buf[] = {SFE_ENS160_DATA_T};
    i2c_write_blocking(ens160_i2c, ENS160_ADDRESS_LOW, buf, 1, false);
	i2c_read_blocking(ens160_i2c, ENS160_ADDRESS_LOW, tempVal, 2, false);

	tempConversion = tempVal[0];
	tempConversion |= (tempVal[1] << 8);
	temperature = (float)tempConversion; 

	temperature = temperature/64; // Formula as described on pg. 32 of datasheet.

	return temperature;
}

float ens160_getTempCelsius() {
	float temperature; 

	temperature = ens160_getTempKelvin();

	return (temperature - 273.15);
}

bool ens160_setTempCompensation(float tempKelvin) {
	int32_t retVal;
	uint8_t tempVal[3] = {0};
	uint16_t kelvinConversion;

	kelvinConversion = (uint16_t) (tempKelvin * 64); // convert value - fixed equation pg. 29 of datasheet
    tempVal[0] = SFE_ENS160_TEMP_IN;
	tempVal[1] = (kelvinConversion & 0x00FF);
	tempVal[2] = (kelvinConversion & 0xFF00) >> 8;

    retVal = i2c_write_blocking(ens160_i2c, ENS160_ADDRESS_LOW, tempVal, 3, false);

	if (retVal != 3) {
		printf("[ens160] Failed setting temp compensation in Kelvin\n");
		return false;
	}
	//printf("[ens160] Set temp compensation to %d Kelvin\n", kelvinConversion);

	return true;
}

bool ens160_setTempCompensationCelsius(float tempCelsius) {
	float kelvinConversion = tempCelsius + 273.15;

	//printf("[ens160] Set temp compensation to %0.1f C => %0.1f Kelvin\n", tempCelsius, kelvinConversion);
	if (ens160_setTempCompensation(kelvinConversion)) {
		return true;
	}
	printf("[ens160] Failed setting temp compensation in Celcius\n");

	return false;
}

float ens160_getRH() {
	uint16_t rh; 
	uint8_t tempVal[2] = {0};

    uint8_t buf[] = {SFE_ENS160_DATA_RH};
    i2c_write_blocking(ens160_i2c, ENS160_ADDRESS_LOW, buf, 1, false);
	i2c_read_blocking(ens160_i2c, ENS160_ADDRESS_LOW, tempVal, 2, false);

	rh = tempVal[0];
	rh |= tempVal[1] << 8;

	rh = rh/512; // Formula as described on pg. 33 of datasheet.

	return rh;
}
