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
#ifndef PARSER_TPP
#define PARSER_TPP

 
 
// This file contains method definitions for class templates
// This is also an include file. It will be included by parser.hpp
// It has been separated in order to have a better structure.

// Note, methods for class template should be in an include file (C++ 98, now, 2015)
// See for example:
// http://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file




// ------------------------------------------------------------------------------------------------------------------------------
// 1. Match functions

// According to the interpreter pattern, all tokens (and their sequence and probably also their length)
// have to be matched against the given grammar. During the parsing process, the "match" functions, will
// be called from the corresponding parse functions of the primitives, so for everything

// So this functions are not so visible but are the working horse of the parser. 


// All functions belong to the internal namespace. Nothing should be used from the outside world
namespace ParserInternal
{

//lint -save -e1961 -e952
//1961 virtual member function 'Symbol' could be made const
//952 Parameter 'Symbol' (Location) could be declared const
	// --------------------------------------------------------------------------------------------------------------------------
	// 1.1 Match function for Primitive with Type and length

		// There are several tokens, where only type and length are important. This is checked here.
		// If the "tokenLength" parameter ist 0 (what is the default), then only the type will be checked.
		template<const Token::TokenType tokenType, const TokenLength tokenLength>
		boolean SmlPrimitive<tokenType, tokenLength>::match(const Token *const token)
		{
			// The type must match in any case. So check, if it is correct
			boolean rc = (token->getType() == tokenType);
			// Now we evaluate, if also the length of the token should be checked
			// But this will only be done, if the type is OK
			if ((null<TokenLength>() != tokenLength) && rc)  // If token length == 0 the compiler should eliminate this
			{
				//lint -e{775}   // non-negative quantity cannot be less than zero
				// So, now check additionaly the token length
				rc = (token->getLength() <= tokenLength); 
			}
			return rc;
			
			
		}

		
	// --------------------------------------------------------------------------------------------------------------------------
	// 1.2 Match function for Primitive with Type and length and an associated value
	
		// There are tokens, where type and length are important. This is checked here. Additionally
		// Tokens may have values. These are taken from the token and stored in the parse tree
		// If the "tokenLength" parameter ist 0 (what is the default), then only the type will be checked.
		template<typename ValueType, const Token::TokenType tokenType, const TokenLength tokenLength>
		boolean SmlPrimitiveWithValue<ValueType, tokenType, tokenLength>::match(const ::Token *const token) 
		{ 
			// Use base class functionality to check for type and length
			const ::boolean rc = this->SmlPrimitive<tokenType, tokenLength>::match(token);
			
			// If everything matched so far . . .
			if  (rc) 
			{
				// Then we read the value of the token
				token->getValueForType(&value);
			}
			return rc;
		}

	// --------------------------------------------------------------------------------------------------------------------------
	// 1.3 Match function for Primitive with Type and length and an associated optional value
	

		// There are tokens, where type and length are important. This is checked here. Additionally
		// Tokens may have values. These are taken from the token and stored in the parse tree
		// If the "tokenLength" parameter ist 0 (what is the default), then only the type will be checked.
		
		// Values may also be optional, meaning omitted. In this case type, length are not checked and
		// there is no value
		template<typename ValueType, const Token::TokenType tokenType, const TokenLength tokenLength>
		boolean SmlPrimitiveWithOptionalValue<ValueType, tokenType, tokenLength>::match(const ::Token *const token) 
		{ 
			// Assume a positive match
			::boolean rc = true;
			// Check, if value (Type) is optional/omitted, so not given. And remember this condition
			isOptional = (token->getType() == ::Token::OPTIONAL);
			// If the value is available then use the functionality of the base class to do the matching
			if (!isOptional)
			{
				rc = this->SmlPrimitiveWithValue<ValueType, tokenType, tokenLength>::match(token);
			}
			return rc;
		}

		

// ------------------------------------------------------------------------------------------------------------------------------
// 2. Parse function

	// The Parse function call the match functions to check, if everything is according to
	// the definded grammer. Then, if data and structure is available, the parse tree will be build
	
	// Since we have a push parser, the raw data arrive byte by byte. In this case the
	// state pr_Processing is returned. Otherwise we might have an Error or an OK result
	
	
	
