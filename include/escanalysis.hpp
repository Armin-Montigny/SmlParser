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


#ifndef ESCANALYZE_HPP
#define ESCANALYZE_HPP

#include "crc16.hpp"
#include "singleton.hpp"

 


// ----------------------------------------------------------------------------------------------------------------------------------
// 1. Declaration of data needed to interface and use the functionality of this module
// ----------------------------------------------------------------------------------------------------------------------------------

// Class EscAnalysis may be instantiated by an other object and can then do
// the ESC analysis as described in the Header



	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.1 Declaration of helper values to be used by invoking functions/objects
    // ------------------------------------------------------------------------------------------------------------------------------

	// Invoking functions may only be interested in the result of the EscAnalyis
	// However. The ESC-Stop sequence contains additional information that might
	// be interesting. This data can be optained by EscAnalysis using a special
	// function. The corresponding data structure is defined here
	struct EscSmlFileEndData
	{
	    EscSmlFileEndData(void) : crc16FromEscStop(0U) ,crc16Calculated(0U),numberOfFillBytes(null<u8>()) {}
		crc16t crc16FromEscStop;	// The crc16 bytes contained in ESC Stop
		crc16t crc16Calculated;	// The calculated crc16 checksum from the SML file
		u8 numberOfFillBytes;	// The number of fill bytes in the SML File
	};


	// These are the function return codes for the main analysis function
	// With that it is possible to get the current condition of the analysis
	// function or, if available, the detected ESC-Sequence
	// If there is malformed/unexpected data, including a wrong checksum,
	//an error value will be returned.
	struct EscAnalysisResult
	{
		enum Code
		{
			ESC_CONDITION_WAITING,		// Waiting for valid next ESC-Start
			ESC_CONDITION_ANALYSING,	// Found possible ESC-Sequence. Need further analysis
			ESC_ANALYSIS_RESULT_START,	// ESC-Start found
			ESC_ANALYSIS_RESULT_STOP,	// ESC-Stop sound
			ESC_ANALYSIS_RESULT_ESCESC, // Escaped ESC found --> Databyte should be ignored
			ESC_ANALYSIS_RESULT_ERROR	// Error condition (malformed/unexpected/wrong checksum)
		};
	};
	


	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.2 Declaration of common data for context and states
    // ------------------------------------------------------------------------------------------------------------------------------
    
    // Design pattern state is used
    // The contect class (EscAnalysis) and the abstract base class for states
    // (EscAnalysisBaseState) as well as the concrete states need to work on
    // some common data. These common data are declared separately form the 
    // context class and passed as a reference to the state classes "analyse"
    // functions. The goal is to exchange only the relevant data. The classes
    // shall be decoupled as much as possible
	//
	// This is also a form of design pattern "mediator" 

	namespace EscAnalyisInternal
	{
		struct EscAnalysisContextData
		{
		    EscAnalysisContextData(void) : resultCode(EscAnalysisResult::ESC_ANALYSIS_RESULT_ERROR),
											escSmlFileEndData(),
											smlFileCrc16Calculator()
            {}
			// Function return code for all analyse functions
			EscAnalysisResult::Code resultCode;
			// SML File ESC-Stop sequence related data
			EscSmlFileEndData escSmlFileEndData;
			// Helper Class for calculating the CRC16 for an SML File
			Crc16CalculatorSmlStart smlFileCrc16Calculator;
		};

	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.3 Declaration of main worker class
    // ------------------------------------------------------------------------------------------------------------------------------

	// This is the only class that shoud be instantiated in the outer world
	
	// Databytes will be passed to the main interface function "analyse"
	// This function will return one of the upper described retrun codes
	// In case of an ESC-Stop, additional information may be obtained by
	// calling "getLastEscFileEndData"
	
	// The functionality of matching desired sequences is realized via
	// a state machine (DFA). The state machine is implemented using the design
	// pattern state. Regarding that patter, this class is the context
	

		class EscAnalysisBaseState; // Forward declaration for abstract base state class
	} // End of	namespace EscAnalyisInternal
	


	
	class EscAnalysis
	{
		public:
			EscAnalysis(void);
			void reset(void);
			
			
			// Main working function for the outer world
			EscAnalysisResult::Code analyse(const EhzDatabyte databyte);
			
			// In case that the outer world is interested in checksum and fill byte information
			const EscSmlFileEndData &getLastEscFileEndData(void) const; 
			
		protected:
		
			// The current state of the state machine
			EscAnalyisInternal::EscAnalysisBaseState *currentState;

			// Common info that will be also passed to the State Classes
			// so that they can access common context data
			EscAnalyisInternal::EscAnalysisContextData eacd;
	};




