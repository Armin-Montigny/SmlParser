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
// Lexical Analysis of SML File Data
// 
// General Description:
// 
// Data acquisition and parametrization of devices on low performance machines require
// a compact and easy communication protocol.
// 
// For example electronic meters for electricity have the need for easy communication 
// with the rest of the world.
// 
// The Smart Message Language (SML) has been developed based on that requirements.
// SML has a data structure which is described in 

// 
// http://www.emsycon.de/downloads/SML_081112_103.pdf
//
// See chapter 6
//
// The type length field encodes the type of the next bytes and their length
// So the TL Byte is always only 8bit and can have 256 values
// These values are hardcoded here. Then we can directly find out the next state
// and the number of bytes, by just using a simple "Jump" table
//
// Structure is defined in scanner.hpp



#include "scanner.hpp"


	// These are static functions that will be used to determine the next state, after a TL byte has been read
	// Some additional stuff, specific for a state will be done as needed.
	// ALl these function will be referenced via a function pointer in below table

	//lint -e{1960}  // this module is part of scanner
	using namespace ScannerInternal;

// ------------------------------------------------------------------------------------------------------------------------------------------------
// 1. Functions that react on TL Byte and set the next state of the scanner	
	
	
	
	// Simple function
	// Usually used for 1 Byte types
	// Simply stay in state
	ScannerBaseState* TlByteAnalysis::handleTlByteBasic(ScannerContextData &)
	{
		return getInstance<ScannerStateAnalyzeTl>();
	}

	// For TL bytes with Optional Tag
	// ANalyze next TL byte
	ScannerBaseState* TlByteAnalysis::handleTlByteOptional(ScannerContextData &scd)
	{
		scd.token.setValue();
		return getInstance<ScannerStateAnalyzeTl>();
	}

	// For invalid TL Bytes. Go back to Idle
	ScannerBaseState* TlByteAnalysis::handleTlByteBasicReset(ScannerContextData &)
	{
		return getInstance<ScannerStateIdle>();
	}

	// Read an Octet
	ScannerBaseState* TlByteAnalysis::handleTlByteOctet(ScannerContextData &scd)
	{
		scd.token.setValue();  									// Initialze octet byte array index pointer  to 0
		return getInstance<ScannerStateReadOctet>();  			// Goto next state. Read Octet bytes
	}

	// Read a boolen
	ScannerBaseState* TlByteAnalysis::handleTlByteBoolean(ScannerContextData &)
	{
		return getInstance<ScannerStateReadBoolean>();  		// Goto next state. Read Boolean
	}

	// Read a signed integer
	ScannerBaseState* TlByteAnalysis::handleTlByteSignedInteger(ScannerContextData &scd)
	{
		scd.isFirstSignedIntegerByte = true;  					// Because of big endian data we need special sign handling
		return getInstance<ScannerStateReadSignedInteger>();  	// Goto next state. Read value
	}

	// Read an unsigned integer
	ScannerBaseState* TlByteAnalysis::handleTlByteUnsignedInteger(ScannerContextData &scd)
	{
		scd.u64TempUnsignedInteger = 0ULL;  // Initial value. Needed for multibyte calculation of value
		return getInstance<ScannerStateReadUnsignedInteger>();  // Goto next state:Read vale
	}

	// Read a multibyte Octet						
	ScannerBaseState* TlByteAnalysis::handleTlByteMultiByteOctet(ScannerContextData &scd)
	{
		scd.multiByteNumberOfTlbyteRead = 1UL;  // We have read one TL field of potential more
		scd.multiByteLength = scd.token.getLength();  // Set initial length
		return getInstance<ScannerStateReadMultiByteOctet>(); // Goto next state. Read more TL bytes
	}

	// Read a multibyte Octet						
	ScannerBaseState* TlByteAnalysis::handleTlByteMultiByteList(ScannerContextData &scd)
	{
		scd.multiByteNumberOfTlbyteRead = 1UL;  // We have read one TL field of potential more
		scd.multiByteLength = scd.token.getLength();  // Set initial length
		return getInstance<ScannerStateReadMultiByteList>(); // Goto next state. Read more TL bytes
	}
	
