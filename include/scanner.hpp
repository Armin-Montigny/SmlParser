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
// net data. Then the net data follow which need to be "combined"/claculated to build the
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

#ifndef SCANNER_HPP
#define SCANNER_HPP

#include "escanalysis.hpp"
#include "token.hpp"

 
 



// ----------------------------------------------------------------------------------------------------------------------------------
// 1. Declaration of data needed to interface and use the functionality of this module
// ----------------------------------------------------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.1 Declaration of common data for context and states
    // ------------------------------------------------------------------------------------------------------------------------------
    
	// This data is only for Scanner internal usage

	namespace ScannerInternal
	{

		// Design pattern state is used
		//
		// The contect class (Scanner) and the abstract base class for states
		// (ScannerBaseState) as well as the concrete states need to work on
		// some common data/functionality. These common data are declared separately 
		// form the context class and passed as a reference to the state classes "scan"
		// functions. The goal is to exchange only the relevant data. The classes
		// shall be decoupled as much as possible
		//
		// This is also a form of design pattern "mediator"

		struct ScannerContextData
		{	
		    ScannerContextData(void);
			virtual ~ScannerContextData(void) {}
		    
			EscAnalysisResult::Code escAnalysisResultCode;	// Result of EsCAnalysis
			EscAnalysis escAnalysis;						// The ESC sequence analyzer

			TokenLength ehzDatabyteReadLoopCounter;			// Number of bytes to read for a type

			TokenLength multiByteOctetLength;				// Length of Multi Byte Octet Data
			TokenLength multiByteOctetNumberOfTlbyteRead;	// Number of TL bytes of a Multi byte Octet

			s64 s64TempSignedInteger;						// Temp for building up s64 from bytes
			u64 u64TempUnsignedInteger;						// Temp for building up u64 from bytes
			boolean isFirstSignedIntegerByte;					// Needed for correct handlign of sign bit

			Token token;									// Token that is produced by scanner
		};
		
		// Initialize Context to default values
        inline ScannerContextData::ScannerContextData(void)
            : escAnalysisResultCode(EscAnalysisResult::ESC_ANALYSIS_RESULT_ERROR),
              escAnalysis(),
              ehzDatabyteReadLoopCounter(0UL),
              multiByteOctetLength(0UL),
              multiByteOctetNumberOfTlbyteRead(0UL),
              s64TempSignedInteger(0LL),
              u64TempUnsignedInteger(0ULL),
              isFirstSignedIntegerByte(true),
              token()
		{}
              


	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.3 Declaration of main worker class
    // ------------------------------------------------------------------------------------------------------------------------------

	// This is the only class that should be instantiated in the outer world

	// Scanner

	// Main "worker" class. This class is used to scan the input datastream send by an 
	// electronic meter in SML format. It does a lexical analysis and produces tokens as
	// an output. It has only one interface function:
	//
	// --> Token &scan(EhzDatabyte ehzDatabyte);
	//
	// The functionality of matching desired tokens is realized via
	// a state machine (DFA). The state machine is implemented using the design
	// pattern state. Regarding that patter, this class is the context


		// Forward declaration of the abstract base class for scanner states
		class ScannerBaseState;

	} // end of namespace ScannerInternal






	class Scanner
	{
		//lint --e{1540}  // 1540 pointer member 'Symbol' (Location) neither freed nor zero'ed by destructor --
		public:
			Scanner(void);
			virtual ~Scanner(void) {}

			// Main "working" function for the outer world
			const Token &scan(const EhzDatabyte ehzDatabyte);
			
			void reset(void); 
		protected:

			// Current state of the state machine
			ScannerInternal::ScannerBaseState *currentState;

			// Common context data for "communication" with the states
			ScannerInternal::ScannerContextData scd;
	};




