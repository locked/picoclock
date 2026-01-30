#include "ens160.h"
#include "pico/stdlib.h"
#include <stdio.h>

static i2c_inst_t *ens160_i2c;

void ens160_init(i2c_inst_t *i2c) {
    ens160_i2c = i2c;

	bool ping = ens160_ping(ENS160_ADDRESS_LOW);
	printf("Gas Sensor Ping: %d\n", ping);

	uint16_t unique_id = ens160_getUniqueID();
	printf("Gas Sensor Unique ID: %04x\n", unique_id);

	if (ens160_setOperatingMode(SFE_ENS160_RESET)) {
		printf("Ready.\n");
	}
	sleep_ms(100);
	ens160_setOperatingMode(SFE_ENS160_IDLE);
	sleep_ms(500);
	ens160_setOperatingMode(SFE_ENS160_STANDARD);
	int ensStatus = ens160_getFlags();
	printf("Gas Sensor Status Flag: %d\n", ensStatus);
}


int32_t ens160_readRegisterRegion(uint8_t reg, uint8_t *data, uint8_t length) {
    i2c_write_blocking(ens160_i2c, ENS160_ADDRESS_LOW, (uint8_t*)(&reg), 1, true);
	return i2c_read_blocking(ens160_i2c, ENS160_ADDRESS_LOW, data, length, false);
}

int32_t ens160_writeRegisterRegion(uint8_t reg, uint8_t data, uint8_t length) {
    uint8_t buf[] = {reg, data};
    return i2c_write_blocking(ens160_i2c, ENS160_ADDRESS_LOW, buf, length, true);
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

    retVal = ens160_readRegisterRegion(SFE_ENS160_PART_ID, tempVal, 2);

	id = tempVal[0];
	id |= tempVal[1] << 8;

	if( retVal != 0 )
		return 0;

	return id; 
}

bool ens160_isConnected() {
	uint16_t uniqueID; 
	uniqueID = ens160_getUniqueID(); 
	if( uniqueID != ENS160_DEVICE_ID )
		return false;
	return true;
}



bool ens160_setOperatingMode(uint8_t val) {
	int32_t retVal;

	if( val > SFE_ENS160_RESET )
		return false;

	retVal = ens160_writeRegisterRegion(SFE_ENS160_OP_MODE, val, 1);

	if( retVal != 0 )
		return false;

	return true; 
}

uint8_t ens160_getFlags() {
	int32_t retVal;
	uint8_t tempVal;

	retVal = ens160_readRegisterRegion(SFE_ENS160_DEVICE_STATUS, &tempVal, 1);

	if( retVal != 0 )
		return 0xFF; // Change to general error

	tempVal = (tempVal & 0x0C) >> 2; 

	switch( tempVal )
	{
		case 0: // Normal operation
			return 0;
			break;
		case 1: // Warm-up phase
			return 1;
			break;
		case 2: // Initial Start-Up Phase
			return 2;
			break;
		case 3: // Invalid Output
			return 3;
			break;
		default:
			return 0xFF;
	}
}

uint16_t ens160_getTVOC() {
	int32_t retVal;
	uint16_t tvoc; 
	uint8_t tempVal[2] = {0}; 

	retVal = ens160_readRegisterRegion(SFE_ENS160_DATA_TVOC, tempVal, 2);

	if( retVal != 0 )
		return 0;

	tvoc = tempVal[0];
	tvoc |= tempVal[1] << 8;

	return tvoc;
}