// ----------------------------------------------------------------------------------------------------------------------------------
// 2. Abstract Base Class for states
// ----------------------------------------------------------------------------------------------------------------------------------

// The design pattern "state" uses a context, an "abstract base class" for states 
// and "concrete states". Here the abstract base is declared. It has one pure
// virtual function "analyse" that must be overwritten by all "concrete" states.

// Additionally some common definitions for the bytes to compare to are defined
// for underlying classes.

// Note: All state classes contain no data. Just member functions.

// There will be no static instances of any of those classes. The classes will be
// constructed when necessary. In order to avoid millions of "delete this / new"
// operations, a singleton will be used to provide exactly one repsresentation of
// each concrete class. The Singleton is defined as a template in the base class
// for easy handling and switching of states. Also here we will not find any
// global static data outside of the class declaration

namespace EscAnalyisInternal {

//lint -e921	
const EhzDatabyte DATABYTE_ESC = static_cast<EhzDatabyte>(0x1BU);
const EhzDatabyte DATABYTE_START = static_cast<EhzDatabyte>(0x01U);
const EhzDatabyte DATABYTE_STOP = static_cast<EhzDatabyte>(0x1AU);
//lint +e921	


class EscAnalysisBaseState
{
	public: 
		// This function analyses given inputs and evaluates next states
		// All derived states need to overwrite and implement this function
		virtual EscAnalysisBaseState *analyse(const EhzDatabyte databyte, EscAnalysisContextData &eacd)  = 0;
		
		// Analyze function matches databytes to these special indicators
		// Get the instance of the next state class
		//lint -e{1960} -e{9026} -e{9022}
		DEFINE_STATIC_SINGLETON_FUNCTION(getInstance)
};


// ----------------------------------------------------------------------------------------------------------------------------------
// 3. Definition of concrete states
// ----------------------------------------------------------------------------------------------------------------------------------

// This module uses the design pattern state, which requires a context, an
// abstract base class for states and the concrete states. Those will be
// defined here.

// The states are independent from the context and implement "state"-specific
// functionality.

// All states have to implement a "analyse" functions, which will check a
// given databyte from a SML File and try to match it to a given part of a
// sequence



	// ------------------------------------------------------------------------------------------------------------------------------
	// 3.1 Macros and templates
    // ------------------------------------------------------------------------------------------------------------------------------

	// The definition of the state classes witht their "analyse" function are
	// all very similar. Therefore macros and templates are used to avoid
	// unnecessary and error prone writing of redundant text
	
	
	// Standard declaration of a state class, derived from the abstract base
	// state class and declaring the concrete "analyse" function
	//lint -e1960 -e9026 -e9022
	#define DEFINE_ESC_STANDARD_STATE(stateName) \
	class stateName : public EscAnalysisBaseState \
	{ \
		public: \
			virtual EscAnalysisBaseState *analyse(const EhzDatabyte databyte, EscAnalysisContextData &eacd); \
	}   
	//lint +e1960 +e9026 +e9022


	// Template for very simple states that just compare the databyte with a 
	// given value (template argument) and then set the next state next state
	// (template argument) and appropriate return values(template argument). 
	// This is just a template and no specific state. 
	template <class NextState, 
			  const EscAnalysisResult::Code matchResult,
			  const EhzDatabyte compareValue = DATABYTE_ESC,
			  const EscAnalysisResult::Code noMatchResult = EscAnalysisResult::ESC_ANALYSIS_RESULT_ERROR>
	class EscAnalysisSimpleState : public EscAnalysisBaseState
	{
		public: 
			virtual EscAnalysisBaseState *analyse(const EhzDatabyte databyte, EscAnalysisContextData &eacd)  ;
	};



