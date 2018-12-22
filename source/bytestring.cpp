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
// bytestring.cpp
//
// General Description
//
// This module is a part of an application to read the data from Electronic Meters for Electric Power
//
// In Germany they are called EHZ. Hence the name of the module an variables.
//
// The EHZ transmits data via an infrared interface. The protocoll is described in:
//
//  http://www.emsycon.de/downloads/SML_081112_103.pdf
//
// In SML wan so called OCTED is used as string. This is a charachter array with an associated lenght
// The std::string fullfills all requirements for such a byte sequence.
// Important is that also non printable characters, including 0, maybe a part of sich a string
//

#include "bytestring.hpp"
#include "mytypes.hpp"
#include "userinterface.hpp"

// ------------------------------------------------------------------------------------------------------------------------------------------------
// 1. Get the time of now and return it as Time_t and put it in the given parameter as string

	// Helper Function. Get the time of now, format it and put it into the given string
	time_t getNowTime(std::string &resultNowTime)
	{
		// This is just a magic number, big enough to hold the formated time string
		const size_t nowStringMaxSize = 65U;
		// Since used functions need a char * / char[] we have to go with that in the beginning
		mchar nowString[nowStringMaxSize];
		// Temp Storage for time functions	
		time_t nowTime;
		const tm *tmp;
		
		//Get time of now
		nowTime = time(null<time_t*>() );
		// Localize
		tmp = localtime(&nowTime);
		// And format it to out needs
		//lint --e(920)
		(void)strftime(&nowString[0], nowStringMaxSize, "%d.%m.%y %H:%M:%S", tmp);
		// Store char array in string
		resultNowTime = &nowString[0];
		// And return the value
		return nowTime;
	}

// ------------------------------------------------------------------------------------------------------------------------------------------------
// 2. Convert a SML Byte STring in something readable (printable)

	// The SML byte string can contain non printable characters. If so, then this functions will convert the 
	// complete string into ascii coded hex. This can be printed in any case
	void convertSmlByteStringNonePrintableCharacters(const SmlByteString &sbsIn, std::string &sbsOut)
	{
		// We will do the conversion to hex with a simple look up table
		const mchar hexToByteLookup[] = "0123456789ABCDEF";
		std::string::size_type i;
		// Could be that the whole string is printable in any way
		boolean isPrintable = true;
		
		// Initialize the output string	
		sbsOut.clear();
		
		// Now check any character in the source stirng
		for ( i=null<std::string::size_type>(); i<sbsIn.length(); ++i)
		{
			// Is it NOT printable?
			//lint -e{921,931}
			if (null<sint>() == isprint(static_cast<sint>(sbsIn[i])))
			{
				// Remember this
				isPrintable = false;
				// STop further evealuation of source bytes. Because, if we have at least on non prinatble
				// byte, we will convert the complete string in any case
				break;
			}
			else
			{
				// Charachter was printable. SO add it to the output std:string
				sbsOut += (sbsIn[i]);
			}
		}
		// If 1 character was not printable
		if (!isPrintable)
		{

		
			// Re initialize the output string and discard what we have possibly read before
			sbsOut.clear();
			// This will be the buffer containing one converted byte
			// It is initialized with a 3 byte empty string. We will insert byte 0 and 1
			// and retain the space at the 3rd postion. The last byte contains the 0 (end of string)
			
			mchar buf[4];
			// Temporary storage for byte of input string
				buf[2] = null<mchar>();		
			
			// For all bytes in the input string		
			for ( i=null<std::string::size_type>(); i<sbsIn.length(); ++i)
			{	
				
				//lint -e{1920,926,927,1960,732,915}
				const uint inByte = sbsIn[i];
				//ui << "convertSmlByteStringNonePrintableCharacters not printable:   " << sbsIn.length()<< " uint Byte: " << inByte << std::endl;	



				// COnvert it to a 2 digit hex using the lookup table
				buf[0] = hexToByteLookup[(inByte >> 4)];
				buf[1] = hexToByteLookup[(inByte & 0x0FU)];
				// Add result to output std::string	
				sbsOut += &buf[0];
				
				//ui << "buf: " << buf << "    sbsOut: " << sbsOut << std::endl;
			
			}
		}
	}

	// ------------------------------------------------------------------------------------------------------------------------------------------------
// 2. Convert a SML Byte STring in something readable (printable)

	// The SML byte string can contain non printable characters. If so, then this functions will convert the 
	// complete string into ascii coded hex. This can be printed in any case
	std::string convertSmlByteStringToHex(const SmlByteString &sbsIn)
	{
		// We will do the conversion to hex with a simple look up table
		const mchar hexToByteLookup[] = "0123456789ABCDEF";
		std::string::size_type i;
		
		// Initialize the output string	
		std::string sbsOut;
	
		// This will be the buffer containing one converted byte
		// It is initialized with a 3 byte empty string. We will insert byte 0 and 1
		// and retain the space at the 3rd postion. The last byte contains the 0 (end of string)
		//lint -e{1960,915}
		static mchar buf[4] = {' ',' ',' ',0};
		
		// For all bytes in the input string		
		for ( i=null<std::string::size_type>(); i<sbsIn.length(); ++i)
		{	
			//lint -e{1920,926,927,1960,732,915}
			const uint inByte = sbsIn[i];
			
			// COnvert it to a 2 digit hex using the lookup table
			buf[0] = hexToByteLookup[(inByte >> 4)];
			buf[1] = hexToByteLookup[(inByte & 0x0FU)];
			// Add result to output std::string	
			sbsOut += &buf[0];
		}
		//lint -e{1901,1911}
		return sbsOut;
	}
