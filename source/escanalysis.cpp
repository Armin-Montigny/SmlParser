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
//
// ESC Sequence Analysis of SML File Data
// 
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
// In case that the net data stream contains also 4 ESC's in a row, those have to be escaped
// as well with additional 4 ESC's. So 8 ESC's in a row are considered as 4 ESC net data.
// 
// If a scanner has to analyse tokens, it must recognise all ESC - Sequences.
// 
// In the special case of EDL21 compliant meters only ESC-Start, ESC-Stop and escaped ESC's
// need to be recognized. Every other ESC Sequence will be considered as an Error.
// 
// Definition of ESC - Sequences:
// 
// Escaped ESC:       0x1B 0x1B 0x1B 0x1B 0x1B 0x1B 0x1B 0x1B  
// ESC Start:         0x1B 0x1B 0x1B 0x1B 0x01 0x01 0x01 0x01
// ESC-Stop:          0x1B 0x1B 0x1B 0x1B 0x1A   fl   ch   cl
// 
// fl: 
// Number of fill bytes. All SML Files have a modulo 4 based length. To achieve that empty
// messages (0x00) will be inserted before the stop sequence (if necessary). In the ESC-Stop
// sequence the number of those fill bytes will be mentioned so that a cross check is possible.
// 
// cl / ch
// Low byte and high byte of a crc16 checksum. SML files are secured by a CRC16 checksum.
// Calculation starts with the first ESC of the ESC-Start sequence and ends with the fill byte.
// in ch and cl the crc16 is encoded. The evaluation program has to calculate the checksum
// and do need to compare the result with the given values in ch/cl.
// 
// 
// 
// The following module will check for the 3 mentioned ESC Sequences and take care of the
// CRC16 calculation and check. Additionally it will inform on the condition of the evaluation
// process. This maybe:
// 
// Condition Waiting: 			Nothing meaningful received. Waiting for begin of SML File
// Condition Analysing:        Evaluation in progress. Result not yet available
// Result Start:               ESC-Start received
// Result Stop:				ESC-Stop received
// Result ESC:					Escaped ESC received
// Result Error:				Unexpected or invalid data received
// 
// 
// Functionality is implemented with the class EscAnalayis with the main working function
// analyse. The Crc16 calculater is implemented as a separate class in an other module.
// 
// 
// A classical DFA (Deterministic finite automaton) / flat FSM (Finite State Machine) is used.
// There is no hierachy among states. The Design pattern "state" is used for implementation.
// 
// All states are derived from one abstract "State" class ("EscAnalysisBaseState"). The abstract 
// base class declares the main worker function "analyse" as pure virtual function. All derived
// "state" classes have to implement there own "analyse" function.
// 
// The "context" class just uses 1 pointer to the abstract state. For information exchange with 
// the "state" classes the struct "EscAnalysisContextData" has been defined. So both "state"
// and "context" class use this external data structure for the exchange of information.
// 
// For an easier usage templates and macros are provided
// 
// State transition is, according to the design pattern state, done by dynamically assigning
// a pointer to the next class to the "currentState" of the context. Each "state" classes'
// "analyze"-function will return a pointer to the next "state" class. This is done
// dynamically. There is no static instance of any state class. All state classes contain
// no data at all. 
// 
// To avoid tons of "delete this and new" operations, the "next state" will be constructed
// using a thread safe singleton (lazy).
// 
// 
// This function is implemented as a "push" analyzer: In contrast to the classical lex/flex
// approach, this function does not get the data by itself. On the contrary. The bytes to 
// analyse are pushed byte by byte into this function.
// 
// 
//


#include "escanalysis.hpp"		


// ----------------------------------------------------------------------------------------------------------------------------------
// 1. Implementation of "analyse" functions for concrete state
// ----------------------------------------------------------------------------------------------------------------------------------
namespace EscAnalyisInternal
{

	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.1 EscStateIdle														State No: 1
    // ------------------------------------------------------------------------------------------------------------------------------

	// Idle-State. Wait for any ESC sequence to Start
	EscAnalysisBaseState *EscStateIdle::analyse(const EhzDatabyte databyte, EscAnalysisContextData &eacd) 
	{
		// Default is to stay in this state;
		EscAnalysisBaseState *nextState = this;

		// Wait for the 1st ESC of an ESC sequence
		//lint -e{911}	// Note 911: Implicit expression promotion from unsigned char to int     What a nonesense
		if (DATABYTE_ESC == databyte) 
		{
			// Inform Scanner that we found something and are evaluating further
			eacd.resultCode = EscAnalysisResult::ESC_CONDITION_ANALYSING;
			// Goto next state
			nextState = getInstance<EscStateWaitFor2ndEsc>();
		}
		else
		{
			// Tell Scanner that nothing is going on
			eacd.resultCode = EscAnalysisResult::ESC_CONDITION_WAITING;
		}
		return nextState;
	}


	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.2 StateWaitForCrc16Byte2											State No: 2
    // ------------------------------------------------------------------------------------------------------------------------------

