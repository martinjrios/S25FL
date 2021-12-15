/*
 *  S25FL_CIAA_port.h
 *
 *  Created on: 17-09-2021
 *  Author: Martin Rios - jrios@fi.uba.ar
 * 
 */

#ifndef _S25FL_CIAA_PORT_H_
#define _S25FL_CIAA_PORT_H_

#include "S25FL.h"

void chipSelect_CIAA_port(csState_t estado);
bool spiRead_CIAA_port(uint8_t* buffer, uint32_t bufferSize);
uint8_t spiReadRegister_CIAA_port(uint8_t reg);
void spiWrite_CIAA_port(uint8_t* buffer, uint32_t bufferSize);
void spiWriteByte_CIAA_port(uint8_t data);
void delay_CIAA_port(uint32_t millisecs);

#endif // _S25FL_CIAA_PORT_H_
