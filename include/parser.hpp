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
//  Parsing of of SML File Data
//  
//  General Description:
//  
//  Data acquisition and parametrization of devices on low performance machines require
//  a compact and easy communication protocol.
//  
//  For example electronic meters for electricity have the need for easy communication 
//  with the rest of the world.
//  
//  The Smart Message Language (SML) has been developed based on that requirements.
//  SML has a data structure which is described in 
//  
//  http://www.emsycon.de/downloads/SML_081112_103.pdf
//  
//  The main structure is the SML File, which is a container for all other information.
//  The SML File has start and end indicators which are implemented with so called 
//  ESC - Sequences. 
//  
//  In between the Start and Stop sequence the net data is transmitted. SML defines 
//  primitive data types (Terminals) and container (Non Terminals).
//  
//  The SML language itself is defined in some kind of grammar.
//  
//  A recursive descent parser is used, to match the read data to the grammar. The parser is
//  implemented in this file.
//  
//  For the sake of clarity:
//  
//  1.  A "Lexer" or "Scanner" builds from raw data "Tokens" with related attributes. This task 
//      is done in scanner.c. That is called "lexical analysis"
//  2.  The "Parser" reads the "Tokens" provided by the "scanner" and matches them to the grammar.
//      Additionally it builds a "parse tree" with values. The parse tree may be traversed by
//  	other functions to retrieve needed information and to do further actions.
//  3.  The final goal of this process is to retrieve desired net data from the given data stream.
//  
//  At the bottom of this file you will find the SML Containers, like SmlMessage. They have no
//  functions defined. Everything is derived from the base classes. An iterator goes through all
//  defined contained classes and calls their macth functions. Additionaly the value is copied.
//  If the contained class is a container itself, a recursive algorithm is used to parse the
//  next container.
//  
//  Please note: This implementation is using the "push parser" concept. There is no buffer and
//  no look ahead symbol. The bytes read from the meter is pushed into the parser as soon as it
//  arrives. This is in contrast to standard implementations like yacc or bison (pull parsers).
//  
//  
//  
//  Several design patterns are used to achieve this task. 
//  
//  1.  The "interpreter" pattern is used to match tokens to expected values
//      --> Function: match
//  2.  The "composite" pattern helps to implement the parse tree
//      --> Function: parse
//  3.  The "visitor" pattern in a special implementation retrieves the desired information
//  	--> Class: Visitor
//  4.  The "mediator" pattern is responsible for data exchange between participating classes
//      --> Class: ParserContext
//  5.  The "factory" pattern supports building the "SML choice" structure
//      --> Class ChoiceFactory
//  6.  The "Iterator" patern is used for a depth first traversal of the parse tree
//      --> Function: traverseAndVisit
//  
//  The needed functionality is implemented using static polymorphism through generic programming
//  with templates and function overloading. Dynamic polymorphism is done with standard C++ virtual
//  functions and in the special case of the visitor pattern we use multi methods. Because only 2
//  polymorphic parameters are in place, we will use the term "double dispatch" instead of multi methods
//  from now on.
//  
//  The structure of the source files are:
//  1.  "parser.hpp"          Main declarations of classes, templates and inline functions
//  2.  "parser2.hpp"         "Implementation" of methods used in template classes (included
//                            by "parser.hpp" add the very end.
//  3.  "parser.cpp"          Implementation of methods for none template classes
//  
//  
//  In this file, "parser.hpp", at first some classes for primitive (Terminals) and container classes (
//  Non Terminals) are defined.
//  These serve as a basis for the later defined specific SML data Types as can be seen in the grammar.
//  
//  In accordance the former described patterns, there is one abstract base class for as a root class
//  for everything else: SmlElementBase
//  
//  The declaration of the SML Types are implicitly using the functionality of the base classes. In most
//  cases we will not see any method in those classes. Only special handling needs will result in a
//  separate declared "parse" function.
//  
//  It should be noted that the class "SmlContainer" is also abstract, although it is also derived
//  from SmlElementBase. The reason for that is, that using the "visitor pattern", we are not interested
//  in visiting primitive data types. "SmlContainer" is additionally derived from the base visitor class
//  "VisitableBase".
//  
//  I am using a special implementation of the visitor pattern explained by "Alexandrescu" in
//  "Modern C++ Design: Generic Programming and Design Patterns Applied". The work is based on
//  Robert C. Martin in "Acyclic Visitor"  (http://www.objectmentor.com/resources/articles/acv.pdf).
//  
//  The main reason is that I want to avoid cyclic name dependencies and forward declarations of
//  possibly incomplete data types.
//  
//  The idea has been modified by me to implement special needs:
//  1.  As already mentioned: I do not want to visit primitives. So the first occurrence of the accept
//      function is in "SmlContainer".
//  2.  I did not implement the "accept" function as a pure function but with an empty body. The 
//      disadvantage is of course that is may be possible to forget to implement an accept function in 
//  	a new class if some other visit functionality is needed. But since at this moment only one
//  	Container can be visited, I decided to go with an empty default function. So all derived
//  	functions that do not override the "accept" function will use the empty default.
//  
//  
#ifndef PARSER_H
#define PARSER_H
#include <math.h>

