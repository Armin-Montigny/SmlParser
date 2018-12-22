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


#include "crc16.hpp"

// ----------------------------------------------------------------------------------------------------------------------------------
// 1. Definition of Constants
// ----------------------------------------------------------------------------------------------------------------------------------

// Coeficents needed for CRC16 calculation
const crc16t Crc16Calculator::CRC16_COEFICIENT [256] = 
{ 
0x0000U, 0x1189U, 0x2312U, 0x329bU, 0x4624U, 0x57adU, 0x6536U, 0x74bfU, 0x8c48U, 0x9dc1U, 
0xaf5aU, 0xbed3U, 0xca6cU, 0xdbe5U, 0xe97eU, 0xf8f7U, 0x1081U, 0x0108U, 0x3393U, 0x221aU, 
0x56a5U, 0x472cU, 0x75b7U, 0x643eU, 0x9cc9U, 0x8d40U, 0xbfdbU, 0xae52U, 0xdaedU, 0xcb64U,
0xf9ffU, 0xe876U, 0x2102U, 0x308bU, 0x0210U, 0x1399U, 0x6726U, 0x76afU, 0x4434U, 0x55bdU,
0xad4aU, 0xbcc3U, 0x8e58U, 0x9fd1U, 0xeb6eU, 0xfae7U, 0xc87cU, 0xd9f5U, 0x3183U, 0x200aU,
0x1291U, 0x0318U, 0x77a7U, 0x662eU, 0x54b5U, 0x453cU, 0xbdcbU, 0xac42U, 0x9ed9U, 0x8f50U,
0xfbefU, 0xea66U, 0xd8fdU, 0xc974U, 0x4204U, 0x538dU, 0x6116U, 0x709fU, 0x0420U, 0x15a9U,
0x2732U, 0x36bbU, 0xce4cU, 0xdfc5U, 0xed5eU, 0xfcd7U, 0x8868U, 0x99e1U, 0xab7aU, 0xbaf3U,
0x5285U, 0x430cU, 0x7197U, 0x601eU, 0x14a1U, 0x0528U, 0x37b3U, 0x263aU, 0xdecdU, 0xcf44U,
0xfddfU, 0xec56U, 0x98e9U, 0x8960U, 0xbbfbU, 0xaa72U, 0x6306U, 0x728fU, 0x4014U, 0x519dU,
0x2522U, 0x34abU, 0x0630U, 0x17b9U, 0xef4eU, 0xfec7U, 0xcc5cU, 0xddd5U, 0xa96aU, 0xb8e3U,
0x8a78U, 0x9bf1U, 0x7387U, 0x620eU, 0x5095U, 0x411cU, 0x35a3U, 0x242aU, 0x16b1U, 0x0738U,
0xffcfU, 0xee46U, 0xdcddU, 0xcd54U, 0xb9ebU, 0xa862U, 0x9af9U, 0x8b70U, 0x8408U, 0x9581U,
0xa71aU, 0xb693U, 0xc22cU, 0xd3a5U, 0xe13eU, 0xf0b7U, 0x0840U, 0x19c9U, 0x2b52U, 0x3adbU,
0x4e64U, 0x5fedU, 0x6d76U, 0x7cffU, 0x9489U, 0x8500U, 0xb79bU, 0xa612U, 0xd2adU, 0xc324U,
0xf1bfU, 0xe036U, 0x18c1U, 0x0948U, 0x3bd3U, 0x2a5aU, 0x5ee5U, 0x4f6cU, 0x7df7U, 0x6c7eU,
0xa50aU, 0xb483U, 0x8618U, 0x9791U, 0xe32eU, 0xf2a7U, 0xc03cU, 0xd1b5U, 0x2942U, 0x38cbU,
0x0a50U, 0x1bd9U, 0x6f66U, 0x7eefU, 0x4c74U, 0x5dfdU, 0xb58bU, 0xa402U, 0x9699U, 0x8710U,
0xf3afU, 0xe226U, 0xd0bdU, 0xc134U, 0x39c3U, 0x284aU, 0x1ad1U, 0x0b58U, 0x7fe7U, 0x6e6eU,
0x5cf5U, 0x4d7cU, 0xc60cU, 0xd785U, 0xe51eU, 0xf497U, 0x8028U, 0x91a1U, 0xa33aU, 0xb2b3U,
0x4a44U, 0x5bcdU, 0x6956U, 0x78dfU, 0x0c60U, 0x1de9U, 0x2f72U, 0x3efbU, 0xd68dU, 0xc704U,
0xf59fU, 0xe416U, 0x90a9U, 0x8120U, 0xb3bbU, 0xa232U, 0x5ac5U, 0x4b4cU, 0x79d7U, 0x685eU,
0x1ce1U, 0x0d68U, 0x3ff3U, 0x2e7aU, 0xe70eU, 0xf687U, 0xc41cU, 0xd595U, 0xa12aU, 0xb0a3U,
0x8238U, 0x93b1U, 0x6b46U, 0x7acfU, 0x4854U, 0x59ddU, 0x2d62U, 0x3cebU, 0x0e70U, 0x1ff9U,
0xf78fU, 0xe606U, 0xd49dU, 0xc514U, 0xb1abU, 0xa022U, 0x92b9U, 0x8330U, 0x7bc7U, 0x6a4eU,
0x58d5U, 0x495cU, 0x3de3U, 0x2c6aU, 0x1ef1U, 0x0f78U
};

// Start / Initial Coefient
const crc16t Crc16Calculator::CRC16_START_CALCULATION_VALUE  = 0xFFFFU;
const crc16t Crc16CalculatorSmlStart::CRC16_START_CALCULATION_VALUE_AFTER_SMLFILE_START = 0x91DCU;





