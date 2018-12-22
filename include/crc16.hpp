// -----------------------------------------------------------------------------------------------------
//
// SML Parser
//
// Copyright (C) 2018  Armin Montigny
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
// -----------------------------------------------------------------------------------------------------
//
// 
// Checksum calculation of an SML File or Message
// 
// Used Standard: CCITT-CRC16 (DIN EN 62056-46)
// Algorithm: CCITT-CRC16, x16 + x12 + x5 + 1, 0x1021 / 0x8408 / 0x8810.
// 
// In our program we are using 2 "crc16 calculators"
//  
//  "Crc16Calculator" is working in defined mode and is using the standard
//  initializer 0xFFFF for the running sum.
//   
//  According to the SML specification in
//  http://www.emsycon.de/downloads/SML_081112_103.pdf
//  2 crc 16 calculation types will be used.
//  
//  1. The whole datastream, including the ESC start sequence 27,27,27,27,1,1,1,1
//     (in hex: 0x1b 0x1b 0x1b 0x1b 0x01 0x01 0x01 0x01) and the stop sequence
//     but of course except the 2 last bytes which are the CRC itself
//  2. Each SML message will also be secured by a CRC16 checksum. It starts with
//     the first byte of the SML Message and ends with the last byte of the 
//     SML message body
// 
//  Additional infor for 1.;
//  As said, the SML-File is embedded in an ESC Start sequence and an ESC Stop sequence. 
//  So we know that an SML file start has been detetced only after having received
//  the ESC Start sequence. Then we want to start the crc16 calculator. But since
//  we already have read 8 bytes (27,27,27,27,1,1,1,1) we need to start with a
//  different inital value for the calculator which takes exactly those 8 bytes
//  into account.
//  
//  
//  Therefore we derive a 2nd class "Crc16CalculatorSmlStart" from "Crc16Calculator"
//  with the only difference of another initial value.
//  
//  The calculation needs to be synchronized with SML data start and stop conditions.
//  For this we use an "enable" flag, which can be set and reset using "start" and
//  "stop" functions.
//  
//  "update"  does the continuous calculation and "getResult" finalises the
//  calculation and returns the result.
//  
// 

#ifndef CRC16_HPP
#define CRC16_HPP

#include "mytypes.hpp"

typedef unsigned int crc16t;

// ----------------------------------------------------------------------------------------------------------------------------------
// 1. Declaration of main class that is needed to interface and use the functionality of this module
// ----------------------------------------------------------------------------------------------------------------------------------

// Standard crc16 calculation class
class Crc16Calculator
{
	public:
		Crc16Calculator(void) : crcRunningSum(0U),enable(false) {}		// Constructor. Disable running calculation
		
		virtual void start(void);					// Start calculation and initalize running sum
		void stop(void) {enable = false;}			// Stop calculation
		void update(const u8 smlbyte);					// Calculate checksum (running sum)

		crc16t getResult(void) const;					// Stop and finalize calculation. Return checksum
		
	protected:
		//lint -e{935}
		crc16t crcRunningSum;							// Running sum
		boolean enable;								// Flag for enabeling and disabeling calculation
		static const crc16t CRC16_COEFICIENT[256];		// Coeficients that are needed for CRC16 calculation

	private:
		static const crc16t CRC16_START_CALCULATION_VALUE;	// Start value (coeficient) fpr CRC16 calculation
};

// Crc16 calculation for situations where already 8 bytes of the 
// ESC Start sequence have been read
class Crc16CalculatorSmlStart : public Crc16Calculator
{
	public:
		Crc16CalculatorSmlStart(void) : Crc16Calculator() {}	// Use base class constructor only
		virtual void start(void);					// Overwrite base class function. Use own start value
	
	protected:
	private:
		// Start value for deffered CRC16 calculation
		static const crc16t CRC16_START_CALCULATION_VALUE_AFTER_SMLFILE_START;	
};






// ----------------------------------------------------------------------------------------------------------------------------------
// 2. Inline functions for classes
// ----------------------------------------------------------------------------------------------------------------------------------


	// ------------------------------------------------------------------------------------------------------------------------------
	// 2.1 Crc16Calculator
    // ------------------------------------------------------------------------------------------------------------------------------


	// Enable calculation and initialize running sum
	inline void Crc16Calculator::start(void)
	{
		crcRunningSum = CRC16_START_CALCULATION_VALUE;
		enable = true;
	}

	// Checksum calculation according to standard
	inline void Crc16Calculator::update(const u8 smlbyte)
	{
		if (enable)
		{
		
			//lint -e{921}
			crcRunningSum = (((crcRunningSum >> 8U) & 0xFFU) ^ 
				CRC16_COEFICIENT [(crcRunningSum ^ static_cast<crc16t>(smlbyte)) & 0xFFU]) & 0xFFFFU;
		}
	}

	// Finalisation of checksum calculation and returning the result
	inline crc16t Crc16Calculator::getResult(void) const
	{
		const crc16t tmp = (crcRunningSum ^ 0xFFFFU) & 0xFFFFU;
		return ((tmp & 0xFF00U) >> 8U) | ((tmp & 0xFFU) << 8U);
	}

	
	// ------------------------------------------------------------------------------------------------------------------------------
	// 2.1 Crc16CalculatorSmlStart
    // ------------------------------------------------------------------------------------------------------------------------------
	
	// Enable calculation and initialize running sum
	inline void Crc16CalculatorSmlStart::start(void)
	{
		crcRunningSum = CRC16_START_CALCULATION_VALUE_AFTER_SMLFILE_START;
		enable = true;
	}

 

#endif