#include <vector>

#include "mytypes.hpp"
#include "scanner.hpp"

#include "factory.hpp"
#include "visitor.hpp"



//lint -save  -e1925 
//1942 Unqualified name ’Symbol’ subject to misinterpretation owing to dependent base class  // Ignored
//1925 Symbol 'Symbol' is a public data member
 
// ------------------------------------------------------------------------------------------------------------------------------
// 1. General definitions

	// The main interface for the parsing functionality is using a "parse" function
	// The "parse" function returns 3 possible value. Those are:
	struct ParserResult 
	{ 
		enum Code 
		{ 
			DONE, 			// Parse activity successful finished. Match OK and data stored
			PROCESSING, 	// Parse activity ongoing. Result not yet available
			ERROR 			// Error detected. Input not matching expected values  
		}; 
	};

	// Alias for "parser"- result code. Save typing work and make code more readable
	typedef ParserResult::Code prCode;  

	// Short cut: Constants (Aliases) for "parse result"-codes
	const prCode pr_DONE = ParserResult::DONE;				// Parse activity successful finished
	const prCode pr_PROCESSING = ParserResult::PROCESSING;  // Parse activity ongoing. Result not yet available
	const prCode pr_ERROR = ParserResult::ERROR;            // Error detected. Input not matching expected values  

	
