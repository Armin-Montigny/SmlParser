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
// The main structure is the SML File, which is a container for all other information.
// The SML File has start and end indicators which are implemented with so called 
// ESC - Sequences. An ESC - Sequence consists of 4 initial ASCII ESC characters 
// (ASCII 27 / 0x1B) followed by another 4 characters that specify the type of the ESC -
// Sequence.
// 
// In between the Start and Stop sequence the net data is transmitted. SML defines sime
// primitive data types and their representation. 
// 
// Primitive data start always with a TL byte that encodes the data type and the length of the
// net data. Then the net data follow which need to be "combined"/calculated to build the
// required information.
// 
// The Scanner in this functions analyzes the types and build a token with attributes (value).
// For special ESC-sequences an other class (EscAnalysis) is used to abtain the needed info.
// 
// The following tokens will be produced:
// 
// START_OF_SML_FILE				A start of a SML File has been deteted
// END_OF_SML_FILE					The end of a SML File was found
// END_OF_MESSAGE					End of a SML Message (0x00) 
// OPTIONAL						Optional Indicator (no data indicator)
// BOOLEAN							Data type boolean and related value
// SIGNED_INTEGER					Data type signed integer and related value
// UNSIGNED_INTEGER				Data type unsigned integer and related value
// OCTET							Data type Octet / string and related value
// LIST							Data structure List and related length
// 				
// CONDITION_NOT_YET_DETECTED		Token not yet build
// CONDITION_ERROR					Error detected
// 
// 
// There is one main function which does the job and returns the token
// 
// 	Token &scan(EhzDatabyte ehzDatabyte);
// 
// 	
// Most stuff is encapsulated.
// 
// 
// 
// A classical DFA (Deterministic finite automaton) / flat FSM (Finite State Machine) is used.
// There is no hierachy among states. The Design pattern "state" is used for implementation.
// 
// All states are derived from one abstract "State" class ("ScannerBaseState"). The abstract 
// base class declares the main worker function "scan" as pure virtual function. All derived
// "state" classes have to implement there own "scan" function.
// 
// The contect class (Scanner) and the abstract base class for states (ScannerBaseState) as well 
// as the concrete states need to work on some common data/functionality. These common data are 
// declared separately form the context class and passed as a reference to the state classes "scan"
// functions. The goal is to exchange only the relevant data. The classes shall be decoupled as much 
// as possible (This is also know as design pattern "mediator")
// 
// To avoid tons of "delete this and new" operations, the "next state" will be constructed
// using a thread safe singleton (lazy).
// 
// 
// This function is implemented as a "push" scanner: In contrast to the classical lex/flex
// approach, this function does not get the data by itself. On the contrary. The bytes to 
// analyse are pushed byte by byte into this function.
// 
// 
#include "scanner.hpp"



