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
#ifndef MYTYPES_HPP
#define MYTYPES_HPP

#include <string>


// --------------------------------------------------------------------------------------------------------------------------------------
// 1. Tyepsafe Null

//lint -save -e921
template <typename T>
const T null(void) 
{
	return static_cast<T>(0);
}




//lint -restore

// --------------------------------------------------------------------------------------------------------------------------------------
// 2. Typedefs for Standard types

typedef char mchar;

typedef unsigned char u8;
typedef unsigned char byte;
typedef int sint;
typedef unsigned int uint;


typedef unsigned short u16;
typedef unsigned long u32;
__extension__ typedef unsigned long long u64;

typedef double mdouble;
typedef signed short s16;
typedef signed char s8;

typedef signed long s32;
__extension__ typedef signed long long s64;

typedef bool boolean;




// --------------------------------------------------------------------------------------------------------------------------------------
// 3. Project specific Typedefs 


typedef struct _SmlListLength
{
	u8 lenght;
} SmlListLength;


// EHZ measured data can be represented as string or as number
// If we do not want to read a value we can indicate that with Null
// In the configuration we will give each possible value one of the following prpoperties
typedef struct _EhzMeasuredDataType
{ 
	enum Type
	{
		Null,		// No date existing, no data required
		Number,		// Data is a number (double)
		String		// Data is a string (SmlByteString / std::string)
	};
}EhzMeasuredDataType;

typedef u8 EhzDatabyte;
typedef u32 TokenLength;
typedef sint Handle;                  	// Handle is a synonym for a Linux file descriptor

typedef std::string SmlByteString;


// --------------------------------------------------------------------------------------------------------------------------------------
// 4. Library specific Constants 


typedef short EventType;     			// Type of Events that will be monitored
//lint -save -e921
const EventType EventTypeIn = 			static_cast<EventType>(0x001);
const EventType EventTypeUrgentIn = 	static_cast<EventType>(0x002);
const EventType EventTypeOut = 			static_cast<EventType>(0x004);
const EventType EventTypeError = 		static_cast<EventType>(0x008);
const EventType EventTypeHangup = 		static_cast<EventType>(0x010);
const EventType EventTypeNotValid = 	static_cast<EventType>(0x020);
//lint -restore

// --------------------------------------------------------------------------------------------------------------------------------------
// 5. Project specific Constants 


// We are requesting/searching for max 4 measured values from our Ehz
const uint NumberOfEhzMeasuredData = 4U;

const u32 MAX_SML_STRING_LEN = 32UL;

const mchar charSTX = '\x002';
const mchar charETX = '\x003';
const mchar charUS = '\x01F';



extern sint globalDebugMode;

enum GlobalDebugMode
{
	DebugModeHeaderOnly,
	DebugModeError,
	DebugModeObis,
	DebugModeParseResult,
	
	DebugModeMax
};






#endif