	// --------------------------------------------------------------------------------------------------------------------------
	// 2.1 Parse function for Primitives. Matches tokens and stores resulting associated values 
		
		template<const Token::TokenType tokenType, const TokenLength tokenLength>
		prCode SmlPrimitive<tokenType, tokenLength>::parse(ParserContext &pc)
		{
			// Assume Error
			prCode rc = pr_ERROR;
			// Match the token against the expected data
			if (this->match(pc.token))
			{
				// And if OK, the parsing for this primitive is OK and done.
				rc = pr_DONE;
			}
			return rc;
		}

		
	// --------------------------------------------------------------------------------------------------------------------------
	// 2.2 Parse function for Container SmlSequence
		
		// The SmlSequence is a container (a compound type) with a given length
		// The parameter "canBeIgnored" allows to ignore the rest of the matching functionality
		// for following container elements.
		// This was introduced to handle the "Any message" type. For none EDL21 compatible meters
		// also other (than the standard) messages could arrive. They will be ignored by this 
		// implementation of the parser. The "canBeIgnored" flag handles that.
		template<const TokenLength NumberOfSmlElements, const boolean canBeIgnored>
		prCode SmlSequence<NumberOfSmlElements, canBeIgnored>::parse(ParserContext &pc) 
		{ 
			//lint --e{788}  //enum constant 'Symbol' not used within defaulted switch  // Not true
			// We assume, that we are not finished yet and still have container elements to check
			prCode rc = pr_PROCESSING;
			// Call the container elements parser function. This can result in a depth first recursion
			// Depending on the result . . .
			switch ((*smlContainerIterator)->parse(pc))
			{
				// Check if the current element is not done yet. If so, stay here and do nothing
				case pr_PROCESSING:
					break;
					
				// Check if the current element has been successfully parsed
				case pr_DONE:
					// Current element was OK, so goto the next
					//lint -e{9049,1963}
					//9049   increment/decrement operation combined with other operation with side-effects
					//Note 1963: Violates MISRA C++ 2008 Advisory Rule 5-2-10, increment or decrement combined with another operator
					++smlContainerIterator;
					// If we reached the last element in the container or
					// if we are no longer interested in any checking
					//lint -e{1960,9007,944,845,774}
					// Note 1960: Violates MISRA C++ 2008 Required Rule 5-14-1, side effects on right hand of logical operator: '||'  // No
					// Note 9007: side effects on right hand of logical operator, ''||''   // No
					// Note 944: Right argument for operator '&&' always evaluates to False [Reference: file p:\project\ehz\include\parser2.hpp: line 168] [MISRA C++ Rule 0-1-1], [MISRA C++ Rule 0-1-2], [MISRA C++ Rule 0-1-9]
					// Info 845: The right argument to operator '&&' is certain to be 0 [Reference: file p:\project\ehz\include\parser2.hpp: line 168]    // Depends on template parameter
					// Info 774: Boolean within 'left side of || within if' always evaluates to False [Reference: file p:\project\ehz\include\parser2.hpp: line 168] [MISRA C++ Rule 0-1-1], [MISRA C++  Rule 0-1-2], [MISRA C++ Rule 0-1-9]
					if ((pc.ignoreRestOfSequence && canBeIgnored) || (smlElementContainer.end() == smlContainerIterator))
					{
						// The parsing of all elements in the container is done
						rc = pr_DONE;
						// The next sequence shall be avalutaed completely again
						pc.ignoreRestOfSequence = false;
						// Set the container iterator to the first element for the next check
						resetIterator();
					}
					break;
				
				// In case of error or unknown result			
				case pr_ERROR:  // fallthrough
				default:
					// Set the container iterator to the first element for the next check
					resetIterator();
					// Report error
					rc = pr_ERROR;
					break;
			}
			return rc;
		}


	// --------------------------------------------------------------------------------------------------------------------------
	// 2.3 Parse function for Container SmlSequenceOf
	