// ------------------------------------------------------------------------------------------------------------------------------
// 2. Parser internal data and structures

	// Internal classes and structures
	namespace ParserInternal
	{


	// --------------------------------------------------------------------------------------------------------------------------
	// 2.1 Mediator class for internal information exchange


		// Mediator class to share information between parsing functions of different classes
		// All "parse" functions will use this as a parameter
		struct ParserContext
		{
			public:
				ParserContext(void) : token(null<Token *>()), crc16Calculator(), fillByteCounter(null<u8>()), ignoreRestOfSequence(false)  {};
				// Pointer to current token returned by the Scanner. The token is matched against the grammar
				// and used to store values, which will be copied into the parse tree
				const Token *token;						
				
				// CRC16 calculator for SML messages. SML messages are secured by a checksum. This class
				// is used specifically for SmlMessages. Other classes will not use this functionality
				Crc16Calculator crc16Calculator;	

				// Fill Byte (End Of Message) counter for SML files. According to the SML Specification the
				// length of a file must be modulo 4. So fill bytes before the ESC-End sequence may be used
				// the align the length of the file. Those fill bytes will be counted here
				u8 fillByteCounter;	
				
				// Flag necessary to handle the "ANY" Message-Body type. This is an extension to a standard
				// parser which would try to match anything against the grammar. This parser has been designed
				// for EDL 21 compatible electronic meters. Those meters will automatically send 3 SML messages
				// at roughly each 2 seconds. Since people may use this software also with other meters, this flag
				// has been introduced to ignore all none EDL21 compatible messages
				boolean ignoreRestOfSequence;			
		};

		
	// --------------------------------------------------------------------------------------------------------------------------
	// 2.2 Abstract base class for all SML Elements

		// This is the base class for all SML Data Elements. All other SML Elements are derived from this
		// class.
		
		// We will use the composite pattern. So there are "Container" (nodes) and "Primitive" (leaf) members.
		// All are derived from this common base class. All Elements are handled via pointers to this base class
		// The "container" contains primitives or again other containers.
		// 
		// The main function is the virtual pure "parse" function which is used to build the parse tree
		// and, using the interpreter pattern, matching the built tokens (from the input data stream) to
		// the grammar defined in:
		//
		// http://www.emsycon.de/downloads/SML_081112_103.pdf
		//
		// There is also one trait function that identifies the EmlElement as either a container
		// or a primitive, in compiler language as terminal or non terminal
		
		class SmlElementBase
		{
			public:
				SmlElementBase(void) {}
				virtual ~SmlElementBase(void) {}					
				// Main "parse" pure function to be implemented / overwritten by all derived classes
				virtual prCode parse(ParserContext &pc) = 0;		
				// Per default an SML Element is not a container
				virtual boolean isContainer(void) const { return false; }
		};
	
		
	// --------------------------------------------------------------------------------------------------------------------------
	// 2.3 Templates for generic primitives. Primitives are "terminals" in compiler language
		
		 // All primitives are following the same pattern. They analyse the token and match it to the expected 
		 // grammar element (interpreter pattern). If a token has an attribute (value) then this is stored. Since SML has
		 // the concept of an "optional" value, a mechanism for dealing with that situation is also available.

		// ----------------------------------------------------------------------------------------------------------------------
		// 2.3.1 Template for primitives that only have a type and maybe a length 

			template<const Token::TokenType tokenType, const TokenLength tokenLength=0UL>
			class SmlPrimitive : public SmlElementBase
			{
				public:
					SmlPrimitive(void) : SmlElementBase() {}
					virtual ~SmlPrimitive(void) {}
					virtual prCode parse(ParserContext &pc);	// Parse function (calls match function)

				protected:		
					virtual boolean match(const Token *const token);	// Match token with expected type and length
			};


		// ----------------------------------------------------------------------------------------------------------------------
		// 2.3.2 Template for Primitives that have a type, a value and maybe an associated length	

			template<typename ValueType, const Token::TokenType tokenType, const TokenLength tokenLength=0UL>
			class SmlPrimitiveWithValue : public SmlPrimitive<tokenType, tokenLength>
			{
				public:
				
					//lint -e{1960,915,919}
					//915 Implicit conversion (Context) Type to Type
					//919 Implicit conversion (Context) Type to Typ
					//Note 1960: Violates MISRA C++ 2008 Required Rule 5-0-4, Implicit conversion changes signedness
					SmlPrimitiveWithValue(void) : ::ParserInternal::SmlPrimitive<tokenType, tokenLength>::SmlPrimitive(), value() {}; 
					virtual ~SmlPrimitiveWithValue(void) {}
					// Parse function is inherited
					// Value will be stored here. Public because of visitor pattern
					ValueType value;	// 
				protected:
					virtual ::boolean match(const ::Token *const token); // Match token with expected type and length and store value
			};

			
		// ----------------------------------------------------------------------------------------------------------------------
		// 2.3.3 Template for Primitives that have a type, a (maybe optional) value and maybe an associated length	
		
			template<typename ValueType, const Token::TokenType tokenType, const TokenLength tokenLength=0UL>
			class SmlPrimitiveWithOptionalValue : public SmlPrimitiveWithValue<ValueType, tokenType, tokenLength>
			{
				public:
					SmlPrimitiveWithOptionalValue(void) :  ::ParserInternal::SmlPrimitiveWithValue<ValueType, tokenType, tokenLength>::SmlPrimitiveWithValue(), isOptional(false) {} 
					virtual ~SmlPrimitiveWithOptionalValue(void) {}
					// Parse function and value is inherited
					// Public because of visitor pattern
					::boolean isOptional;  // Indicates that Optional has been found. So no value is existing.
				protected:
					virtual ::boolean match(const ::Token *const token); // Match token with expected type and length and store value
			};

			
		// ----------------------------------------------------------------------------------------------------------------------
		// 2.3.4 A primitive that matches to everything and returns immediately "Done"
		
			// Can be used for optional value handling	
			class SmlPrimitiveAny : public SmlElementBase
			{
				public:
					SmlPrimitiveAny(void) : SmlElementBase() {}
					virtual ~SmlPrimitiveAny(void) {}
					//lint -e{1961}		// virtual member function 'Symbol' could be made const   // No
					virtual prCode parse(ParserContext &) { return pr_DONE; }  // Always OK

			};
		


	// --------------------------------------------------------------------------------------------------------------------------
	// 2.4 Templates for generic containers

		// "Container" contain Primitives or other "Containers". The matching algorithm of the Primitives is
		// used. Other containers are parsed recursively (depth first). SML defines several kinds of containers, 
		// which can be derived from this base class.
		//
		// Please note: After the parse tree has been built we can evaluate the container contents. For
		// this purpose we will use the visitor pattern and use a double dispatch mechanism.
		//
		// std::vector is used to store the Primitives. The iterator pattern is used to traverse the elements


		// ----------------------------------------------------------------------------------------------------------------------
		// 2.4.1 Abstract Base class for other containers. Can also be visited by a Visitor hierarchy.
		// Containers are "Non Terminals" in compiler language
		
			// The standard "Container" contains static elements. (Not dynamic created elements)
			class SmlContainer : public SmlElementBase, 
								 public VisitableBase		// Make class visitable
			{
				public: 
					// size type for vector
					typedef std::vector<SmlElementBase *>::size_type vst;
					
					// Ctor initializes the vector and the iterator for the vector
					explicit SmlContainer(const vst numberOfElements = 9U) : SmlElementBase(), VisitableBase(), smlElementContainer() , smlContainerIterator()
											{ smlElementContainer.reserve(numberOfElements); resetIterator();}
					virtual ~SmlContainer(void) {}
					
					// The parse function is again virtual pure abstract. We do not want a direct instantiation of the Container	
					virtual prCode parse(ParserContext &pc) = 0;
					
					// And yes: This is a container
					virtual boolean isContainer(void) const { return true; }
					
					// Traverse container and accept a visitor
					virtual void traverseAndVisit(VisitorBase *const visitor);

					// Define this class to be visitable. See description regarding visitor pattern
					DEFINE_VISITABLE()

				protected:
					std::vector<SmlElementBase *> smlElementContainer;				// The vector for storing other SML elements
					std::vector<SmlElementBase *>::iterator smlContainerIterator;	// And its associated iterator
							
					// Inline alias functions to save typing work and to make code more readable		
					void resetIterator(void) { smlContainerIterator = smlElementContainer.begin(); }
					void add(SmlElementBase  *const smlElement) { smlElementContainer.push_back(smlElement);}
					void addL(SmlElementBase *const smlElement) { smlElementContainer.push_back(smlElement);resetIterator();}
					//lint -e{1702} -e{1901}
					//1702 operator 'Name' is both an ordinary function 'String' and a member function 'String'
					//1901 Creating a temporary of type 'Symbol'
					void addSet(SmlElementBase *const smlElement) { smlElementContainer.push_back(smlElement);
						smlContainerIterator = smlElementContainer.end() - 1;}
			};

			
		// ----------------------------------------------------------------------------------------------------------------------
		// 2.4.2 Base class for container with dynamically created elements

		// Most SML-containers have static elements. But some container will be build during the parsing process.
		// A typical need for this kind of container is the "SML Sequence Of". Here the contents are not known in
		// the first place and will be created dynamically during the parsing process
		class SmlContainerDynamic : public SmlContainer
		{
			public:
				// Reserve default space for container. 9 is a magic number. This is a "good" estimate
				explicit SmlContainerDynamic(const vst numberOfElements = 9U) : SmlContainer(numberOfElements)	{}
				// Destructor will delete contained Elements and free the vector
				virtual ~SmlContainerDynamic() { try{releaseElements();}catch(...){}}
			protected:
				// Delete Elements and vector
				void releaseElements(const sint offset = 0);
				// Shortcut / alias function to keep the first Element in the container
				// According the SML the first Element in a container may be a SML List. No need to delete it
				void releaseElementsButNotTheFirst(void) { releaseElements(1); }
		};

// ------------------------------------------------------------------------------------------------------------------------------
// 3. SML Data Type


	// --------------------------------------------------------------------------------------------------------------------------
	// 3.1 SML primitive Data Types (Terminals)

		// See the language description for reference
		
		// SML List:
		// There are 2 types of SML Lists necessary. This has logical reasons and is needed for
		// the parsing process. Type 1 is a list with an expected length, as for example for a
		// SML message. Here the length needs to be 6. Otherwise it is an error.
		// For Type 2 the length of the List determines the number of elements in a dynamic container.
		// Here we need no match.
		
		// This is SML List type 1. A list with a given length that must match the expected value.
		template<const TokenLength tokenLength>
		class SmlListSpecific : public SmlPrimitiveWithValue<SmlListLength, Token::LIST, tokenLength> {};

		// SML Value:
		// SML Value can be of any data type. So we simple store the complete token value that we will
		// receive from the scanner
		class SmlValue : public SmlElementBase
		{
			public:
				SmlValue(void) : SmlElementBase(), token(), value(null<mdouble>()), sbs() {}
				virtual ~SmlValue(void) {}
				// Special handling: Copy token and finish. No match functionality
				virtual prCode parse(ParserContext &pc) { token = *pc.token; value = token.getDoubleValue(); token.getValueForType(&sbs); return pr_DONE; }
				
				Token token;
				mdouble value;
				SmlByteString sbs;
			protected:
				
		};
		
		// End of Message:
		// End of SML Message (0x00). Needs special handling for crc16 calculation for messages
		class EndOfSmlMessage : public SmlPrimitive<Token::END_OF_MESSAGE> 
		{
			public:
				EndOfSmlMessage(void) : SmlPrimitive() {}
				virtual ~EndOfSmlMessage(void) {}
				// Special Handling: Start CRC16 calculator and begin next calculation.
				virtual prCode parse(ParserContext &pc); 
		};
		
		// CRC16:
		// CRC16 data for messages. Check of crc16 for messages
		class Unsigned16crc : public SmlPrimitiveWithValue<u16, Token::UNSIGNED_INTEGER, 2UL>
		{
			public:
				Unsigned16crc(void) : SmlPrimitiveWithValue<u16, Token::UNSIGNED_INTEGER, 2UL>::SmlPrimitiveWithValue() {}
				virtual ~Unsigned16crc(void) {}
				// Special Handling: Calculate CRC16 and check it.
				virtual prCode parse(ParserContext &pc);
		};
		
		// End of SML File:
		// End of SML file is given by an Escape Sequence. This is pre evaluated in a separate function
		class SmlFileEnd : public SmlPrimitiveWithValue<EscSmlFileEndData, Token::END_OF_SML_FILE>
		{
			public:
				SmlFileEnd(void) : SmlPrimitiveWithValue<EscSmlFileEndData, Token::END_OF_SML_FILE>::SmlPrimitiveWithValue() {}
				virtual ~SmlFileEnd(void) {} 
				// Special handling. No Match. Check fill bytes
				virtual prCode parse(ParserContext &pc);
		};
		
		
		// SML List:
		// SML List type 2. Check for List and get and store the length
		typedef SmlPrimitiveWithValue<SmlListLength, Token::LIST> 									SmlList;
		
		// Start of an SML File:
		typedef SmlPrimitive<Token::START_OF_SML_FILE> 												SmlFileStart;
		
		// Match Octet and store String:
		typedef SmlPrimitiveWithValue<SmlByteString, Token::OCTET, MAX_SML_STRING_LEN> 				OctetString;

		// Match Octet and store string or "optional2:
		typedef SmlPrimitiveWithOptionalValue<SmlByteString, Token::OCTET, MAX_SML_STRING_LEN>	 	OctetStringOptional;

		// Match an unsigned 8 and store its value:
		typedef SmlPrimitiveWithValue<u8, Token::UNSIGNED_INTEGER, 1UL> 							Unsigned8;

		// Match an unsigned 8 and store its value or "optional":
		typedef SmlPrimitiveWithOptionalValue<u8, Token::UNSIGNED_INTEGER, 1UL> 					Unsigned8Optional;

		// Match a signed 8 and store its value:
		typedef SmlPrimitiveWithValue<s8, Token::SIGNED_INTEGER, 1UL> 								Integer8;

		// Match a signed 8 and store its value or "optional":
		typedef SmlPrimitiveWithOptionalValue<s8, Token::SIGNED_INTEGER, 1UL> 						Integer8Optional;

		// Match an unsigned 16 and store its value:

		typedef SmlPrimitiveWithValue<u16, Token::UNSIGNED_INTEGER, 2UL> 							Unsigned16;
		// Match an unsigned 32 and store its value:

		typedef SmlPrimitiveWithValue<u32, Token::UNSIGNED_INTEGER, 4UL> 							Unsigned32;
		// Match an unsigned 64 and store its value:

		typedef SmlPrimitiveWithValue<u64, Token::UNSIGNED_INTEGER, 8UL> 							Unsigned64;
		// Match an unsigned 64 and store its value or "optional":

		typedef SmlPrimitiveWithOptionalValue<u64, Token::UNSIGNED_INTEGER,  8UL> 					Unsigned64Optional;
		// Alias: Match a SmlSecIndex and store its value:

		typedef Unsigned32 																			SmlSecIndex;
		// Alias: Match a SmlTimestamp and store its value:

		typedef Unsigned32 																			SmlTimestamp;
		// Alias: Match a SmlStatus and store its value:

		typedef Unsigned64 																			SmlStatus; 
		// Alias: Match a SmlStatusOptional and store its value or "optional":

		typedef Unsigned64Optional 																	SmlStatusOptional; 
		// Alias: Match a SmlUnit and store its value:

		typedef Unsigned8 																			SmlUnit;
		// Alias: Match a SmlUnitOptional and store its value or "optional":

		typedef Unsigned8Optional 																	SmlUnitOptional;
		// Alias: Match a SmlSignature and store its value:

		typedef OctetString 																		SmlSignature;
		// Alias: Match a SmlSignatureOptional and store its value or "optional":

		typedef OctetStringOptional 																SmlSignatureOptional;
		
	// --------------------------------------------------------------------------------------------------------------------------
	// 3.2 SML compound Data Types (Non Terminals)
		
		// Templates for SML Container:
		//   --SmlSequence
		//   --SmlSequenceOf
		//   --SmlChoice

		// SML Sequence:
		// A SML Sequence is a SML container with with fixed size of Elements (Primitives or other containers)
		// The first Element is always a SML list, indicating the length of the list
	
		// The parameter "canBeIgnored" allows to ignore the rest of the matching functionality
		// for following container elements. This is for undefined (any) containers
		template<const TokenLength NumberOfSmlElements, const boolean canBeIgnored = false>
		class SmlSequence : public SmlContainer
		{
			public:
				// Reserve requested number of elements (+1 for the SML list)
				// Add the SML List as first Element
				//lint -e{917}	//917 Prototype coercion (Context) Type to Type
				SmlSequence(void) :  SmlContainer(NumberOfSmlElements+1UL), smlList()   {addL(&smlList);}
				virtual ~SmlSequence(void) {}
				// Parsing function for SML Sequence
				virtual prCode parse(ParserContext &pc);
			protected:
				// The SML List
				SmlPrimitiveWithValue<SmlListLength, Token::LIST, NumberOfSmlElements> smlList;
		};

		
		// SML SequenceOf:
		// A SML SequenceOf is a container with a variable number of Elements
		// It starts with a SML List, that indicates the number of Elements in the Sequence
		// We will store only pointer to base Elements in that list	
		template<class SmlElementType>
		struct SmlSequenceOf : public SmlContainerDynamic
		{ 
				//lint -e{1732,1733}
				//1732 new in constructor for class 'Name' which has no assignment
				//1733 new in constructor for class 'Name' which has no copy constructor
				SmlSequenceOf(void) : SmlContainerDynamic() { addL( new SmlList); }   // Add SML List as first Element in the container
				virtual ~SmlSequenceOf(void) { try{releaseElements();}catch(...){} }   // Destruct Elements and shrink container
				virtual prCode parse(ParserContext &pc);  // Parse it 
		};
		
		
		// SML Choice:
		// The SML Choice is a somehow tricky Element.
		// This is because here a late / lazy instantiation will be used. 
		
		// Example for "Messagebody". The type of the message body will be read late (after the starting 
		// bytes of the message have already been processed). And the message body determines the type 
		// of the message. A "factory" is used to create instances of the needed message bodies.
		// An SML choice always consists of 3 Elements:
		
		// --A list with length 2
		// --A tag (unsigned 32 or unsigned 16)
		// --The sub container body itself which is again a Sequence
		
		// The SML choice is derived from a sequence with 2 elements for the tag and the sub container
		// The SML choice factory is a template parameter and is used to create all kind of SML Choice elements
		template<class ChoiceFactory>
		class SmlChoice : public SmlSequence<2UL>
		{
			public:
				SmlChoice(void);
				// Destructor will delete the factory and the allocated message body 
				//lint -e{9008,1740}
				//1740 pointer member 'Symbol' (Location) not directly freed or zero'ed by destructor 
				//9008   comma operator used
				virtual ~SmlChoice(void) { try{delete choiceFactory; delete smlElementContainer[2U];smlElementContainer[2U]=null<SmlElementBase *>();}catch(...){}}
				// Also here the parse function
				virtual prCode parse(ParserContext &pc);
			protected:
				ChoiceFactory* choiceFactory;		// Choice factory that will be used
				Unsigned32 tag;						// Tag to select the choice
				SmlElementBase *specificSmlElement; // Sub Container element
		};

		
		// ----------------------------------------------------------------------------------------
		// For the definition of dedicated SML Choice objects we need to define the Factory classes

		// For SML Message Body
		class ChoiceFactorySmlMessageBody : public BaseClassFactory<u32, SmlElementBase>
		{
			public:
				ChoiceFactorySmlMessageBody(void);
				virtual ~ChoiceFactorySmlMessageBody(void) {}
				
				virtual SmlElementBase* createInstance(const u32 &selector);
		};
		
		// For SML Time
		class ChoiceFactorySmlTime : public BaseClassFactory<u32, SmlElementBase>
		{
			public:
				ChoiceFactorySmlTime(void);
				virtual ~ChoiceFactorySmlTime(void) {}
		};

		
		
	// --------------------------------------------------------------------------------------------------------------------------
	// 3.3 SML Container (Non Terminals)
		
		// Definition of concrete SML container objects
		// Please see again:
		//
		// http://www.emsycon.de/downloads/SML_081112_103.pdf
		//

		// SML Message Body:
		class SmlMessageBody : public SmlChoice<ChoiceFactorySmlMessageBody>
		{
			public:
				SmlMessageBody(void) : SmlChoice() {}
				virtual ~SmlMessageBody(void) {}
				// Special Handling because of possible "Any Message" data type
				virtual prCode parse(ParserContext &pc);
		};

		// SML Message Body ANY:
		// EDL21 compatible electronic meters just use 3 messages:
		// --SmlPublicOpenResponse
		// --SmlGetListResponse
		// --SmlPublicCloseResponse
		// If we should receive message bodies indicating a different message, we will use this
		// message body. It will eat up all tokens, until the message ends.
		class  SmlMessageBodyAny : public SmlElementBase
		{
			public:
				SmlMessageBodyAny(void) : SmlElementBase() {}
				virtual ~SmlMessageBodyAny(void) {}
				// Special Handling of "Any Message" data type
				virtual prCode parse(ParserContext &pc);
		};

		// SML Time:
		typedef SmlChoice<ChoiceFactorySmlTime> SmlTime;
		
		// SML Time Optional:
		class SmlTimeOptional : public SmlChoice<ChoiceFactorySmlTime>
		{
			public:
				SmlTimeOptional(void) : SmlChoice(), optionalValueRead(false) {}
				virtual ~SmlTimeOptional(void) {}
				// Special handling. We need to know, if the read data was optional
				virtual prCode parse(ParserContext &pc);
				// public member. Must be visble for visitor
				boolean optionalValueRead;
		};
		
		// SML List Entry
		class SmlListEntry : public SmlSequence<7UL>
		{
			public:
				virtual ~SmlListEntry(void) {}
				// Add container elements to internal vector
				SmlListEntry(void) : SmlSequence(), objName(),status(),valTime(),unit(),scaler(),value(),valueSignature()
				{
					add(&objName);
					add(&status);
					add(&valTime);
					add(&unit);
					add(&scaler);
					add(&value);
					addL(&valueSignature);
				}
				// SML Container elements 
				OctetString 			objName;
				SmlStatusOptional 		status;
				SmlTimeOptional 		valTime;
				SmlUnitOptional 		unit;
				Integer8Optional 		scaler;
				SmlValue 				value;
				SmlSignatureOptional 	valueSignature;
				
				// This container accepts a visitor
				DEFINE_VISITABLE() 
		};
		
		
		// SML Val List:
		typedef SmlSequenceOf<SmlListEntry> SmlValList;

		// SML Message: GetListResponse:
		class SmlGetListResponse : public SmlSequence<7UL>
		{
			public:
				virtual ~SmlGetListResponse(void) {}
				// Add container elements to internal vector
				SmlGetListResponse(void) : SmlSequence(),clientID(),serverID(),listName(),actSensorTime(),valList(),listSignature(),actGatewayTime()
				{
					add(&clientID);
					add(&serverID);
					add(&listName);
					add(&actSensorTime);
					add(&valList);
					add(&listSignature);
					addL(&actGatewayTime);
				}
			protected:
				// SML Container elements 
				OctetStringOptional 	clientID;
				OctetString 			serverID;
				OctetStringOptional 	listName;
				SmlTimeOptional 		actSensorTime;
				SmlValList 				valList;
				SmlSignatureOptional 	listSignature;
				SmlTimeOptional 		actGatewayTime;
		};
		
		// SML Message: Public Open Response:
		class SmlPublicOpenResponse : public SmlSequence<6UL>
		{
			public:
				virtual ~SmlPublicOpenResponse(void) {}
				// Add container elements to internal vector
				SmlPublicOpenResponse(void) : SmlSequence(), codepage(),clientId(),reqFileId(),serverId(),refTime(),smlVersion()
				{
					add(&codepage);
					add(&clientId);
					add(&reqFileId);
					add(&serverId);
					add(&refTime);
					addL(&smlVersion);
				}
			protected:
				// SML Container elements 
				OctetStringOptional 	codepage;
				OctetStringOptional 	clientId;		
				OctetString 			reqFileId;
				OctetString				serverId;
				SmlTimeOptional 		refTime;
				OctetStringOptional 	smlVersion;
		};

		
		
		// SML Message: Public Close Response:
		class SmlPublicCloseResponse : public SmlSequence<1UL>
		{
			public:
				virtual ~SmlPublicCloseResponse(void) {}
				// Add container elements to internal vector
				SmlPublicCloseResponse(void) : SmlSequence(), globalSignature()
				{
					addL(&globalSignature);
				}
			protected:
				// SML Container elements 
				SmlSignatureOptional 	globalSignature;
		};

		// General SML Message:
		class SmlMessage : public SmlSequence<6UL, true>
		{
			public:
				virtual ~SmlMessage(void) {}
				// Add container elements to internal vector
				SmlMessage(void) : SmlSequence(), transactionId(),groupNo(),abortOnError(),messageBody(),crc16(),endOfSmlMessage()
				{ 
					add(&transactionId);
					add(&groupNo);
					add(&abortOnError);
					add(&messageBody);
					add(&crc16);
					addL(&endOfSmlMessage);
				}
			protected:
				// SML Container elements 
				OctetString 		transactionId;
				Unsigned8 			groupNo;
				Unsigned8 			abortOnError;
				SmlMessageBody 		messageBody;
				Unsigned16crc 		crc16;
				EndOfSmlMessage 	endOfSmlMessage;
		};
		
		// SML File:
		// Root Container for everything(consists of SML Messages):
		// The SML File needs special handling because it has a heterogenous structure.
		// It consists of an SML Start and STop sequence, SML Messages and may have 
		// fill bytes.
		class SmlFile : public SmlContainerDynamic
		{
			public:
				SmlFile(void); 
				virtual ~SmlFile(void) { try{releaseElements();}catch(...){} } 
				virtual prCode parse(ParserContext &pc);
				// Reset container 
				// Make it empty for next telegram or 
				// in case of error
				void reset(void);	

				
			protected:
				boolean parseMessage;	// Flag for internal handling
		};



// ------------------------------------------------------------------------------------------------------------------------------
// 4. Extract data from Parse Tree

	// After the parse tree has been build, the visitor pattern is used to extract the needed
	// data. Data are encoded in the SmlListEntry container. Different data are marked with 
	// OBIS indices. So here we will look, if an OBIS value, encoded in the objName member
	// of the SmlListEntry container. 
	// If we find a match, then we will read and copy the value

		
		//lint -e{1790}  // 1790 Base class 'Symbol' has no non-destructor virtual functions  // Yes
		class VisitorForSmlListEntry : public VisitorBase, // required
									   public Visitor<SmlListEntry>   //
										//public Visitor<SmlContainer> // 
		{
			public:
				VisitorForSmlListEntry(void) : VisitorBase(), Visitor<SmlListEntry>() {}
				virtual ~VisitorForSmlListEntry(void) {}
				//virtual void visit(SmlContainer&) { ui.msgf("SmlContainer  \n"); }
				virtual void visit(SmlListEntry &smlListEntry) = 0;    // Derived Class will do the work
			protected:
		};

		
		
		
		
		

	} // End Namespace ParserInternal	
// ------------------------------------------------------------------------------------------------------------------------------
// 5. The Parser

// This is the only class in the global namespace. Everything is handled here.

// There is one main public function:

// 		prCode parse(EhzDatabyte databyte);

// This function takes a raw databyte and returns pr_Done, after a complete SML File has been read.



class Parser
{
	public: 
		Parser(void) : smlFile(),pc(),scanner() {}
		~Parser(void) {}
	
		prCode parse(const EhzDatabyte databyte, const uint ehzIndex);
		
		void reset(void) { smlFile.reset(); }
		void traverseAndEvaluate(ParserInternal::VisitorForSmlListEntry * const visitorForSmlListEntry );

	protected:
		// Main / Root Cointainer
		ParserInternal::SmlFile smlFile;
		
		// The context for the parsing process
		ParserInternal::ParserContext pc;
		// The Scanner(Lexer). This will produce the tokens
		Scanner scanner;
};


//lint -restore


#endif