// ----------------------------------------------------------------------------------------------------------------------------------
// 2. Abstract Base Class for states
// ----------------------------------------------------------------------------------------------------------------------------------

	// This data is only for Scanner internal usage
	namespace ScannerInternal
	{


		// Definition of abstract base class for all states.
		// The design pattern states requires one abstract base class for all implemented states.
		// It has on pure virtual function 
		//
		// virtual ScannerBaseState* scan(EhzDatabyte ehzDatabyte, ScannerContextData &scd) = 0;
		//
		// This function will scann the data stream. It will return a pointer to the next state
		// The next state is a concrete state, but will be returned as a pointer to the base state
		// All derived "states" have to overwrite this function
		// 
		// A singleton function is provided to create "states".
		//

		class ScannerBaseState
		{
			public:
				virtual ScannerBaseState* scan(const EhzDatabyte ehzDatabyte, ScannerContextData &scd) = 0;

				// Get the instance of the next state class (Singleton)
				DEFINE_STATIC_SINGLETON_FUNCTION(getInstance)
		};


	


// ----------------------------------------------------------------------------------------------------------------------------------
// 3. Definition of concrete states
// ----------------------------------------------------------------------------------------------------------------------------------

		// Declaration of concrete states

		// According to the design pattern state, a "conrete state" is declared for all needed states
		// of the state machine. The concrete state is derived from the common "abstract base state"
		// and implements the states specific functionality.



		// ------------------------------------------------------------------------------------------------------------------------------
		// 3.1 Macros 		
		// ------------------------------------------------------------------------------------------------------------------------------

		// Macro for declaring standard states that all look the same
		// So we can save error prone typing or copy and past work 

		//lint -save
		//9022   unparenthesized macro parameter in definition of macro 'String'
		//9026   Function-like macro, 'String', defined 
		//lint -e1960 -e9022 -e9026
		#define DEFINE_SCANNER_STANDARD_STATE(stateName)  \
		class stateName : public ScannerBaseState  \
		{  \
			public:  \
				virtual ScannerBaseState* scan(const EhzDatabyte ehzDatabyte, ScannerContextData &scd);  \
		}
		//lint -restore
		
		// TL Byte Analysis using a jump table.
		// All possible TL-Bytes (Max 256 different values) will be handled with a tabular approach
		class TlByteAnalysis : public ScannerBaseState
		{  
			public:  
				virtual ScannerBaseState* scan(const EhzDatabyte ehzDatabyte, ScannerContextData &scd); 
			protected:
				// The Table declaration
				struct TLBT
				{
					Token::TokenType tokenType;	// Token type (depending on TL byte)
					TokenLength tokenLength;	// Token length (depending on TL byte)
					ScannerBaseState* (*handleTlByte)(ScannerContextData &scd);
				};
				// Table definition
				static const TLBT tlbt[256];
				
				// Handling functions used in the table
				static ScannerBaseState* handleTlByteBasic(ScannerContextData &scd);
				static ScannerBaseState* handleTlByteOptional(ScannerContextData &scd);
				static ScannerBaseState* handleTlByteBasicReset(ScannerContextData &scd);
				static ScannerBaseState* handleTlByteOctet(ScannerContextData &scd);
				static ScannerBaseState* handleTlByteBoolean(ScannerContextData &scd);
				static ScannerBaseState* handleTlByteSignedInteger(ScannerContextData &scd);
				static ScannerBaseState* handleTlByteUnsignedInteger(ScannerContextData &scd);
				static ScannerBaseState* handleTlByteMultiByteOctet(ScannerContextData &scd);
		};


		// ------------------------------------------------------------------------------------------------------------------------------
		// 3.2 Concrete states
		// ------------------------------------------------------------------------------------------------------------------------------

			// --------------------------------------------------------------------------------------------------------------------------
			// 3.2.1 Idle /  Wait for first ESC Start sequence								State No: 1
			// --------------------------------------------------------------------------------------------------------------------------
			DEFINE_SCANNER_STANDARD_STATE(ScannerStateIdle);

			// --------------------------------------------------------------------------------------------------------------------------
			// 3.2.2 Analyze the SML-TL byte												State No: 2
			// --------------------------------------------------------------------------------------------------------------------------
			DEFINE_SCANNER_STANDARD_STATE(ScannerStateAnalyzeTl);

			// --------------------------------------------------------------------------------------------------------------------------
			// 3.2.3 Read all bytes for an Octet											State No: 3
			// --------------------------------------------------------------------------------------------------------------------------
			DEFINE_SCANNER_STANDARD_STATE(ScannerStateReadOctet);

			// --------------------------------------------------------------------------------------------------------------------------
			// 3.2.4 Read all TL bytes for a Multi Byte Octet								State No: 4
			// --------------------------------------------------------------------------------------------------------------------------
			DEFINE_SCANNER_STANDARD_STATE(ScannerStateReadMultiByteOctet);

			// --------------------------------------------------------------------------------------------------------------------------
			// 3.2.5 Read a boolean value													State No: 5
			// --------------------------------------------------------------------------------------------------------------------------
			DEFINE_SCANNER_STANDARD_STATE(ScannerStateReadBoolean);

			// --------------------------------------------------------------------------------------------------------------------------
			// 3.2.6 Read a signed integer													State No: 6
			// --------------------------------------------------------------------------------------------------------------------------
			DEFINE_SCANNER_STANDARD_STATE(ScannerStateReadSignedInteger);

			// --------------------------------------------------------------------------------------------------------------------------
			// 3.2.7 Read an unsigned integer												State No: 7
			// --------------------------------------------------------------------------------------------------------------------------
			DEFINE_SCANNER_STANDARD_STATE(ScannerStateReadUnsignedInteger);




	} // end of namespace ScannerInternal



// ----------------------------------------------------------------------------------------------------------------------------------
// 4. Definition of Inline member functions
// ----------------------------------------------------------------------------------------------------------------------------------

inline void Scanner::reset(void)
{
	// Set initial state
	currentState = ScannerInternal::ScannerBaseState::getInstance<ScannerInternal::ScannerStateIdle>();
	// Reset ESC Analysis
	scd.escAnalysis.reset();
}



inline Scanner::Scanner(void) : currentState(ScannerInternal::ScannerBaseState::getInstance<ScannerInternal::ScannerStateIdle>()), scd()
{
	// Set initial state
}


// Main "worker" function
inline const Token &Scanner::scan(const EhzDatabyte ehzDatabyte)
{
	// Check for potential ESC Sequence
	scd.escAnalysisResultCode = scd.escAnalysis.analyse(ehzDatabyte);

	// The main state machine. Read bytes and produce tokens
	currentState = currentState->scan(ehzDatabyte, scd);

	// Return the found token
	return scd.token;
}

 


#endif
