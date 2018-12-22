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
// bytestring.hpp
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



#ifndef BYTESTRING_HPP
#define BYTESTRING_HPP

#include "mytypes.hpp"

// A SmlByteString as define in the SML specification can contain none printable characters
// Actually it is a character array. But since all operations on this object can be handled
// by std::string, we will use this type instead
//typedef std::string SmlByteString;

// The SML byte string can contain non printable characters. If so, then this functions will convert the 
// complete string into ascii coded hex. This can be printed in any case
extern void convertSmlByteStringNonePrintableCharacters(const SmlByteString &sbsIn, std::string &sbsOut);

// This is a helper function, that gets the time of now, formats it in put it into the given string
extern time_t getNowTime(std::string &resultNowTime);

extern std::string convertSmlByteStringToHex(const SmlByteString &sbsIn);


#endif
