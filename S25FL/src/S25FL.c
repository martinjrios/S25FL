/*
 *  S25FL064L.c
 *
 *  Created on: 17-09-2021
 *  Author: Martin Rios - jrios@fi.uba.ar
 * 
 */

#include "S25FL.h"
#include <stddef.h>

static s25fl_t s25fl;

// Parametros para una memoria de 64 Mbits
static int32_t pagesize = 256;
static int8_t addrsize = 24;
static int32_t pages = 32768;
static uint32_t totalsize; // 8 MBytes

static bool S25FL_waitForReady(uint32_t timeout);

/*************************************************************************************************
	 *  @brief      Inicializacion del driver S25FL
     *
     *  @details    Se copian los punteros a funciones pasados por argumentos a la estructura interna
     *              del driver, la cual no puede ser accedida por el resto del programa            
     *   	
	 *  @param		config	Estructura de configuracion para el driver.
	 *  @return     True si se inicializo correctamente.
***************************************************************************************************/
bool S25FL_InitDriver(s25fl_t config)
{
    if(config.chip_select_ctrl != NULL)
        s25fl.chip_select_ctrl = config.chip_select_ctrl;
    else return false;

    if(config.spi_read_fnc != NULL)
        s25fl.spi_read_fnc = config.spi_read_fnc;
    else return false;

    if(config.spi_write_fnc != NULL)
        s25fl.spi_write_fnc = config.spi_write_fnc;
    else return false;

    if(config.spi_writeByte_fnc != NULL)
        s25fl.spi_writeByte_fnc = config.spi_writeByte_fnc;
    else return false;

    if(config.spi_read_register != NULL)
        s25fl.spi_read_register = config.spi_read_register;
    else return false;

    if(config.delay_fnc != NULL)
        s25fl.delay_fnc = config.delay_fnc;
    else return false;

    switch(s25fl.memory_size)
    {
        case S64MB:
            pagesize = 256;
            addrsize = 24;
            pages = 32768;            
            break;
        
        case S128MB:
            pagesize = 256;
            addrsize = 24;
            pages = 65536;
            break;
        
        case S256MB:
            pagesize = 256;
            addrsize = 24;
            pages = 131072;
            break;    

        default:
            return false;
            break;                    
    }
    totalsize = pages * pagesize;

    return true;
}

/**************************************************************************/
/*! 
    @brief      Lee el registro de estado de la memoria.

    @return     0 - Memoria desocupada y escritura deshabilitada.
                1 - Si la memoria esta ocupada.
                2 - Si esta habilitada la escritura y no esta ocupada.
                3 - Si esta habilitada la escritura y esta ocupada.
*/
/**************************************************************************/
uint8_t S25FL_readStatus()
{
    uint8_t status = 0;
    uint8_t reg;
    uint8_t rxBuff[1];

    reg = S25FL_CMD_READSTAT1;
    s25fl.chip_select_ctrl(CS_ENABLE);
    s25fl.spi_writeByte_fnc(reg);
    s25fl.spi_read_fnc(rxBuff, 1);
    s25fl.chip_select_ctrl(CS_DISABLE);

    status = rxBuff[0];
    return (status & (SPIFLASH_STAT_BUSY | SPIFLASH_STAT_WRTEN));
}

/**************************************************************************/
/*! 
    @brief      Lee el ID de la memoria guardado en un registro no volatil.

    @return     El ID de 4 bytes del dispositvo.
*/
/**************************************************************************/
uint32_t S25FL_readDevID()
{
    uint32_t devId = 0;
    uint8_t reg;
    uint8_t rxBuff[4];

    reg = S25FL_CMD_JEDECID;
    s25fl.chip_select_ctrl(CS_ENABLE);
    s25fl.spi_writeByte_fnc(reg);
    s25fl.spi_read_fnc(rxBuff, 4);
    s25fl.chip_select_ctrl(CS_DISABLE);

    devId = (((uint32_t)rxBuff[0])<<16) + (((uint32_t)rxBuff[1])<<8) + ((uint32_t)rxBuff[2]);
    return devId;
}