	// ------------------------------------------------------------------------------------------------------------------------------
	// 3.2 Concrete states
    // ------------------------------------------------------------------------------------------------------------------------------
    
    // Sequence of state declaration has nothing to do with real functionality.
    // It is necessary to avoid forward declarations of incomplete types


		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.1 Idle /  Wait for first ESC									State No: 1
		// --------------------------------------------------------------------------------------------------------------------------

		DEFINE_ESC_STANDARD_STATE(EscStateIdle);


		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.2 Read 2nd crc16 byte of ESC Stop sequence					State No: 2
		// --------------------------------------------------------------------------------------------------------------------------

		DEFINE_ESC_STANDARD_STATE(StateWaitForCrc16Byte2);


		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.3 Read 1st crc16 byte of ESC Stop sequence					State No: 3
		// --------------------------------------------------------------------------------------------------------------------------

		DEFINE_ESC_STANDARD_STATE(StateWaitForCrc16Byte1);


		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.4 Read fill byte out of ESC Stop sequence					State No: 4
		// --------------------------------------------------------------------------------------------------------------------------

		DEFINE_ESC_STANDARD_STATE(StateWaitForFillByte);

		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.5 Match the 4th ESC of an ESC ESC sequence 					State No: 5
		// --------------------------------------------------------------------------------------------------------------------------

		typedef EscAnalysisSimpleState
		<
			EscStateIdle,
			EscAnalysisResult::ESC_ANALYSIS_RESULT_ESCESC
		> EscStateWaitFor4thESCESC;
		
		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.6 Match the 3rd ESC of an ESC ESC sequence 					State No: 6
		// --------------------------------------------------------------------------------------------------------------------------

		typedef EscAnalysisSimpleState
		<
			EscStateWaitFor4thESCESC, 
			EscAnalysisResult::ESC_ANALYSIS_RESULT_ESCESC
		> EscStateWaitFor3rdESCESC;
		
		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.7 Match the 2nd ESC of an ESC ESC sequence 					State No: 7
		// --------------------------------------------------------------------------------------------------------------------------

		typedef EscAnalysisSimpleState
		<
			EscStateWaitFor3rdESCESC, 
			 EscAnalysisResult::ESC_ANALYSIS_RESULT_ESCESC
		> EscStateWaitFor2ndESCESC;

		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.8 Match the 4th Start (0x01) of an ESC Start sequence		State No: 8 
		// --------------------------------------------------------------------------------------------------------------------------
		
		DEFINE_ESC_STANDARD_STATE(EscStateWaitFor4thStart);


		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.9 Match the 3rd Start (0x01) of an ESC Start sequence		State No: 9
		// --------------------------------------------------------------------------------------------------------------------------

		typedef EscAnalysisSimpleState
		<
			EscStateWaitFor4thStart, 
			EscAnalysisResult::ESC_CONDITION_ANALYSING,
			DATABYTE_START
		> EscStateWaitFor3rdStart;
		
		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.10 Match the 2nd Start (0x01) of an ESC Start sequence		State No: 10 
		// --------------------------------------------------------------------------------------------------------------------------

