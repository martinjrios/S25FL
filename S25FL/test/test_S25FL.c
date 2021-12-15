/*
 *  test_S25FL.c
 *
 *  Created on: 14-12-2021
 *  Author: Martin Rios - jrios@fi.uba.ar
 * 
 * Prueba unitaria del modulo S25FL.c que forma parte del driver para el manejo de una memoria flash SPI
 * de 64 Mb, cuyo codigo de componente es S25FL064L. 
 * 
 */

#include "unity.h"
#include "S25FL.h"
#include "mock_S25FL_CIAA_port.h"

#define ERROR_ESCRITURA         0

/**
 * @brief Estructura que contiene las funciones para el manejo de bajo nivel de la memoria.
 * 
 */
s25fl_t s25flDriverStruct;

void setUp(void) {
    // Se inicializa la estructura con los punteros a las funciones del port
    s25flDriverStruct.chip_select_ctrl = chipSelect_CIAA_port;
    s25flDriverStruct.spi_write_fnc = spiWrite_CIAA_port;
    s25flDriverStruct.spi_writeByte_fnc = spiWriteByte_CIAA_port;
    s25flDriverStruct.spi_read_fnc = spiRead_CIAA_port;
    s25flDriverStruct.spi_read_register = spiReadRegister_CIAA_port;
    s25flDriverStruct.delay_fnc = delay_CIAA_port;
    s25flDriverStruct.memory_size = S64MB;
}

void tearDown(void) {
}

/**
 * @brief Prueba de inicializacion del driver para la memoria flash.
 * 
 */
void test_inicializar_driver(void) {
    bool result = false;

    result = S25FL_InitDriver(s25flDriverStruct);
    
    TEST_ASSERT_EQUAL(true, result);
}

/**
 * @brief Prueba la lectura del registro de estado de la memoria que indica que esta
 *        realizando alguna operacion interna y por lo tanto no esta disponible para
 *        operar.
 */
void test_lectura_estado_flash_ocupada(void) {
    uint8_t rxBuff[] = {SPIFLASH_STAT_BUSY}, estado = 0;

    chipSelect_CIAA_port_Expect(CS_ENABLE); // Antes de enviar un comando a la memoria se debe habilitar el chip select
    spiWriteByte_CIAA_port_Expect(S25FL_CMD_READSTAT1); // Se debe escribir primero el valor del registro de estado numero 1
    
    // Se simula que ingresan datos por el puerto SPI
    spiRead_CIAA_port_ExpectAndReturn(rxBuff, 1, SPIFLASH_STAT_BUSY);
    spiRead_CIAA_port_IgnoreArg_buffer();
    spiRead_CIAA_port_ReturnArrayThruPtr_buffer(rxBuff, 1);

    chipSelect_CIAA_port_Expect(CS_DISABLE); // Luego de terminar la operacion con la memoria se debe liberar el chip select
    
    estado = S25FL_readStatus();

    TEST_ASSERT_EQUAL_UINT8(SPIFLASH_STAT_BUSY, estado);
}

/**
 * @brief Prueba la lectura del registro de estado de la memoria que indica 
 *        que se encuentra habilitada la escritura de datos.
 * 
 */
void test_lectura_estado_flash_habilitada_escritura(void) {
    uint8_t rxBuff[] = {SPIFLASH_STAT_WRTEN}, estado = 0;

    chipSelect_CIAA_port_Expect(CS_ENABLE); // Antes de enviar un comando a la memoria se debe habilitar el chip select
    spiWriteByte_CIAA_port_Expect(S25FL_CMD_READSTAT1); // Se debe escribir primero el valor del registro de estado numero 1
    
    // Se simula que ingresan datos por el puerto SPI
    spiRead_CIAA_port_ExpectAndReturn(rxBuff, 1, SPIFLASH_STAT_WRTEN);
    spiRead_CIAA_port_IgnoreArg_buffer();
    spiRead_CIAA_port_ReturnArrayThruPtr_buffer(rxBuff, 1);
    
    chipSelect_CIAA_port_Expect(CS_DISABLE); // Luego de terminar la operacion con la memoria se debe liberar el chip select
    
    estado = S25FL_readStatus();

    TEST_ASSERT_EQUAL_UINT8(SPIFLASH_STAT_WRTEN, estado);
}