/**************************************************************************/
/*! 
    @brief      Habilita la escritura.

    @param[in]  enable
                True habilita, false deshabilita la escritura.
*/
/**************************************************************************/
void S25FL_writeEnable (bool enable)
{
    uint8_t reg;

    reg = enable ? S25FL_CMD_WRITEENABLE : S25FL_CMD_WRITEDISABLE;

    s25fl.chip_select_ctrl(CS_ENABLE);
    s25fl.spi_writeByte_fnc(reg);
    s25fl.chip_select_ctrl(CS_DISABLE);
}

/**************************************************************************/
/*! 
    @brief      Lee la cantidad de bytes especificada desde la direccion 
                suministrada.

    Esta funcion leera uno o mas bytes comenzando desde la direccion
    suministrada.

    @param[in]  address
                La direccion de 24 bits donde comenzara la lectura.
    @param[out] *buffer
                Puntero al buffer donde se guardaran los datos leidos.
    @param[in]  len
                Longitud del buffer.
*/
/**************************************************************************/
uint32_t S25FL_readBuffer (uint32_t address, uint8_t *buffer, uint32_t len)
{
    uint32_t a = 0, i = 0;
    uint8_t reg, txData[S25FL_MAX_ADDRESS_SIZE];

    // Se chequea que la direccion sea valida
    if (address >= totalsize)
    {
        return 0;
    }
    
    s25fl.chip_select_ctrl(CS_ENABLE);

    reg = SPIFLASH_SPI_DATAREAD;
    s25fl.spi_writeByte_fnc(reg);   // Se envia el comando de lectura

    if (addrsize == 24) // 24 bit addr
    { 
        txData[0] = (address >> 16) & 0xFF;     // address upper 8
        txData[1] = (address >> 8) & 0xFF;      // address mid 8
        txData[2] = (address) & 0xFF;           // address lower 8

        s25fl.spi_write_fnc(txData, 3);     // Escribimos los 3 bytes de la direccion
    }
    else // (addrsize == 16) // Se asume que la direccion es de 16 bit 
    { 
        txData[0] = (address >> 8) & 0xFF;      // address high 8
        txData[1] = (address) & 0xFF;           // address lower 8        

        s25fl.spi_write_fnc(txData, 2);     // Escribimos los 2 bytes de la direccion
    }

    // En caso de sobrepasar la capacidad maxima de la memoria, se trunca
    if ((address+len) > totalsize) 
    {
        len = totalsize - address;
    }

    s25fl.spi_read_fnc(buffer, len);    // Se leen los datos del puerto spi

    s25fl.chip_select_ctrl(CS_DISABLE);

    return len; // Se devuelve la cantidad de bytes leidos
}

/**************************************************************************/
/*! 
    @brief      Espera a que la memoria flash indique que esta lista (no ocupada)
                o hasta que se termine el tiempo de espera.

    @param[in]  timeout
                El tiempo de espera maximo.

    @return     True si la flash esta lista, false si esta ocupada
*/
/**************************************************************************/
static bool S25FL_waitForReady(uint32_t timeout)
{
  uint8_t status;

  while ( timeout > 0 )
  {
    status = S25FL_readStatus() & SPIFLASH_STAT_BUSY;
    if (status == 0)
    {
      return true;
    }
    s25fl.delay_fnc(1);
    timeout--;
  }

  return false;
}