	// Last activity for a ESC Stop sequence. Check on CRC16 codes
	//lint -e{1961}
	EscAnalysisBaseState *StateWaitForCrc16Byte2::analyse(const EhzDatabyte databyte, EscAnalysisContextData &eacd)
	{
		// Store the 16 bit CRC 16 checksum
		//lint -e{921}
		eacd.escSmlFileEndData.crc16FromEscStop = 
			((eacd.escSmlFileEndData.crc16FromEscStop << 8U) & 0xff00U) | static_cast<crc16t>(databyte);
			
		// Get the result from the running calculation of the checksum	
		eacd.escSmlFileEndData.crc16Calculated = eacd.smlFileCrc16Calculator.getResult();

		// Compare the calculated result with the checksum transmitted in the data stream
		if ((eacd.escSmlFileEndData.crc16FromEscStop == eacd.escSmlFileEndData.crc16Calculated))
		{
			// Everthing OK. Inform scanner that we found a complete ESC Stop sequence
			eacd.resultCode = EscAnalysisResult::ESC_ANALYSIS_RESULT_STOP;
		}
		else
		{
			// Checksum mismatch --> error
			eacd.resultCode = EscAnalysisResult::ESC_ANALYSIS_RESULT_ERROR;
		}
		// In any case go back to Idle state
		return getInstance<EscStateIdle>();
	}


	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.3 StateWaitForCrc16Byte1											State No: 3
    // ------------------------------------------------------------------------------------------------------------------------------

	// Read the first byte of the crc16 checksum
	//lint -e{1961}
	EscAnalysisBaseState *StateWaitForCrc16Byte1::analyse(const EhzDatabyte databyte, EscAnalysisContextData &eacd) 
	{
		// Store first part (low byte) of the crc16 checksum
		//lint -e{921}
		eacd.escSmlFileEndData.crc16FromEscStop = static_cast<crc16t>(databyte);

		// Still doing analysis. Need to get the 2nd crc16 byte
		eacd.resultCode = EscAnalysisResult::ESC_CONDITION_ANALYSING;
		// Goto next state
		return getInstance<StateWaitForCrc16Byte2>();
	}


	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.4 StateWaitForFillByte												State No: 4
    // ------------------------------------------------------------------------------------------------------------------------------

	// Part of ESC Stop sequence analysis. Get the number of fill bytes
	//lint -e{1961}
	EscAnalysisBaseState *StateWaitForFillByte::analyse(const EhzDatabyte databyte, EscAnalysisContextData &eacd) 
	{
		// Save the number of illbytes in the context object
		eacd.escSmlFileEndData.numberOfFillBytes = databyte;
		// All relevant part of SML file has been read. Next will be the crc16 bytes.
		// Therefore we stop the crc16 calculation now
		eacd.smlFileCrc16Calculator.stop();
		// We need to do further analysis
		eacd.resultCode = EscAnalysisResult::ESC_CONDITION_ANALYSING;
		// return new StateWaitForCrc16Byte1;
		return getInstance<StateWaitForCrc16Byte1>();

	}


	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.5 EscStateWaitFor4thStart											State No: 8
    // ------------------------------------------------------------------------------------------------------------------------------

	// Final Match of "ESC-Start" sequence. Already analzed 1B 1B 1B 1B 01 01 01
	//lint -e{1961}
	EscAnalysisBaseState *EscStateWaitFor4thStart::analyse(const EhzDatabyte databyte, EscAnalysisContextData &eacd) 
	{
		// Check for final "Start Indicator" (0x01) for a ESC Start sequence
		//lint -e{911}	// Note 911: Implicit expression promotion from unsigned char to int
		if (DATABYTE_START == databyte)
		{
			// Complete ESC Start sequence read. Inform Scanner
			eacd.resultCode = EscAnalysisResult::ESC_ANALYSIS_RESULT_START;
			// Start crc16 calculation for a complete SML File
			eacd.smlFileCrc16Calculator.start();
		}
		else
		{
			// Although we've read 1B 1B 1B 1B 01 01 01, we did not see the last 01 --> Error
			eacd.resultCode = EscAnalysisResult::ESC_ANALYSIS_RESULT_ERROR;
		}
		// Under any condition. Go back to state "Idle"
		return getInstance<EscStateIdle>();
	}


	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.6 EscState4InitialEscRead											State No: 11
    // ------------------------------------------------------------------------------------------------------------------------------

	// For initial ESCs of an ESC sequence have been read. Next byte will decide on what 
	// kind of ESC sequence it will be
	//lint -e{1961}
	EscAnalysisBaseState *EscState4InitialEscRead::analyse(const EhzDatabyte databyte, EscAnalysisContextData &eacd) 
	{
		// Default is to go back to Idle state
		EscAnalysisBaseState *nextState = getInstance<EscStateIdle>();

		// We will evaluate Start (0x01), Stop (0x1A) and ESC (0x1B)
		// Everything else is dismissed
		//lint -e{921}		// cast from unsigned char to int
		switch(static_cast<sint>(databyte))
		{
			case static_cast<sint>(DATABYTE_START):
				// Check for a complete Start sequence. Further evaluation is necessary
				eacd.resultCode = EscAnalysisResult::ESC_CONDITION_ANALYSING;
				// Goto next state
				nextState = getInstance<EscStateWaitFor2ndStart>();
				break;	
				
			case static_cast<sint>(DATABYTE_STOP):
				// Check for a Stop sequence. Further evaluation is necessary
				eacd.resultCode = EscAnalysisResult::ESC_CONDITION_ANALYSING;
				// Goto next state
				nextState = getInstance<StateWaitForFillByte>();
				break;
				
			case static_cast<sint>(DATABYTE_ESC):
				// Escaped ESC. Wait for additional ESCs. Inform Scanner
				eacd.resultCode = EscAnalysisResult::ESC_ANALYSIS_RESULT_ESCESC;
				// Goto next state
				nextState = getInstance<EscStateWaitFor2ndESCESC>();
				break;
				
			default:
				// We found something unexpected. This is an error
				eacd.resultCode = EscAnalysisResult::ESC_ANALYSIS_RESULT_ERROR;
				break;
		}
		return nextState;		
	}

} // End of	namespace EscAnalyisInternal