/**
 * @brief Prueba la lectura de datos guardados en una cierta direccion
 *        de la memoria.
 * 
 */
void test_leyendo_datos(void) {
    uint32_t addr = 1024;
    uint8_t readBuff[32] = {0};
    uint32_t len = 10, readLen = 0;
    uint8_t response[] = "Prueba mem";

    chipSelect_CIAA_port_Expect(CS_ENABLE); // Antes de enviar un comando a la memoria se debe habilitar el chip select
    spiWriteByte_CIAA_port_Expect(SPIFLASH_SPI_DATAREAD); // Se debe escribir primero el comando de lectura
    spiWrite_CIAA_port_Ignore();  // Se ignora la escritura de datos de bajo nivel
    
    // Se simula que se leen datos por el puerto SPI
    spiRead_CIAA_port_ExpectAndReturn(readBuff, len, response);
    spiRead_CIAA_port_IgnoreArg_buffer();
    spiRead_CIAA_port_ReturnArrayThruPtr_buffer(response, len);    
    
    chipSelect_CIAA_port_Expect(CS_DISABLE); // Luego de terminar la operacion con la memoria se debe liberar el chip select

    readLen = S25FL_readBuffer(addr, readBuff, len);

    TEST_ASSERT_EQUAL_UINT32(len, readLen);
    TEST_ASSERT_EQUAL_STRING(response, readBuff);
}

/**
 * @brief Prueba de la escritura de datos en una pagina de la memoria.
 * 
 */
void test_escritura_pagina(void) {
    uint32_t addr = 256;
    uint8_t writeBuff[256] = "Probando";
    uint32_t len = 8, writeLen = 0;
    bool fastQuit = false;

    delay_CIAA_port_Ignore();   // Se ignora la funcion para generar los delays de hardware
    chipSelect_CIAA_port_Expect(CS_ENABLE); // Antes de enviar un comando a la memoria se debe habilitar el chip select
    spiWriteByte_CIAA_port_Expect(S25FL_CMD_WRITEENABLE); // Primero se debe enviar el comando de habilitar escritura
    chipSelect_CIAA_port_Expect(CS_DISABLE); // Luego de escribir el comando se debe liberar el chip select
    chipSelect_CIAA_port_Expect(CS_ENABLE); // Antes de enviar un comando o datos a la memoria se debe habilitar el chip select
    spiWriteByte_CIAA_port_Expect(S25FL_CMD_PAGEPROG);  // Antes de escribir los datos se debe enviar el comando de programacion de pagina
    spiWrite_CIAA_port_Ignore();  // Se ignora la funcion de bajo nivel para escribir los datos en memoria
    chipSelect_CIAA_port_Expect(CS_DISABLE); // Luego de terminada la escritura de datos se debe liberar el chip select

    writeLen = S25FL_writePage(addr, writeBuff, len, fastQuit);

    TEST_ASSERT_EQUAL_UINT32(len, writeLen);    
}

/**
 * @brief Prueba de escritura de pagina cuando se pasan argumentos
 *        invalidos a la funcion.
 * 
 */
void test_falla_escritura_pagina(void) {
    uint32_t addr;
    uint8_t writeBuff[256] = {0};
    uint32_t len, writeLen = 0;
    bool fastQuit = false;

    // Se prueba que la direccion sea valida
    addr = S25FL_MAXADDRESS;
    len = 8;
    writeLen = S25FL_writePage(addr, writeBuff, len, fastQuit);
    TEST_ASSERT_EQUAL_UINT32(ERROR_ESCRITURA, writeLen);     

    // Se prueba que la longitud de los datos no supere el tamaño de la pagina
    addr = 0;
    len = 270;
    writeLen = S25FL_writePage(addr, writeBuff, len, fastQuit);
    TEST_ASSERT_EQUAL_UINT32(ERROR_ESCRITURA, writeLen);   

    // Se prueba que los datos no sean escritos mas alla de los limites de la pagina
    addr = 255;
    len = 4;
    writeLen = S25FL_writePage(addr, writeBuff, len, fastQuit);
    TEST_ASSERT_EQUAL_UINT32(ERROR_ESCRITURA, writeLen);         
}