		// This is a container, with a dynamic LENGTH. 
		// (This is in contrast to the SmlChoice, which has a dynamic TYPE)
		// The elements are build during the parsing process.
		// The first element of a SmlSequenceOf is always a SmlList. This List gives the number
		// of elements that the container should have. After reading the SmlList, the requested
		// Elements (The "Of" part of SmlSequenceOf) are created and parsed

		template<class SmlElementType>
		prCode SmlSequenceOf<SmlElementType>::parse(ParserContext &pc) 
		{ 
			//lint --e{788}  //788 enum constant 'Symbol' not used within defaulted switch   // Wronmg
			// Assume the the parsing for the container elements is ongoing
			prCode rc = pr_PROCESSING;
			
			// Special handling for the first Element in the container, which must be a SmlList
			// So check, if we are parsing the first Element
			if (smlElementContainer.begin() == smlContainerIterator)
			{
				// Yes, first element will be evaluated
				// A SmlList must return immediately with a done, because it is encoded 
				// in 1 raw data byte, Therefore we test for "Done"
				// The parse functions reads of course also the value (length) of the SmlList
				if (pr_DONE == (*smlContainerIterator)->parse(pc))
				{
					// Cast to SmlList and make sure that it is really an SmlList. So we do a 2nd check
					//lint -e{929}   //929 Cast from pointer to pointer  // Yes
					const SmlList *const smlList = dynamic_cast<SmlList *>(*smlContainerIterator);
					// Was dynamic cast successfull?
					if (null<SmlList *>()  != smlList)
					{
						// Ok, it was a SML List. Get Length. That is then the number of Elements
						// in the SmlSequenceOf
						//lint -e{921,613}
						//921 Cast from Type to Type   // Yes
						//613 Possible use of null pointer 'Symbol' in [left/right] argument to operator 'String' Reference // No
						const TokenLength elementsInSequence = static_cast<TokenLength>(smlList->value.lenght);
						// Delete old/previous elements and their pointer in the container
						// Keep the first Element, the SmlList
						releaseElementsButNotTheFirst();
						// Create new container elements
						for (TokenLength i=null<TokenLength>(); i<elementsInSequence;i++)
						{
							add( new SmlElementType);
						}
						// Goto 2nd element in container. Next time we will analyze the first real 
						// Element (the "Of" Part)  of the SmlSequenceOf
						//lint -e{1702,1901}
						//1702 operator 'Name' is both an ordinary function 'String' and a member function 'String'  //No
						//1901 Creating a temporary of type 'Symbol' // Aha
						smlContainerIterator = smlElementContainer.begin()+1;
					}
					else 
					{
						rc = pr_ERROR;
					}
				}
				else 
				{
					rc = pr_ERROR;		
				}
			}
			else 
			{
				// We are not evaluating the first Element of the container (the SmlList)
				// Now we are checking members of the Sequence.
				// Call their parse function. This can result in a depth first recursion
				// Depending on the result . . .
				switch ((*smlContainerIterator)->parse(pc))
				{
					// Check if the current element is not done yet. If so, stay here and do nothing	
					case pr_PROCESSING:
						break;

					// Check if the current element has been successfully parsed
					case pr_DONE:
						// Current element was OK, so goto the next
						//lint -e{9049,1963}
						//9049   increment/decrement operation combined with other operation with side-effects
						// Note 1963: Violates MISRA C++ 2008 Advisory Rule 5-2-10, increment or decrement combined with another operator
						++smlContainerIterator;
						// If we reached the last element in the container
						if (smlElementContainer.end() == smlContainerIterator)
						{
							// Then also this SmlSequenceOf is OK and done
							rc = pr_DONE;
						}
						break;

					// In case of error or unknown result			
					case pr_ERROR:  // fallthrough
					default:
						rc = pr_ERROR;
						break;
				}
			}
			// If Error or OK and done. We need to reset the iterator to the first element
			if (pr_PROCESSING != rc)
			{
				resetIterator();
			}
			return rc;
		}



	// --------------------------------------------------------------------------------------------------------------------------
	// 2.4 Parse function for Container SmlChoice

		// This is a container with a fixed size, but with one member of a dynamic TYPE
		// This is in contrast to the SmlSequenceOf which has a dynamic LENGTH)
		