/**************************************************************************/
/*! 
    @brief      Borra el contenido de un sector de la flash.

    @param[in]  sectorNumber
                El numero de sector a borrar (comienza en cero)
*/
/**************************************************************************/
bool S25FL_eraseSector (uint32_t sectorNumber)
{
    uint8_t reg, txData[S25FL_MAX_ADDRESS_SIZE];
    
    // Se chequea que sea un sector valido
    if (sectorNumber >= S25FL_SECTORS) return false;

    // Se espera hasta que el dispositivo este listo o a que se agote el tiempo de espera
    if (!S25FL_waitForReady(READY_TIMEOUT))    return false;

    // Se habilita la escritura
    S25FL_writeEnable (true);

    // Se chequea que se haya habilitado la escritura
    uint8_t status;
    status = S25FL_readStatus();
    if (!(status & SPIFLASH_STAT_WRTEN))
    {
        return false;
    }

    uint32_t address = sectorNumber * S25FL_SECTORSIZE;
    s25fl.chip_select_ctrl(CS_ENABLE);
    
    // Se envia el comando para borrar el sector
    reg = S25FL_CMD_SECTERASE4;
    s25fl.spi_writeByte_fnc(reg);
    
    txData[0] = (address >> 16) & 0xFF;     // address upper 8
    txData[1] = (address >> 8) & 0xFF;      // address mid 8
    txData[2] = (address) & 0xFF;           // address lower 8   

    s25fl.spi_write_fnc(txData, 3);     // Escribimos los 3 bytes de la direccion

    s25fl.chip_select_ctrl(CS_DISABLE);

    // Se espera hasta que el dispositivo se desocupe antes de retornar.
    // Segun la hoja de datos esto puede demorar hasta 400 ms.
    if (!S25FL_waitForReady(500))    return false;

    return true;
}

/**************************************************************************/
/*! 
    @brief      Escribe un flujo de datos continuo que automaticamente
                puede cruzar de una pagina a otra.      
                
    @note       Antes de escribir los datos, asegurarse que los sectores
                correspondientes han sido borrados, de otro modo, los datos
                no tendrian sentido.      

    @param[in]  address
                La direccion de 24 bits donde comenzara la escritura.
    @param[out] *buffer
                Puntero al buffer de los datos a escribir.
    @param[in]  len
                Longitud del buffer, dentro de los limites de la capacidad
                de la flash.
*/
/**************************************************************************/
uint32_t S25FL_writeBuffer(uint32_t address, uint8_t *buffer, uint32_t len)
{
    uint32_t bytestowrite;
    uint32_t bufferoffset;
    uint32_t results;
    uint32_t byteswritten;

    // Las validaciones de address y len se realizaran en la funcion de escribir
    // pagina por lo que no tiene sentido duplicarlas aca.

    // Si los datos estan solo en una sola pagina, se escribe esa pagina directamente
    if ((address % pagesize) + len <= pagesize)
    {
        return S25FL_writePage(address, buffer, len, false);
    }

    // Escritura de multiples paginas
    byteswritten = 0;
    bufferoffset = 0;
    while(len)
    {
        // Se determina la cantidad de bytes necesarios a escribir en esta pagina
        bytestowrite = pagesize - (address % pagesize);
        // Se escribe la pagina actual
        results = S25FL_writePage(address, buffer+bufferoffset, bytestowrite, false);
        byteswritten += results;
        
        // Si ocurrio algun error, se sale devolviendo la cantidad de bytes escritos hasta el momento
        if (!results)   return byteswritten;
        
        // Se ajustan los valores de la direccion, longitud y offset del buffer
        address += bytestowrite;
        len -= bytestowrite;
        bufferoffset+=bytestowrite;
        
        // Si es la ultima pagina, se escribe y se sale, si no,
        // se sigue en el loop con la proxima pagina.
        if (len <= pagesize)
        {
            // Se escriben los ultimos bytes en la pagina y se sale
            results = S25FL_writePage(address, buffer+bufferoffset, len, false);
            byteswritten += results;

            // Si ocurrio algun error, se sale devolviendo la cantidad de bytes escritos hasta el momento
            if (!results)   return byteswritten;
            
            len = 0;    // Se setea len en cero para salir del loop
        }
    }

    // Se devuelve la cantidad de bytes escritos
    return byteswritten;
}