		typedef EscAnalysisSimpleState
		<
			EscStateWaitFor3rdStart, 
			EscAnalysisResult::ESC_CONDITION_ANALYSING,
			DATABYTE_START
		> EscStateWaitFor2ndStart;
		
		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.11 Initial ESC sequence (4 ESC's) read						State No: 11
		// --------------------------------------------------------------------------------------------------------------------------

		DEFINE_ESC_STANDARD_STATE(EscState4InitialEscRead);

		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.12 Wait for 4th ESC of initial ESC-sequence					State No: 12
		// --------------------------------------------------------------------------------------------------------------------------

		typedef EscAnalysisSimpleState
		<
			EscState4InitialEscRead, 
			EscAnalysisResult::ESC_CONDITION_ANALYSING,
			DATABYTE_ESC, 
			EscAnalysisResult::ESC_CONDITION_WAITING
		> EscStateWaitFor4thEsc;
		
		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.13 Wait for 3rd ESC of initial ESC-sequence					State No: 13
		// --------------------------------------------------------------------------------------------------------------------------

		typedef EscAnalysisSimpleState
		<
			EscStateWaitFor4thEsc, 
			EscAnalysisResult::ESC_CONDITION_ANALYSING,
			DATABYTE_ESC, 
			EscAnalysisResult::ESC_CONDITION_WAITING
		> EscStateWaitFor3rdEsc;

		// --------------------------------------------------------------------------------------------------------------------------
		// 3.2.14 Wait for 2nd ESC of initial ESC-sequence					State No: 14
		// --------------------------------------------------------------------------------------------------------------------------

		typedef EscAnalysisSimpleState
		<
			EscStateWaitFor3rdEsc, 
			EscAnalysisResult::ESC_CONDITION_ANALYSING,
			DATABYTE_ESC, 
			EscAnalysisResult::ESC_CONDITION_WAITING
		> EscStateWaitFor2ndEsc;


// ----------------------------------------------------------------------------------------------------------------------------------
// 4. Definition of member functions
// ----------------------------------------------------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------------------------------------------------
	// 4.1 Member functions for class EscAnalysisSimpleState
    // ------------------------------------------------------------------------------------------------------------------------------

	// Member function "analyse" for Simple State class.
	// Just matches the given databyte to a template parameter argument
	
	//lint -e{1961}
	template <class NextState, 
			  const EscAnalysisResult::Code matchResult,
			  const EhzDatabyte compareValue, 
			  const EscAnalysisResult::Code noMatchResult>
	inline EscAnalysisBaseState* EscAnalysisSimpleState<NextState, matchResult, compareValue, noMatchResult>
		::analyse(const EhzDatabyte databyte, EscAnalysisContextData &eacd ) 
	{
		EscAnalysisBaseState* nextState = getInstance<EscStateIdle>();
		// Do we see the desired value
		//lint -e{911}	// Note 911: Implicit expression promotion from unsigned char to int
		if (compareValue == databyte)
		{
			// Yep, return positive Result
			eacd.resultCode = matchResult;
			// Goto next state
			nextState = getInstance<NextState>();
		}
		else
		{
			// A no match. Go back to idle state
			eacd.resultCode = noMatchResult;
		}
		return nextState;
	}



} // End of	namespace EscAnalyisInternal

	

	// ------------------------------------------------------------------------------------------------------------------------------
	// 4.2 Inline member functions for main class EscAnalysis
    // ------------------------------------------------------------------------------------------------------------------------------

	inline void EscAnalysis::reset(void)
	{
		// Set start state
		currentState = EscAnalyisInternal::EscAnalysisBaseState::getInstance<EscAnalyisInternal::EscStateIdle>();
	}
	
	inline EscAnalysis::EscAnalysis(void) : currentState(EscAnalyisInternal::EscAnalysisBaseState::getInstance<EscAnalyisInternal::EscStateIdle>()), eacd()
	{}

	// Implementation of main "analyse" function 
	inline EscAnalysisResult::Code EscAnalysis::analyse(const EhzDatabyte databyte) 
	{
		// Continuosly Run the CRC16 calculation for a complete SmlFile
		// Start and Stop of the calculation is set in the corresponding states
		eacd.smlFileCrc16Calculator.update(databyte);
		
		// This is the main state machine
		// Analyse data stream and produce result for scanner
		currentState = currentState->analyse(databyte,eacd);

		return eacd.resultCode;
	}


	// Get ESC-Sequence Stop associated  SML-File end data
	inline const EscSmlFileEndData &EscAnalysis::getLastEscFileEndData(void) const
	{
		return eacd.escSmlFileEndData;
	}


 

#endif