		// The SML choice consists of 3 Elements:
		// 1.  An SmlList, always with Length 2 
		// 2.  A "tag" that determines the TYPE of the following element
		// 3.  The "element" itself which is of type "TYPE"
		// The SmlChoice is derived from a SmlSquence<2>. So the handling of the first Element,
		// the SmlList, is used from the super function
		// The ChoiceFactory is used to dynamically create the required type
		template<class ChoiceFactory>
		prCode SmlChoice<ChoiceFactory>::parse(ParserContext &pc)
		{
			//lint --e{788}   // 788 enum constant 'Symbol' not used within defaulted switch   // Not true
			// We assume that the current Element is still beeing evaluated
			prCode rc = pr_PROCESSING;
			
			// Call the container elements parser function. 
			// This will result in a depth first recursion, especially for the 3rd element
			// Depending on the result . . .
			switch ((*smlContainerIterator)->parse(pc))
			{
				// Parsing for this Element (1. or 2. or 3.) is ongoing. Do nothing
				case pr_PROCESSING:
					break;
					
				// The parsing of one of the container Elements (1. or 2. or 3.) is Done and OK
				case pr_DONE:
					// We need a different Handling for Each of the Elements (1. or 2. or 3.)
					//lint -e{1901,1911}
					//1901 Creating a temporary of type 'Symbol
					//1911 Implicit call of constructor 'Symbol' (see text)
					switch (std::distance(smlElementContainer.begin(), smlContainerIterator))
					{
						// Element 1. at Offset 0: The SmlList
						case 0:
							// We are not interested in the Element itself. But at this point in Time
							// we want to do something, so
							// Delete the dynamic Element (3.)
							//lint -e{9008}   // 9008   comma operator used  // lint bug
							delete smlElementContainer[2U];
							// And set the pointer in the vector to 0, creating no harm, if we delete again
							smlElementContainer[2U] = null<SmlElementBase *>();
							// And then parse the next Element (The "tag")
							//lint -e{9049,1963}
							//9049   increment/decrement operation combined with other operation with side-effects
							//Note 1963: Violates MISRA C++ 2008 Advisory Rule 5-2-10, increment or decrement combined with another operator
							++smlContainerIterator;
							break;
							
						// Element 2. at offset 1: The "tag"	
						case 1:
							{
								// Read the value of the "tag". This serves as a selector for the Choice Factory
								//lint -e{929}    // 929 Cast from pointer to pointer
								const Unsigned32 *const unsigned32ForTag = dynamic_cast<Unsigned32 *>(*smlContainerIterator);
								u32 selector = 0UL;
								if (null<Unsigned32 *>() != unsigned32ForTag)
								{
									//lint -e{613}  // 613 Possible use of null pointer 'Symbol' in [left/right] argument to operator 'String' Reference  // No
									selector = unsigned32ForTag->value;
								}
								
								// Set iterator already now to Element 3. (the TYPE), so that we can assign a value
								//lint -e{9049,1963}
								//9049   increment/decrement operation combined with other operation with side-effects
								//Note 1963: Violates MISRA C++ 2008 Advisory Rule 5-2-10, increment or decrement combined with another operator
								++smlContainerIterator;
								
								// Create the new type
								 SmlElementBase *const smlElementBase = choiceFactory->createInstance(selector);
								
								// If we could not create a type from the requested selector, this is an error
								if (null<SmlElementBase *>() == smlElementBase) 
								{
									rc = pr_ERROR;
								}
								else
								{
									// Standard handling. The factory could create a type. Assign this to the
									// container (element 3.)
									*smlContainerIterator = smlElementBase;
								}
							}
							break;
							
						// Element 3. at offset2: The Type
						case 2:
							// If the Type has been parsed successfully then also the SmlChoice is now OK and done
							rc = pr_DONE;
							break;
						
						// Should never happen. Programming error
						default:
							rc = pr_ERROR;
							break;
					}
					break;
					
				// In case the parsing of the subelement produced an error	
				case pr_ERROR:  // fallthrough
				default:
					rc = pr_ERROR;
					break;
			}
			// If Error or OK and done. We need to reset the iterator to the first element
			if (pr_PROCESSING != rc)
			{
				resetIterator();
			}
			return rc;
		}