// ------------------------------------------------------------------------------------------------------------------------------------------------
// 2. The lookup or "Jump"-Table

	const TlByteAnalysis::TLBT TlByteAnalysis::tlbt[256] = 
	{
		{ Token::END_OF_MESSAGE, 0UL, &handleTlByteBasic },             //0x00
		{ Token::OPTIONAL, 1UL, &handleTlByteOptional },                   //0x01
		{ Token::CONDITION_NOT_YET_DETECTED, 1UL, &handleTlByteOctet },                      //0x02
		{ Token::CONDITION_NOT_YET_DETECTED, 2UL, &handleTlByteOctet },                      //0x03
		{ Token::CONDITION_NOT_YET_DETECTED, 3UL, &handleTlByteOctet },                      //0x04
		{ Token::CONDITION_NOT_YET_DETECTED, 4UL, &handleTlByteOctet },                      //0x05
		{ Token::CONDITION_NOT_YET_DETECTED, 5UL, &handleTlByteOctet },                      //0x06
		{ Token::CONDITION_NOT_YET_DETECTED, 6UL, &handleTlByteOctet },                      //0x07
		{ Token::CONDITION_NOT_YET_DETECTED, 7UL, &handleTlByteOctet },                      //0x08
		{ Token::CONDITION_NOT_YET_DETECTED, 8UL, &handleTlByteOctet },                      //0x09
		{ Token::CONDITION_NOT_YET_DETECTED, 9UL, &handleTlByteOctet },                      //0x0A
		{ Token::CONDITION_NOT_YET_DETECTED, 10UL, &handleTlByteOctet },                     //0x0B
		{ Token::CONDITION_NOT_YET_DETECTED, 11UL, &handleTlByteOctet },                     //0x0C
		{ Token::CONDITION_NOT_YET_DETECTED, 12UL, &handleTlByteOctet },                     //0x0D
		{ Token::CONDITION_NOT_YET_DETECTED, 13UL, &handleTlByteOctet },                     //0x0E
		{ Token::CONDITION_NOT_YET_DETECTED, 14UL, &handleTlByteOctet },                     //0x0F
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x10
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x11
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x12
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x13
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x14
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x15
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x16
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x17
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x18
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x19
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x1A
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x1B
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x1C
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x1D
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x1E
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x1F
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x20
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x21
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x22
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x23
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x24
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x25
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x26
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x27
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x28
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x29
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x2A
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x2B
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x2C
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x2D
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x2E
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x2F
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x30
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x31
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x32
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x33
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x34
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x35
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x36
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x37
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x38
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x39
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x3A
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x3B
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x3C
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x3D
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x3E
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x3F
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x40
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x41
		{ Token::CONDITION_NOT_YET_DETECTED, 1UL, &handleTlByteBoolean },                  //0x42
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x43
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x44
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x45
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x46
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x47
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x48
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x49
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x4A
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x4B
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x4C
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x4D
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x4E
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x4F
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x50
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x51
		{ Token::CONDITION_NOT_YET_DETECTED, 1UL, &handleTlByteSignedInteger },     //0x52
		{ Token::CONDITION_NOT_YET_DETECTED, 2UL, &handleTlByteSignedInteger },     //0x53
		{ Token::CONDITION_NOT_YET_DETECTED, 3UL, &handleTlByteSignedInteger },     //0x54
		{ Token::CONDITION_NOT_YET_DETECTED, 4UL, &handleTlByteSignedInteger },     //0x55
		{ Token::CONDITION_NOT_YET_DETECTED, 5UL, &handleTlByteSignedInteger },     //0x56
		{ Token::CONDITION_NOT_YET_DETECTED, 6UL, &handleTlByteSignedInteger },     //0x57
		{ Token::CONDITION_NOT_YET_DETECTED, 7UL, &handleTlByteSignedInteger },     //0x58
		{ Token::CONDITION_NOT_YET_DETECTED, 8UL, &handleTlByteSignedInteger },     //0x59
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x5A
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x5B
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x5C
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x5D
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x5E
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x5F
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x60
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x61
		{ Token::CONDITION_NOT_YET_DETECTED, 1UL, &handleTlByteUnsignedInteger }, //0x62
		{ Token::CONDITION_NOT_YET_DETECTED, 2UL, &handleTlByteUnsignedInteger }, //0x63
		{ Token::CONDITION_NOT_YET_DETECTED, 3UL, &handleTlByteUnsignedInteger }, //0x64
		{ Token::CONDITION_NOT_YET_DETECTED, 4UL, &handleTlByteUnsignedInteger }, //0x65
		{ Token::CONDITION_NOT_YET_DETECTED, 5UL, &handleTlByteUnsignedInteger }, //0x66
		{ Token::CONDITION_NOT_YET_DETECTED, 6UL, &handleTlByteUnsignedInteger }, //0x67
		{ Token::CONDITION_NOT_YET_DETECTED, 7UL, &handleTlByteUnsignedInteger }, //0x68
		{ Token::CONDITION_NOT_YET_DETECTED, 8UL, &handleTlByteUnsignedInteger }, //0x69
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x6A
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x6B
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x6C
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x6D
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x6E
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x6F
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },       //0x70
		{ Token::LIST, 1UL, &handleTlByteBasic },                       //0x71
		{ Token::LIST, 2UL, &handleTlByteBasic },                       //0x72
		{ Token::LIST, 3UL, &handleTlByteBasic },                       //0x73
		{ Token::LIST, 4UL, &handleTlByteBasic },                       //0x74
		{ Token::LIST, 5UL, &handleTlByteBasic },                       //0x75
		{ Token::LIST, 6UL, &handleTlByteBasic },                       //0x76
		{ Token::LIST, 7UL, &handleTlByteBasic },                       //0x77
		{ Token::LIST, 8UL, &handleTlByteBasic },                       //0x78
		{ Token::LIST, 9UL, &handleTlByteBasic },                       //0x79
		{ Token::LIST, 10UL, &handleTlByteBasic },                      //0x7A
		{ Token::LIST, 11UL, &handleTlByteBasic },                      //0x7B
		{ Token::LIST, 12UL, &handleTlByteBasic },                      //0x7C
		{ Token::LIST, 13UL, &handleTlByteBasic },                      //0x7D
		{ Token::LIST, 14UL, &handleTlByteBasic },                      //0x7E
		{ Token::LIST, 15UL, &handleTlByteBasic },                      //0x7F
		{ Token::CONDITION_NOT_YET_DETECTED, 0UL, &handleTlByteMultiByteOctet },                      //0x80
		{ Token::CONDITION_NOT_YET_DETECTED, 1UL, &handleTlByteMultiByteOctet },                      //0x81
		{ Token::CONDITION_NOT_YET_DETECTED, 2UL, &handleTlByteMultiByteOctet},                      //0x82
		{ Token::CONDITION_NOT_YET_DETECTED, 3UL, &handleTlByteMultiByteOctet },                      //0x83
		{ Token::CONDITION_NOT_YET_DETECTED, 4UL, &handleTlByteMultiByteOctet },                      //0x84
		{ Token::CONDITION_NOT_YET_DETECTED, 5UL, &handleTlByteMultiByteOctet },                      //0x85
		{ Token::CONDITION_NOT_YET_DETECTED, 6UL, &handleTlByteMultiByteOctet },                      //0x86
		{ Token::CONDITION_NOT_YET_DETECTED, 7UL, &handleTlByteMultiByteOctet },                      //0x87
		{ Token::CONDITION_NOT_YET_DETECTED, 8UL, &handleTlByteMultiByteOctet },                      //0x88
		{ Token::CONDITION_NOT_YET_DETECTED, 9UL, &handleTlByteMultiByteOctet },                      //0x89
		{ Token::CONDITION_NOT_YET_DETECTED, 10UL, &handleTlByteMultiByteOctet },                      //0x8A
		{ Token::CONDITION_NOT_YET_DETECTED, 11UL, &handleTlByteMultiByteOctet },                      //0x8B
		{ Token::CONDITION_NOT_YET_DETECTED, 12UL, &handleTlByteMultiByteOctet },                      //0x8C
		{ Token::CONDITION_NOT_YET_DETECTED, 13UL, &handleTlByteMultiByteOctet },                      //0x8D
		{ Token::CONDITION_NOT_YET_DETECTED, 14UL, &handleTlByteMultiByteOctet },                      //0x8E
		{ Token::CONDITION_NOT_YET_DETECTED, 15UL, &handleTlByteMultiByteOctet },                      //0x8F
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x90
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x91
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x92
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x93
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x94
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x95
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x96
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x97
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x98
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x99
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x9A
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x9B
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x9C
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x9D
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x9E
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0x9F
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xA0
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xA1
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xA2
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xA3
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xA4
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xA5
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xA6
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xA7
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xA8
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xA9
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xAA
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xAB
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xAC
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xAD
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xAE
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xAF
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xB0
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xB1
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xB2
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xB3
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xB4
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xB5
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xB6
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xB7
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xB8
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xB9
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xBA
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xBB
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xBC
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xBD
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xBE
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xBF
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xC0
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xC1
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xC2
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xC3
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xC4
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xC5
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xC6
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xC7
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xC8
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xC9
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xCA
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xCB
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xCC
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xCD
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xCE
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xCF
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xD0
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xD1
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xD2
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xD3
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xD4
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xD5
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xD6
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xD7
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xD8
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xD9
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xDA
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xDB
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xDC
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xDD
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xDE
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xDF
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xE0
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xE1
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xE2
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xE3
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xE4
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xE5
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xE6
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xE7
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xE8
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xE9
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xEA
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xEB
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xEC
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xED
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xEE
		{ Token::CONDITION_ERROR, 0UL, &handleTlByteBasicReset },      //0xEF
		{ Token::CONDITION_NOT_YET_DETECTED, 0UL, &handleTlByteMultiByteList },      //0xF0
		{ Token::CONDITION_NOT_YET_DETECTED, 1UL, &handleTlByteMultiByteList },      //0xF1
		{ Token::CONDITION_NOT_YET_DETECTED, 2UL, &handleTlByteMultiByteList },      //0xF2
		{ Token::CONDITION_NOT_YET_DETECTED, 3UL, &handleTlByteMultiByteList },      //0xF3
		{ Token::CONDITION_NOT_YET_DETECTED, 4UL, &handleTlByteMultiByteList },      //0xF4
		{ Token::CONDITION_NOT_YET_DETECTED, 5UL, &handleTlByteMultiByteList },      //0xF5
		{ Token::CONDITION_NOT_YET_DETECTED, 6UL, &handleTlByteMultiByteList },      //0xF6
		{ Token::CONDITION_NOT_YET_DETECTED, 7UL, &handleTlByteMultiByteList },      //0xF7
		{ Token::CONDITION_NOT_YET_DETECTED, 8UL, &handleTlByteMultiByteList },      //0xF8
		{ Token::CONDITION_NOT_YET_DETECTED, 9UL, &handleTlByteMultiByteList },      //0xF9
		{ Token::CONDITION_NOT_YET_DETECTED, 10UL, &handleTlByteMultiByteList },      //0xFA
		{ Token::CONDITION_NOT_YET_DETECTED, 11UL, &handleTlByteMultiByteList },      //0xFB
		{ Token::CONDITION_NOT_YET_DETECTED, 12UL, &handleTlByteMultiByteList },      //0xFC
		{ Token::CONDITION_NOT_YET_DETECTED, 13UL, &handleTlByteMultiByteList },      //0xFD
		{ Token::CONDITION_NOT_YET_DETECTED, 14UL, &handleTlByteMultiByteList },      //0xFE
		{ Token::CONDITION_NOT_YET_DETECTED, 15UL, &handleTlByteMultiByteList }       //0xFF
	};		



	
// ------------------------------------------------------------------------------------------------------------------------------------------------
// 3. The scan function for a TL byte

	//lint -e{1961,9003}
	// Gets the TL byte as parameter, interpretes that and call the corresponding function from the lookup table
	
	ScannerBaseState* TlByteAnalysis::scan(const EhzDatabyte ehzDatabyte, ScannerContextData &scd)
	{
		//lint -e{921}       Note 921: cast from unsigned char to unsigned int
		// Get pointer to Element in the lookup table by using the read TL byte
		const TLBT * const ptlbt = &tlbt[static_cast<uint>(ehzDatabyte)];
		
		// Set the type and length of the token from the TL Byte
		scd.token.setTokenTypeAndLength(ptlbt->tokenType, ptlbt->tokenLength);
		// If nexessary, initialize the loop counter
		// so that the scanner knows, how many bytes it must read for this type
		scd.ehzDatabyteReadLoopCounter = ptlbt->tokenLength;
		return (*ptlbt->handleTlByte)(scd);
	}
	 