// ----------------------------------------------------------------------------------------------------------------------------------
// 1. Implementation of "scan" functions for concrete state
// ----------------------------------------------------------------------------------------------------------------------------------

	namespace ScannerInternal
	{


		// --------------------------------------------------------------------------------------------------------------------------
		// 1.1 Idle /  Wait for first ESC Start sequence								State No: 1
		// --------------------------------------------------------------------------------------------------------------------------

		// Initial state. We need to synchronize with the telegrams in the data stream
		// So we will wait for an ESC Start Sequence which denotes the start of an SML File
		// After that we know where we are an can analyze TL bytes and data  
		ScannerBaseState* ScannerStateIdle::scan(const EhzDatabyte , ScannerContextData &scd)
		{
			//ehzDatabyte: Data will be handled by the EscAnalysis class

			// Set next default state to this. Meaning: Default is to stay in this state
			ScannerBaseState *nextState = this;	

			// Set default analyze result to "CONDITION_NOT_YET_DETECTED". That means,
			// we could not yet find a complete token and need to analyze more data
			scd.token.setTlType(Token::CONDITION_NOT_YET_DETECTED);

			// Check if EscAnalyis could find a Start Sequence
			if (EscAnalysisResult::ESC_ANALYSIS_RESULT_START == scd.escAnalysisResultCode)
			{
				// ESC Start sequence found. Set Token type
				scd.token.setTlType(Token::START_OF_SML_FILE);
				// Goto next state
				nextState = getInstance<ScannerStateAnalyzeTl>();
			}
			return nextState;
		}


		// --------------------------------------------------------------------------------------------------------------------------
		// 1.2 Analyze the SML-TL byte													State No: 2
		// --------------------------------------------------------------------------------------------------------------------------

		// Analyze a TL-Byte. To denote a certain data type and data length a TL byte is defined
		// in the SML language description. The High Nibble of the byte is the Type. The low
		// nibble is the length of the following data. The preanalysis will be done in a dedicated
		// class "TypeLengthField". Here the length and type are already preanalyzed and set.
		
		// The typical behaviour is: First check the Type and the Length. If token can be
		// derived directly, then set it and restart the functionality. Else, goto an other state
		// and analyze more bytes to detect the value of the token

		ScannerBaseState* ScannerStateAnalyzeTl::scan(const EhzDatabyte ehzDatabyte, ScannerContextData &scd)
		{
			// Set next default state to this. Meaning: Default is to stay in this state
			ScannerBaseState *nextState = this;
			// Set default analyze result to "CONDITION_NOT_YET_DETECTED". That means,
			// we could not yet find a complete token and need to analyze more data
			scd.token.setTlType(Token::CONDITION_NOT_YET_DETECTED);
			
			// Only do something, if the ESC Analyser is ideling
			if (EscAnalysisResult::ESC_CONDITION_WAITING == scd.escAnalysisResultCode)
			{
				//lint -e{1502}		1502 defined object 'Symbol' of type Name has no non-static data members
				TlByteAnalysis tlba;  // Analyze TL Byte and goto Next State
				nextState = tlba.scan(ehzDatabyte,scd); 
			}
			else if (EscAnalysisResult::ESC_ANALYSIS_RESULT_STOP == scd.escAnalysisResultCode)
			{
				// We found an "end of SML File"
				scd.token.setTlType(Token::END_OF_SML_FILE);
				// Store associated data: Number of Fill bytes and checksum info
				scd.token.setValue(scd.escAnalysis.getLastEscFileEndData());
				nextState = getInstance<ScannerStateIdle>();  // Goto next state
			}
			else if (EscAnalysisResult::ESC_CONDITION_ANALYSING != scd.escAnalysisResultCode)
			{
				// ESC Analyser is not busy, but we could not see a valid TL byte
				scd.token.setTlType(Token::CONDITION_ERROR);  // Set error condtion
				nextState = getInstance<ScannerStateIdle>();  // Go back to idle and wait for next telegram
			} 
			else
			{
			    // else: The ESC handler is still active and is doing something
			}   
			return nextState;
		}



		// --------------------------------------------------------------------------------------------------------------------------
		// 1.3 Read all bytes for an Octet												State No: 3
		// --------------------------------------------------------------------------------------------------------------------------
		
		// The TL byte indicated an Octet. An Octet is a string. The length of the string 
		// was also given in the TL info. Note. SML Octets / Strings are not 0 terminated
		ScannerBaseState* ScannerStateReadOctet::scan(const EhzDatabyte ehzDatabyte, ScannerContextData &scd)
		{
			// Set next default state to this. Meaning: Default is to stay in this state
			ScannerBaseState *nextState = this;

			// Set default analyze result to "CONDITION_NOT_YET_DETECTED". That means,
			// we could not yet find a complete token and need to analyze more data
			scd.token.setTlType(Token::CONDITION_NOT_YET_DETECTED);

			// Do only something, if we did not see and escaped ESC. In that case we would have
			// already consumed the first 4 ESCs of an escaped ESC sequence as data bytes
			if (EscAnalysisResult::ESC_ANALYSIS_RESULT_ESCESC  != scd.escAnalysisResultCode)
			{
				// Add one byte to the resulting string
				scd.token.setValue(ehzDatabyte);

				// Check if we read all bytes as indicated by the TL field
				--scd.ehzDatabyteReadLoopCounter;
				if (0UL == scd.ehzDatabyteReadLoopCounter)
				{
					// All bytes analyzed. Set Token Type
					scd.token.setTlType(Token::OCTET);
					nextState = getInstance<ScannerStateAnalyzeTl>();  // Goto next State and read next TL
				}
			}
			return nextState;
		}


		// --------------------------------------------------------------------------------------------------------------------------
		// 1.4 Read all TL bytes for a Multi Byte Octet									State No: 4
		// --------------------------------------------------------------------------------------------------------------------------

		// Since the TL byte codes the length of the following data in the low nibble, only 4 bits
		// are available for the length information. For longer strings this might not be sufficient
		// Therefor SML defines a multi byte Octet, where several TL bytes follow each other and
		// the low nibbles are shifted and ored.
		ScannerBaseState* ScannerStateReadMultiByteOctet::scan(const EhzDatabyte ehzDatabyte, ScannerContextData &scd)
		{
			// Set next default state to this. Meaning: Default is to stay in this state
			ScannerBaseState *nextState = this;

			// Set default analyze result to "CONDITION_NOT_YET_DETECTED". That means,
			// we could not yet find a complete token and need to analyze more data
			scd.token.setTlType(Token::CONDITION_NOT_YET_DETECTED);

			// Check, if multibyte Octet is done. Then we read a normal Octet Type
			// Tl field at the end of the TL field sequence
			//lint -e921          921 Cast from Type to Type
			if (0U == (0xF0U & static_cast<uint>(ehzDatabyte)))
			{
				// TL fields are also contained in the length information.
				// Therefore we must subtract the number of read TL bytes to obtain the net data length
				scd.ehzDatabyteReadLoopCounter = scd.multiByteOctetLength - scd.multiByteOctetNumberOfTlbyteRead;
				// Set the length field in the token (without terminating 0)
				scd.token.setTlLength(scd.ehzDatabyteReadLoopCounter);
				// Goto next state
				nextState = getInstance<ScannerStateReadOctet>();  // Goto next state and read the Octet
			}
			else if (0x80U == (0x80U & static_cast<uint>(ehzDatabyte)))   //  ? 0x80U
			{
				// We found a further TL byte with type SML_DATA_TYPE_MULTIBYTE_OCTET 
				// Shift already calculated length by 4 bits to get room for another low nibble
				scd.multiByteOctetLength <<= 4;
				// Add low nibble of current TL field to get the new length
				scd.multiByteOctetLength += (static_cast<TokenLength>(ehzDatabyte) & 0x0FLU);
				// Remember the number of read Multibyte Octets (TL-Bytes)
				++scd.multiByteOctetNumberOfTlbyteRead;
			}
			else
			{
				// At the moment we do only recognize Multi Byte octets
				// No other data type with more than one TL byte shall be detected
				scd.token.setTlType(Token::CONDITION_ERROR);  // Raise error
				nextState = getInstance<ScannerStateIdle>();  // Go back to Idle state and wait for next telegram
			}
			return nextState;
		}


		// --------------------------------------------------------------------------------------------------------------------------
		// 1.5 Read a boolean value														State No: 5
		// --------------------------------------------------------------------------------------------------------------------------

		// A boolean type consits of one TL byte and one byte of net data
		//lint -e{1961}   1961 virtual member function 'Symbol' could be made const
		ScannerBaseState* ScannerStateReadBoolean::scan(const EhzDatabyte ehzDatabyte, ScannerContextData &scd)
		{
			scd.token.setValue((0U != static_cast<uint>(ehzDatabyte)));  // Set the token value
			scd.token.setTlType(Token::BOOLEAN);  // Set the token type
			return getInstance<ScannerStateAnalyzeTl>();  // Goto next state. Read next TL Byte
		}


		// --------------------------------------------------------------------------------------------------------------------------
		// 1.6 Read a signed integer													State No: 6
		// --------------------------------------------------------------------------------------------------------------------------

		// The TL byte has shown a Signed Integer Type with a given length
		// In this state we will read the corresponding databyte and build up the resulting value
		ScannerBaseState* ScannerStateReadSignedInteger::scan(const EhzDatabyte ehzDatabyte, ScannerContextData &scd)
		{
			// Set next default state to this. Meaning: Default is to stay in this state
			ScannerBaseState *nextState = this;

			// Set default analyze result to "CONDITION_NOT_YET_DETECTED". That means,
			// we could not yet find a complete token and need to analyze more data
			scd.token.setTlType(Token::CONDITION_NOT_YET_DETECTED);

			// Do only something, if we did not see and escaped ESC. In that case we would have
			// already consumed the first 4 ESCs of an escaped ESC sequence as data bytes
			if (EscAnalysisResult::ESC_ANALYSIS_RESULT_ESCESC  != scd.escAnalysisResultCode)
			{
				// We have to deal with Big Endian signed data
				// We receive the data byte by byte and calculate the longest possible integer
				// The high byte comes first. We will use sign bit propagation
				if (scd.isFirstSignedIntegerByte)
				{
					// First byte. If the byte is negative the whole s64 will be negative 
					scd.s64TempSignedInteger = s64(ehzDatabyte);
				}
				else
				{
					// Further bytes. Shift the old value and or in the new byte
					scd.s64TempSignedInteger *= 256LL;
					scd.s64TempSignedInteger += (static_cast<s64>(ehzDatabyte) % 256LL);
				}

				// Check if we consumed all net data
				--scd.ehzDatabyteReadLoopCounter;
				if (null<TokenLength>() == scd.ehzDatabyteReadLoopCounter)
				{
					// Done. Store resulting value in token
					scd.token.setValue(scd.s64TempSignedInteger);
					scd.token.setTlType(Token::SIGNED_INTEGER); // Set token type
					nextState = getInstance<ScannerStateAnalyzeTl>();  // Goto next state. Read next TL Byte
				}
			}
			return nextState;
		}



		// --------------------------------------------------------------------------------------------------------------------------
		// 1.6 Read an unsigned integer													State No: 7
		// --------------------------------------------------------------------------------------------------------------------------

		// The TL byte has shown an Unsigned Integer Type with a given length
		// In this state we will read the corresponding databyte and build up the resulting value
		ScannerBaseState* ScannerStateReadUnsignedInteger::scan(const EhzDatabyte ehzDatabyte, ScannerContextData &scd)
		{
			// Set next default state to this. Meaning: Default is to stay in this state
			ScannerBaseState *nextState = this;

			// Set default analyze result to "CONDITION_NOT_YET_DETECTED". That means,
			// we could not yet find a complete token and need to analyze more data
			scd.token.setTlType(Token::CONDITION_NOT_YET_DETECTED);

			// Do only something, if we did not see and escaped ESC. In that case we would have
			// already consumed the first 4 ESCs of an escaped ESC sequence as data bytes
			if (EscAnalysisResult::ESC_ANALYSIS_RESULT_ESCESC  != scd.escAnalysisResultCode)
			{
				// Further bytes. Shift the old value and or in the new byte
				scd.u64TempUnsignedInteger <<= 8;
				scd.u64TempUnsignedInteger |= (static_cast<u64>(ehzDatabyte) & 0xFFLLU);

				// Check if we consumed all net data
				--scd.ehzDatabyteReadLoopCounter;
				if (null<TokenLength>() == scd.ehzDatabyteReadLoopCounter)
				{
					// Done. Store resulting value in token
					scd.token.setValue(scd.u64TempUnsignedInteger);
					scd.token.setTlType(Token::UNSIGNED_INTEGER);  // Set token type
					nextState = getInstance<ScannerStateAnalyzeTl>();  // Goto next state. Read next TL Byte
				}
			}
			return nextState;
		}

		

	
		
		
		
		
		
	} // end of namespace ScannerInternal




