/*
 *  S25FL.h
 *
 *  Created on: 17-09-2021
 *  Author: Martin Rios - jrios@fi.uba.ar
 * 
 */

#ifndef _S25FL_H_
#define _S25FL_H_

#include <stdint.h>
#include <stdbool.h>

#define  SPIFLASH_SPI_STATREAD          0x02
#define  SPIFLASH_SPI_DATAWRITE         0x01
#define  SPIFLASH_SPI_DATAREAD          0x03
#define  SPIFLASH_SPI_READY             0x01

// Flash status bits
#define  SPIFLASH_STAT_BUSY             0x01   // Erase/Write in Progress
#define  SPIFLASH_STAT_WRTEN            0x02   // Write Enable Latch

// SPI Flash Characteristics (S25FL Specific)
#define S25FL_MAXADDRESS                0x07FFFFF
#define S25FL_MAX_ADDRESS_SIZE          3      // bytes address size
#define S25FL_PAGESIZE                  256    // 256 bytes per programmable page
#define S25FL_PAGES                     32768  // 8,388,608 Bytes / 256 bytes per page
#define S25FL_SECTORSIZE                4096   // 1 erase sector = 4096 bytes
#define S25FL_SECTORS                   2048    // 8,388,608 Bytes / 4096 bytes per sector
#define S25FL_BLOCKSIZE                 65536  // 1 erase block = 64K bytes
#define S25FL_BLOCKS                    128     // 8,388,608 Bytes / 4096 bytes per sector
#define S25FL_MANUFACTURERID            0x01   // Used to validate read data
#define S25FL_DEVICEID                  0x60   // Used to validate read data

// Erase/Program Instructions       
#define S25FL_CMD_WRITEENABLE           0x06   // Write Enabled
#define S25FL_CMD_WRITEDISABLE          0x04   // Write Disabled
#define S25FL_CMD_READSTAT1             0x05   // Read Status Register 1
#define S25FL_CMD_READSTAT2             0x07   // Read Status Register 2
#define S25FL_CMD_WRITESTAT             0x01   // Write Status Register
#define S25FL_CMD_PAGEPROG              0x02   // Page Program
#define S25FL_CMD_QUADPAGEPROG          0x32   // Quad Page Program
#define S25FL_CMD_SECTERASE4            0x20   // Sector Erase (4KB)
#define S25FL_CMD_BLOCKERASE32          0x52   // Block Erase (32KB)
#define S25FL_CMD_BLOCKERASE64          0xD8   // Block Erase (64KB)
#define S25FL_CMD_CHIPERASE             0x60   // Chip Erase
#define S25FL_CMD_ERASESUSPEND          0x75   // Erase Suspend
#define S25FL_CMD_ERASERESUME           0x7A   // Erase Resume
#define S25FL_CMD_POWERDOWN             0xB9   // Deep Power Down
#define S25FL_CMD_CRMR                  0x99   // Software Reset

// Read Instructions
#define S25FL_CMD_FREAD                 0x0B   // Fast Read
#define S25FL_CMD_FREADDUALOUT          0x3B   // Fast Read Dual Output
#define S25FL_CMD_FREADDUALIO           0xBB   // Fast Read Dual I/O
#define S25FL_CMD_FREADQUADOUT          0x6B   // Fast Read Quad Output
#define S25FL_CMD_FREADQUADIO           0xEB   // Fast Read Quad I/O
//#define S25FL_CMD_WREADQUADIO         0xE7   // Word Read Quad I/O
//#define S25FL_CMD_OWREADQUADIO        0xE3   // Octal Word Read Quad I/O
// ID/Security Instructions
#define S25FL_CMD_RPWRDDEVID            0xAB   // Release Power Down/Device ID
//#define S25FL_CMD_MANUFDEVID          0x90   // Manufacturer/Device ID
//#define S25FL_CMD_MANUFDEVID2         0x92   // Manufacturer/Device ID by Dual I/O
#define S25FL_CMD_MANUFDEVID4           0xAF   // Manufacturer/Device ID by Quad I/O
#define S25FL_CMD_JEDECID               0x9F   // JEDEC ID
#define S25FL_CMD_READUNIQUEID          0x4B   // Read Unique ID

#define S25FL_ID_LEN                    3

#define READY_TIMEOUT                   2000

typedef enum
{
    CS_ENABLE = 0,
    CS_DISABLE,
} csState_t;

typedef enum
{
    S64MB,
    S128MB,
    S256MB,
} s25fl_size_t;

typedef void (*csFunction_t)(csState_t);
typedef bool (*spiRead_t)(uint8_t*, uint32_t);
typedef void (*spiWrite_t)(uint8_t*, uint32_t);
typedef void (*spiWriteByte_t)(uint8_t);
typedef uint8_t (*spiReadRegister_t)(uint8_t);
typedef void (*delayFnc_t)(uint32_t);

typedef struct
{
    csFunction_t chip_select_ctrl;
    spiRead_t spi_read_fnc;
    spiWrite_t spi_write_fnc;
    spiWriteByte_t spi_writeByte_fnc;
    spiReadRegister_t spi_read_register;
    delayFnc_t delay_fnc;
    s25fl_size_t memory_size;
} s25fl_t;


bool S25FL_InitDriver(s25fl_t config);
uint8_t S25FL_readStatus();
uint32_t S25FL_readDevID();
void S25FL_writeEnable (bool enable);
uint32_t S25FL_readBuffer (uint32_t address, uint8_t *buffer, uint32_t len);
bool S25FL_eraseSector (uint32_t sectorNumber);
uint32_t S25FL_writeBuffer(uint32_t address, uint8_t *buffer, uint32_t len);
uint32_t S25FL_writePage (uint32_t address, uint8_t *buffer, uint32_t len, bool fastquit);
int32_t S25FL_pageSize();
int8_t S25FL_addressSize();
int32_t S25FL_numPages();

#endif // _S25FL_H_