	// --------------------------------------------------------------------------------------------------------------------------
	// 2.5 Parse function for EndOfSmlMessage

		// The End of a SML Message needs a special handling, because after Parsing a SML Message
		// the CRC16 calculator (for next messages) needs to be started. This assumes that after
		// this message, a next message will begin.
		inline prCode EndOfSmlMessage::parse(ParserContext &pc)
		{
			// Call base class parse function for the SmlEndOfMessage
			const prCode pr = this->SmlPrimitive<Token::END_OF_MESSAGE>::parse(pc);
			// And if this is done and OK
			if (pr_DONE == pr)
			{
				// Restart thr CRC16 calculator, so that the next message is calculated starting
				// with its first byte
				pc.crc16Calculator.start();
			}
			return pr;
		}
		
	// --------------------------------------------------------------------------------------------------------------------------
	// 2.6 Parse function for Unsigned16crc
		
		// After Unsigned16crc has been read, the checksum will be compared with the calculated checksum
		inline prCode Unsigned16crc::parse(ParserContext &pc)
		{
			// Call base class parse function for the crc16
			prCode pr = this->SmlPrimitiveWithValue<u16, Token::UNSIGNED_INTEGER, 2UL>::parse(pc);
			// If that hase been parsed OK
			if (pr_DONE == pr)
			{
				// Compare checksum stored in this value with calculated checksum for the message
				//lint -e{921}  //921 Cast from Type to Type
				if (pc.crc16Calculator.getResult() != static_cast<crc16t>(this->value))
				{
					// In case of not matching values report an error
					pr = pr_ERROR;
				}
			}
			return pr;
		}

		
// ------------------------------------------------------------------------------------------------------------------------------
// 3. Other, general functions
		
	// Delete elements from dynamic container. So, call destructor for the element,
	// set the element pointer to 0 (so that we can delete again with no harm) and
	// then delete the pointer from the vector of the dynamic container
	
	// We have the special requirement, that for some container we do want to keep certain elements
	// and want only to delete parts of the vector. This is handled with the offset parameter
	void SmlContainerDynamic::releaseElements(const sint offset) 
	{
		// Delete old Elements
		// Initialize an iterator with the first (+offset) element
		//lint -e{1702,1901}
		//1702 operator 'Name' is both an ordinary function 'String' and a member function 'String'  //No
		//1901 Creating a temporary of type 'Symbol' // Aha
		std::vector<SmlElementBase *>::iterator it = smlElementContainer.begin() + offset;
		// Go through all elements in the container
		while(smlElementContainer.end() != it)  // potential problem with offset greater than vector
		{
			// Delete Element stored in the container and set it to 0
			//lint -e{9008}
			delete *it; 
			*it = null<SmlElementBase*>();
			// Goto next element
			++it;
		}
		// Delete pointer from container
		//lint -e{1702} -e{1901} -e{534}
		//1702 operator 'Name' is both an ordinary function 'String' and a member function 'String'  //No
		//1901 Creating a temporary of type 'Symbol' // Aha
		//534 Ignoring return value of function 'Symbol
		smlElementContainer.erase(smlElementContainer.begin()+offset, smlElementContainer.end());
		// Set container iterator back to first element
		resetIterator();
	}


	// Constructors for SmlChoice
	
	// A choice factory is created (according to the template parameter)
	// The SmlList (the first element) has already been created by the base class
	//lint -e{1732,1733}
	//1732 new in constructor for class 'Name' which has no assignment operator
	//1733 new in constructor for class 'Name' which has no copy constructor
	template<class ChoiceFactory>
	SmlChoice<ChoiceFactory>::SmlChoice(void) : SmlSequence<2UL>(), choiceFactory(new ChoiceFactory), tag(),specificSmlElement(null<SmlElementBase *>())
	{ 	
		// Create a factory for getting instances of specific types
		//choiceFactory = new ChoiceFactory;
		// Add a "tag" statically
		add(&tag);
		// and add the 3rd element, which will be dynamically created
		addL(specificSmlElement);
	}

}

 
//lint -restore

#endif
