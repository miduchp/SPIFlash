/* Arduino SPIFlash Library v.3.0.0
 * Copyright (C) 2017 by Prajwal Bhattaram
 * Created by Prajwal Bhattaram - 02/05/2017
 *
 * This file is part of the Arduino SPIFlash Library. This library is for
 * Winbond NOR flash memory modules. In its current form it enables reading
 * and writing individual data variables, structs and arrays from and to various locations;
 * reading and writing pages; continuous read functions; sector, block and chip erase;
 * suspending and resuming programming/erase and powering down for low power operation.
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License v3.0
 * along with the Arduino SPIFlash Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "SPIFlash.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//     Private functions used by read, write and erase operations     //
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

//Double checks all parameters before calling a read or write. Comes in two variants
//Takes address and returns the address if true, else returns false. Throws an error if there is a problem.
bool SPIFlash::_prep(uint8_t opcode, uint32_t _addr, uint32_t size) {
  switch (opcode) {
    case PAGEPROG:
    #ifndef HIGHSPEED
    if(!_addressCheck(_addr, size) || !_notBusy() || !_writeEnable() || !_notPrevWritten(_addr, size)) {
      return false;
    }
    #else
    if (!_addressCheck(_addr, size) || !_notBusy() || !_writeEnable()) {
      return false;
    }
    #endif
    return true;
    break;

    case ERASEFUNC:
    _currentAddress = _addr;
    if(!_notBusy()||!_writeEnable()) {
      return false;
    }
    return true;
    break;

    default:
    if (!_addressCheck(_addr, size) || !_notBusy()){
      return false;
    }
    return true;
    break;
  }
}

// Transfer Address.
bool SPIFlash::_transferAddress(void) {
  _nextByte(_currentAddress >> 16);
  _nextByte(_currentAddress >> 8);
  _nextByte(_currentAddress);
  return true;
}

bool SPIFlash::_startSPIBus(void) {
  #ifndef SPI_HAS_TRANSACTION
      noInterrupts();
  #endif

  #if defined (ARDUINO_ARCH_SAM)
    _dueSPIInit(DUE_SPI_CLK);
  #elif defined (ARDUINO_ARCH_SAMD)
    #ifdef SPI_HAS_TRANSACTION
      _spi->beginTransaction(_settings);
    #else
      _spi->setClockDivider(SPI_CLOCK_DIV_4)
      _spi->setDataMode(SPI_MODE0);
      _spi->setBitOrder(MSBFIRST);
    #endif
  #else
    #if defined (ARDUINO_ARCH_AVR)
      //save current SPI settings
      _SPCR = SPCR;
      _SPSR = SPSR;
    #endif
    #ifdef SPI_HAS_TRANSACTION
      SPI.beginTransaction(_settings);
    #else
      SPI.setClockDivider(SPI_CLOCK_DIV_4)
      SPI.setDataMode(SPI_MODE0);
      SPI.setBitOrder(MSBFIRST);
    #endif
  #endif
  SPIBusState = true;
  return true;
}

//Initiates SPI operation - but data is not transferred yet. Always call _prep() before this function (especially when it involves writing or reading to/from an address)
bool SPIFlash::_beginSPI(uint8_t opcode) {
  if (!SPIBusState) {
    _startSPIBus();
  }
  CHIP_SELECT
  switch (opcode) {
    case READDATA:
    _nextByte(opcode);
    _transferAddress();
    break;

    case PAGEPROG:
    _nextByte(opcode);
    _transferAddress();
    break;

    case FASTREAD:
    _nextByte(opcode);
    _nextByte(DUMMYBYTE);
    _transferAddress();

    case SECTORERASE:
    _nextByte(opcode);
    _transferAddress();

    case BLOCK32ERASE:
    _nextByte(opcode);
    _transferAddress();

    case BLOCK64ERASE:
    _nextByte(opcode);
    _transferAddress();

    default:
    _nextByte(opcode);
    break;
  }
  return true;
}
//SPI data lines are left open until _endSPI() is called

//Reads/Writes next byte. Call 'n' times to read/write 'n' number of bytes. Should be called after _beginSPI()
uint8_t SPIFlash::_nextByte(uint8_t data) {
//#if defined (ARDUINO_ARCH_SAM)
//  return _dueSPITransfer(data);
//#else
  return xfer(data);
//#endif
}

//Reads/Writes next int. Call 'n' times to read/write 'n' number of bytes. Should be called after _beginSPI()
uint16_t SPIFlash::_nextInt(uint16_t data) {
#if defined (ARDUINO_ARCH_SAMD)
  return _spi->transfer16(data);
#else
  return SPI.transfer16(data);
#endif
}

//Reads/Writes next data buffer. Call 'n' times to read/write 'n' number of bytes. Should be called after _beginSPI()
void SPIFlash::_nextBuf(uint8_t opcode, uint8_t *data_buffer, uint32_t size) {
  uint8_t *_dataAddr = &(*data_buffer);
  switch (opcode) {
    case READDATA:
    #if defined (ARDUINO_ARCH_SAM)
      _dueSPIRecByte(&(*data_buffer), size);
    #elif defined (ARDUINO_ARCH_SAMD)
      _spi->transfer(&data_buffer[0], size);
    #else
      for (uint16_t i = 0; i < size; i++) {
        *_dataAddr = xfer(NULLBYTE);
        _dataAddr++;
      }
    #endif
    break;

    case PAGEPROG:
    #if defined (ARDUINO_ARCH_SAM)
      _dueSPISendByte(&(*data_buffer), size);
    #elif defined (ARDUINO_ARCH_SAMD)
      _spi->transfer(&(*data_buffer), size);
    #else
      SPI.transfer(&(*data_buffer), size);
      /*for (uint16_t i = 0; i < size; i++) {
        xfer(*_dataAddr);
        _dataAddr++;
      }*/
    #endif
    break;
  }
}