/**************************************************************************/
/*! 
    @brief      Escribe hasta 256 bytes de datos en la pagina especificada.
                
    @note       Antes de escribir los datos a la pagina, asegurarse que el
                sector de 4k que contiene la pagina especifica ha sido
                borrado, de otro modo, los datos no tendrian sentido.

    @param[in]  address
                La direccion de 24 bits donde comenzara la escritura.
    @param[in] *buffer
                Puntero al buffer de los datos a escribir.
    @param[in]  len
                Longitud del buffer. El valor debe estar entre 1 y 256.                
    @param[in]  fastquit
                Si es true, la funcion retorna sin esperar a que el
                dispositivo este disponible nuevamente.
*/
/**************************************************************************/
uint32_t S25FL_writePage (uint32_t address, uint8_t *buffer, uint32_t len, bool fastquit)
{
    uint8_t status;
    uint8_t reg, txData[S25FL_MAX_ADDRESS_SIZE];

    // Se chequea que la direccion sea valida
    if (address >= S25FL_MAXADDRESS)
    {
        return 0;
    }

    // Se chequea que la longitud de los datos no supere el tamaño de la pagina
    if (len > pagesize)
    {
        return 0;
    }

    // Se chequea que los datos no sean escritos mas alla de los limites de la pagina
    if ((address % pagesize) + len > pagesize)
    {
        // Si se trata de escribir en una pagina despues del ultimo byte,
        // este dato caera al principio de la pagina, mezclandose con lo que
        // ya habia.
        return 0;
    }

    // Se habilita la escritura
    s25fl.chip_select_ctrl(CS_ENABLE);
    s25fl.spi_writeByte_fnc(S25FL_CMD_WRITEENABLE);
    s25fl.chip_select_ctrl(CS_DISABLE);

    s25fl.delay_fnc(1);     // Delay para que termine de realizar el chequeo del bit de escritura
    s25fl.chip_select_ctrl(CS_ENABLE);

    if (addrsize == 24) // Se envia el comando de escritura de pagina seguido de la direccion de 24 bits
    {       
        reg = S25FL_CMD_PAGEPROG;
        s25fl.spi_writeByte_fnc(reg);
        
        txData[0] = (address >> 16) & 0xFF;     // address upper 8
        txData[1] = (address >> 8) & 0xFF;      // address mid 8

        if (len == pagesize)
        {
            // Si la longitud es igual al tamaño de la pagina, los ultimos 8 bits
            // deben ser 0 para que esten dentro de los limites de la pagina
            txData[2] = 0;                      // address lower 8 
        }
        else
        {
            txData[2] = (address) & 0xFF;       // address lower 8 
        }

        s25fl.spi_write_fnc(txData, 3);         // Escribimos los 3 bytes de la direccion
    } 
    else if (addrsize == 16) // Se envia el comando de escritura de pagina seguido de la direccion de 16 bits
    {
        reg = S25FL_CMD_PAGEPROG;
        s25fl.spi_writeByte_fnc(reg);
        
        txData[0] = (address >> 8) & 0xFF;      // address upper 8
        txData[1] = (address) & 0xFF;           // address lower 8

        s25fl.spi_write_fnc(txData, 2);         // Escribimos los 2 bytes de la direccion
    } 
    else 
    {
        return 0;
    }

    // Se envian los datos
    s25fl.spi_write_fnc(buffer, len); 

    // La escritura ocurre luego de que CS se ponga en alto
    s25fl.chip_select_ctrl(CS_DISABLE);

    if (! fastquit) {
        // Se espera hasta que el dispositivo este listo
        s25fl.delay_fnc(5);
    }

    return(len);
}

/**************************************************************************/
/*! 
    @return     El tamaño de pagina de la flash.
*/
/**************************************************************************/
int32_t S25FL_pageSize()
{
    return pagesize;
}

/**************************************************************************/
/*! 
    @return     La cantidad de bits de las direcciones de memoria.
*/
/**************************************************************************/
int8_t S25FL_addressSize()
{
    return addrsize;
}

/**************************************************************************/
/*! 
    @return     El numero total de paginas de la flash.
*/
/**************************************************************************/
int32_t S25FL_numPages()
{
    return pages;
}