//Stops all operations. Should be called after all the required data is read/written from repeated _nextByte() calls
void SPIFlash::_endSPI(void) {
  CHIP_DESELECT
#ifdef SPI_HAS_TRANSACTION
  #if defined (ARDUINO_ARCH_SAMD)
    _spi->endTransaction();
  #else
    SPI.endTransaction();
  #endif
#else
  interrupts();
#endif
#if defined (ARDUINO_ARCH_AVR)
  SPCR = _SPCR;
  SPSR = _SPSR;
#endif
  SPIBusState = false;
}

// Checks if status register 1 can be accessed - used to check chip status, during powerdown and power up and for debugging
uint8_t SPIFlash::_readStat1(void) {
	_beginSPI(READSTAT1);
  uint8_t stat1 = _nextByte();
  CHIP_DESELECT
	return stat1;
}

// Checks if status register 2 can be accessed, if yes, reads and returns it
uint8_t SPIFlash::_readStat2(void) {
  _beginSPI(READSTAT2);
  uint8_t stat2 = _nextByte();
  stat2 = _nextByte();
  CHIP_DESELECT
  return stat2;
}

// Checks the erase/program suspend flag before enabling/disabling a program/erase suspend operation
bool SPIFlash::_noSuspend(void) {
  switch (_chip.manufacturerID) {
    case WINBOND_MANID:
    if(_readStat2() & SUS) {
      _troubleshoot(SYSSUSPEND);
  		return false;
    }
  	return true;
    break;

    case MICROCHIP_MANID:
    if(_readStat1() & WSE || _readStat1() & WSP) {
      _troubleshoot(SYSSUSPEND);
  		return false;
    }
  	return true;
  }

}

// Polls the status register 1 until busy flag is cleared or timeout
bool SPIFlash::_notBusy(uint32_t timeout) {
  _delay_us(WINBOND_WRITE_DELAY);
	uint32_t startTime = millis();

	do {
    state = _readStat1();
		if((millis()-startTime) > timeout){
      _troubleshoot(CHIPBUSY);
			return false;
		}
	} while(state & BUSY);
	return true;
}

//Enables writing to chip by setting the WRITEENABLE bit
bool SPIFlash::_writeEnable(uint32_t timeout) {
  uint32_t startTime = millis();
  //if (!(state & WRTEN)) {
    do {
      _beginSPI(WRITEENABLE);
      CHIP_DESELECT
      state = _readStat1();
      if((millis()-startTime) > timeout) {
        _troubleshoot(CANTENWRITE);
        return false;
      }
     } while (!(state & WRTEN)) ;
  //}
  return true;
}

//Disables writing to chip by setting the Write Enable Latch (WEL) bit in the Status Register to 0
//_writeDisable() is not required under the following conditions because the Write Enable Latch (WEL) flag is cleared to 0
// i.e. to write disable state:
// Power-up, Write Disable, Page Program, Quad Page Program, Sector Erase, Block Erase, Chip Erase, Write Status Register,
// Erase Security Register and Program Security register
bool SPIFlash::_writeDisable(void) {
	_beginSPI(WRITEDISABLE);
  CHIP_DESELECT
	return true;
}

//Checks the device ID to establish storage parameters
bool SPIFlash::_getManId(uint8_t *b1, uint8_t *b2) {
	if(!_notBusy())
		return false;
	_beginSPI(MANID);
  _nextByte();
  _nextByte();
  _nextByte();
  *b1 = _nextByte();
  *b2 = _nextByte();
  CHIP_DESELECT
	return true;
}

//Checks for presence of chip by requesting JEDEC ID
bool SPIFlash::_getJedecId(void) {
  if(!_notBusy()) {
  	return false;
  }
  _beginSPI(JEDECID);
	_chip.manufacturerID = _nextByte(NULLBYTE);		// manufacturer id
	_chip.memoryTypeID = _nextByte(NULLBYTE);		// memory type
	_chip.capacityID = _nextByte(NULLBYTE);		// capacity
  CHIP_DESELECT
  if (!_chip.manufacturerID || !_chip.memoryTypeID || !_chip.capacityID) {
    _troubleshoot(NORESPONSE);
    return false;
  }
  else {
    return true;
  }
}

bool SPIFlash::_getSFDP(void) {
  if(!_notBusy()) {
  	return false;
  }
  _currentAddress = 0x00;
  _beginSPI(READSFDP);
  _transferAddress();
  _nextByte(DUMMYBYTE);
  for (uint8_t i = 0; i < 4; i++) {
    _chip.sfdp += (_nextByte() << (8*i));
  }
  CHIP_DESELECT
  if (_chip.sfdp == 0x50444653) {
    //Serial.print("_chip.sfdp: ");
    //Serial.println(_chip.sfdp, HEX);
    return true;
  }
  else {
    return false;
  }
}

//Identifies the chip
bool SPIFlash::_chipID(void) {
  //Get Manfucturer/Device ID so the library can identify the chip
  //_getSFDP();
  if (!_getJedecId()) {
    return false;
  }

  if (_chip.manufacturerID == MICROCHIP_MANID) {
    uint8_t _stat1 = _readStat1();
    _stat1 &= 0xC3;
    _beginSPI(WRITESTATEN);
    CHIP_DESELECT
    _beginSPI(WRITESTAT);
    _nextByte(_stat1);
    CHIP_DESELECT
  }

  // If no capacity is defined in user code
#if !defined (CHIPSIZE)
  if (_chip.manufacturerID == WINBOND_MANID || _chip.manufacturerID == MICROCHIP_MANID || _chip.manufacturerID == CYPRESS_MANID) {
    //Identify capacity
    for (uint8_t i = 0; i < sizeof(_capID); i++) {
      if (_chip.capacityID == _capID[i]) {
        _chip.capacity = (_memSize[i]);
        _chip.eraseTime = _eraseTime[i];
      }
    }
    _chip.supported = true;
    return true;
  }
  else {
    _troubleshoot(UNKNOWNCAP); //Error code for unidentified capacity
    return false;
  }
#else
  // If a custom chip size is defined
  _chip.eraseTime = (400L *S);
  if (_chip.manufacturerID!= WINBOND_MANID && _chip.manufacturerID != MICROCHIP_MANID && _chip.manufacturerID != CYPRESS_MANID) {
    _chip.supported = false;// This chip is not officially supported
    _troubleshoot(UNKNOWNCHIP); //Error code for unidentified chip
    //while(1);         //Enable this if usercode is not meant to be run on unsupported chips
  }
  return true;
#endif
}

//Checks to see if page overflow is permitted and assists with determining next address to read/write.
//Sets the global address variable
bool SPIFlash::_addressCheck(uint32_t _addr, uint32_t size) {
  if (errorcode == UNKNOWNCAP || errorcode == NORESPONSE) {
    return false;
  }
	if (!_chip.eraseTime) {
    _troubleshoot(CALLBEGIN);
    return false;
	}

  //for (uint32_t i = 0; i < size; i++) {
  if (_addr + size >= _chip.capacity) {
  #ifdef DISABLEOVERFLOW
    _troubleshoot(OUTOFBOUNDS);
    return false;					// At end of memory - (!pageOverflow)
  #else
    _currentAddress = 0x00;
    return true;					// At end of memory - (pageOverflow)
  #endif
  }
  //}
  _currentAddress = _addr;
  return true;				// Not at end of memory if (address < _chip.capacity)
}

bool SPIFlash::_notPrevWritten(uint32_t _addr, uint32_t size) {
  _beginSPI(READDATA);
  for (uint32_t i = 0; i < size; i++) {
    if (_nextByte() != 0xFF) {
      CHIP_DESELECT;
      return false;
    }
  }
  CHIP_DESELECT
  return true;
}