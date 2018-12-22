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
// Method definitions for parser related classes
// This are the definitions of the methods for none class templates.
// Methods for class templates are defined in "parser.tpp"
// 
// In this module mainly "parse" functions are defined which do not follow
// the typical "match-and-store-value" scheme.
//


#include "parser.hpp"
//lint -esym(1960,3-1-1)		// Header cannot be included in more than one translation unit because of the definition of
#include "parser2.hpp"
#include "bytestring.hpp"
#include "userinterface.hpp"


// ------------------------------------------------------------------------------------------------------------------------------
// 1. Parse functions

// Here we define parse functions that need some kine of special handling. The basic principle with
// eating tokens and return a pr_Done on success is the same as for others
namespace ParserInternal
{

	// --------------------------------------------------------------------------------------------------------------------------
	// 1.1 SMLFileEnd:
	
		// Parse function for the SmlFileEnd primitive. The data stream encodes the SML File End in a
		// ESC sequence. Part of the ESC Sequence are the checksum and the number of fill bytes.
		// Fill Byte: According to the SML specification defined in
		// http://www.emsycon.de/downloads/SML_081112_103.pdf
		// the length of a SML file has to be modulo 4
		
		// So fill bytes (0x00) will be inserted after the last SmlMessage to achieve the correct padding
		// in the ESC File End sequence the number of fill bytes is mentioned. So we can compare this
		// number with the previously counted number.
		
		// The checksum is handled by the ESC Analysis function directly.

		prCode SmlFileEnd::parse(ParserContext &pc)
		{
			prCode pr = this->SmlPrimitiveWithValue<EscSmlFileEndData, Token::END_OF_SML_FILE>::parse(pc);
			if (pr_DONE == pr)
			{
				//lint -e{911}  // 911 Implicit expression promotion from Type to Type
				if (pc.fillByteCounter != value.numberOfFillBytes)
				{
					pr = pr_ERROR;
				}	
			}
			return pr;			
		}

	// --------------------------------------------------------------------------------------------------------------------------
	// 1.2 SmlTimeOptional:

		// SmlTimeOptional is actually a compound type. See SML description for details.
		// This is the version with an "optional" variant. So it may be there or it may be 0x01
		
		// Hence we need a special handling. The class is derived from SmlTime which is a SmlChoice
		// So we need to handle the "SmlChoice" part of this class correctly.
		
		prCode SmlTimeOptional::parse(ParserContext &pc)
		{
			// Assume that we are still processing the token
			prCode rc = pr_PROCESSING;
			// Now check, if we got an optional token, which may only arrive at the first position in a SML Time element
			//lint -e{1960} -e{9007}
			// Note 1960: Violates MISRA C++ 2008 Required Rule 5-14-1, side effects on right hand of logical operator: '||'  // No
			// Note 9007: side effects on right hand of logical operator, ''||''   // No
			if ((smlContainerIterator == smlElementContainer.begin()) && (Token::OPTIONAL == pc.token->getType()) )
			{
				// See the SmlChoice class for reference. Delete the old specific element
				//lint -e{9008}  // 9008   comma operator used // Bug in Lint
				delete (smlElementContainer[2U]);
						
				// Assign something new. Note. This will never be evaluated.
				smlElementContainer[2U]= new SmlPrimitiveAny;
				// And since optional consists only of one byte, we are immediately done and OK
				rc = pr_DONE;
				// Anywere we store the "optional" condition in the parse tree for later evaluation
				optionalValueRead = true;
			}
			else
			{
				// Although the value is optional, in this case we received data
				// So we use the standard parse function from our base class (SmlTime)
				rc = this->SmlTime::parse(pc);
				// And we indicate that a value is existing (so not optional)
				optionalValueRead = false;
			}
			return rc;
		}

	// --------------------------------------------------------------------------------------------------------------------------
	// 1.3 SmlMessageBody:
	
		// As described for the SmlMessageBodyAny, we introduced a special handling for
		// not EDL21 compliant meters. In case we have such a "unknown" message body, everything
		// up to the end of the message, may be ignored.
		
		// This functionality is implemented here
		prCode SmlMessageBody::parse(ParserContext &pc)
		{
			// First use the standard parse function of a "normal" message body
			const prCode pr = this->SmlChoice<ChoiceFactorySmlMessageBody>::parse(pc);
			if (pr_DONE == pr)
			{
				// if we do NOT ignore the rest of the MessageBody and the message
				// So for the standard handling of a Message
				if (!pc.ignoreRestOfSequence) 
				{
					// Stop the crc16 calculation now, because of after the SmlMessageBody
					// the next byte will be the checksum (stored in SmlMessage). This byte can of
					// course not be used to calculate the checksum
					pc.crc16Calculator.stop(); 
				}
			}
			return pr;
		}
		
	// --------------------------------------------------------------------------------------------------------------------------
	// 1.4 SmlMessageBodyAny:
			
	// Special handling of SmlMessageBodyAny. This is a not EDL 21 compliant MessageBody or any kind of unknown 
	// Message body. In this case all input bytes will be eaten until we receive the end of the message. The problem 
	// with this is that the SmlMessageBody is a sub container of the SmlMessage. And since we eat all bytes until 
	// the end of the message we go back over the edge of the SmlMessagebody into the SmLMessage and it its bytes also.
	// For this we need a special handling. We use the ignoreRestOfSequence-flag to indicate this situation.
		//lint -e{1961}   // 1961 virtual member function 'Symbol' could be made const
		prCode SmlMessageBodyAny::parse(ParserContext &pc)
		{
			// Assume that the parsing is still ongoing
			prCode rc = pr_PROCESSING;
			
			// Eat every byte until we receive an EndOfMessage (0x00).
			// So did we receive this know?
			if (Token::END_OF_MESSAGE == pc.token->getType())
			{
				// Indicate that we are ignoring everything
				pc.ignoreRestOfSequence = true;
				// We received the EndOfmessage: So we are OK and done
				rc = pr_DONE;
				// Start the crc16 calculator for reading the next message
				// If there is no next message, then we do not care and calculate some nonsense.
				pc.crc16Calculator.start(); 
			}
			return rc;
		}
	// --------------------------------------------------------------------------------------------------------------------------
	// 1.4 SmlFile:

		// This is the main container for all SML Data. All net data information is packed into
		// an SML File. The SML File consists of an :
		//  - ESC start sequence
		//  - One or more SmlMessages
		//  - 0 or 1 or 2 or 3 Fill bytes (0x00)
		//  - An ESC Stop sequence
		// There is no indicator on how many elements are in that container. Therefore we need
		// a special handling to distinguish between the different elements
		
		prCode SmlFile::parse(ParserContext &pc)
		{
			//lint --e{788}  // 788 enum constant 'Symbol' not used within defaulted switch // Wrong message
			// For the time being we assume that we are still parsing a SmlFile
			prCode rc = pr_PROCESSING;
			
			// First we must detect, what kind of data we are waiting for. As described:
			// At first we do not parse messages. But wait for an indicator on what to do
			if (!parseMessage)
			{
				// Check, if we are looking at the first element of the SML File
				if (smlElementContainer.begin() == smlContainerIterator)
				{
					// If so, then we need to wait for a ESC-Start
					if (Token::START_OF_SML_FILE == pc.token->getType())
					{
						reset();					// Reset iterator. Release all elements from previous parsing 
						//lint -e{1963} -e{9049} 
						//9049   increment/decrement operation combined with other operation with side-effects
						//Note 1963: Violates MISRA C++ 2008 Advisory Rule 5-2-10, increment or decrement combined with another operator
						++smlContainerIterator;		// Set iterator to element after ESC start. Which should be the first message
						pc.crc16Calculator.start(); // Start the crc16 calculation for the first message
						pc.fillByteCounter = null<u8>();     // At this moment we have no seen any fill byte
					}
				}
				else 
				{	
					// So, we are not looking at the first element (which would be the ESC start sequence)
					// What now could come is:
					//	- End of SML File
					//  - End of SML Message (fill byte)
					//  - A regular SML Message
					// (Or an error)
					// So, what do we have
					switch(pc.token->getType())
					{
						// End of SML file
						case Token::END_OF_SML_FILE:
							// Add an SmlFileEnd primitive to the container and set the iterator to it.
							addSet(new SmlFileEnd);
							// And instruct that we shall parse this primitive afterwards
							parseMessage = true;
							// In any case: Preset the functions return code with OK and done, since a complete SML file was read
							rc = pr_DONE;
							break;
						
						// EndOfSmlMessage
						case Token::END_OF_MESSAGE:
							// Please note: We do not prevent the reception of a regular message after a fill byte
							// Normally fill bytes should be at the end; so after the messages and before the 
							// ESC end sequence. But we do not care
							
							// Add a EndOfSmlMessage to the container and set an iterator to it
							// This is a fill byte or an empty message
							addSet(new EndOfSmlMessage);
							// Now we have one fill byte more. The total count of fill bytes will later be
							// compared to the number transmitted in the ESC End sequence
							//lint -e{911} //911 Implicit expression promotion from Type to Type
							++pc.fillByteCounter;
							break;
							
						// Error from the scanner	
						case Token::CONDITION_ERROR:
							reset(); // Reset everything and report error
							rc = pr_ERROR;
							break;
							
						// Everything else has been checked. Meaning: This must be a SML message now.	
						default:
							// So add a SmlMessage to the container. And set the iterator to it
							addSet(new SmlMessage);
							// And we want to parse the SmlMessage immediately
							parseMessage = true;
							break;
					}
				}
			}
			
			// If we need to parse a message (or other primitive, as written before)
			if (parseMessage)
			{
				// Parse the current element in the container and evaluate the result
				switch ((*smlContainerIterator)->parse(pc))
				{
					// Parsing still ongoing. Do nothing and wait
					case pr_PROCESSING:
						break;
						
					// Parsing of the current container element successfully finished
					case pr_DONE:
						// No need to parse the message any longer. We need to evaluate the next byte
						// again in order to determine what comes next (message, End etc.)
						parseMessage = false;
						break;
					
					// In case of error or unexpected result
					case pr_ERROR:  // fallthrough
					default:
						reset();		// Reset/delete everything and report error
						rc = pr_ERROR;
						break;
				}
			}
			
			return rc;
		}

// ------------------------------------------------------------------------------------------------------------------------------
// 2. General methods of parser classes
		
	// --------------------------------------------------------------------------------------------------------------------------
	// 2.1 Reset the state of the SmlFile:
		void SmlFile::reset(void)
		{
			// Delete all elements in the SmlFile container but the first. First is ESC Start
			releaseElementsButNotTheFirst();
			// We did not yet find a message
			parseMessage = false;
			// Set iterator to first element
			resetIterator();
		}

	// --------------------------------------------------------------------------------------------------------------------------
	// 2.2 Constructor for SmlFile
		SmlFile::SmlFile(void) : SmlContainerDynamic(), parseMessage(false)
		{
			// First wait for ESC Start sequence
			parseMessage = false;
			// First element is always a EscStartSequence. Add element and reset iterator
			//lint -e{1732} -e{1733}  We do not need a copy constructor or assigment operator
			//1732 new in constructor for class 'Name' which has no assignment operator
			//1733 new in constructor for class 'Name' which has no copy constructor			
			addL( new SmlFileStart);
		}

	// --------------------------------------------------------------------------------------------------------------------------
	// 2.3 Traverse the parse tree and apply the visitor
	
		// Depth first traversal of the parse tree
		// A recursive implementation with an internal iterator: This the cursor concept.
		// Containers will be visited by a visitor hierarchy. According to the visitor pattern
		// a double dispatch mechanism will be used. So the container will be call through its
		// base pointer and the visitor will be called through its base pointer. In the end
		// the correct function for the dereferenced container and the dereferenced visitor will be used.
		
		void SmlContainer::traverseAndVisit(VisitorBase *const visitor)
		{
			// Temporary pointer to Base Element of container hierarchy
			SmlContainer *smlContainer;
			// Iterator for this container (that we are traversing at this moment). Set it to the first element
			std::vector<SmlElementBase *>::iterator smlTraverseIterator = smlElementContainer.begin();
			
			// Now iterate through all elements of this container
			while (smlElementContainer.end() != smlTraverseIterator)
			{
				// If the element under evaluation is itself a container, then we will immediately
				// call the traversal function of this element. So we do a recursive, depth first traversal
				if ((*smlTraverseIterator)->isContainer())
				{
					// Get a pointer to a container Element. Normally the pointer is of type SmlElementBase
					// but I decided to visit only container. So we start with the SmlContainer in the 
					// class hierarchy
					//lint -e{929}   // Cast from pointer to pointer  // Yes
					smlContainer = dynamic_cast<SmlContainer *>(*smlTraverseIterator);
					if (null<SmlContainer *>() != smlContainer)  // This should never be false. Line even not necessary
					{
						//lint --e{613}  613 Possible use of null pointer 'Symbol' in [left/right] argument to operator 'String' Reference  // Wrong message
						// This is the visitor pattern.
						// SmlContainer is polymorph and visitor is polymorph as well.
						// So we have a double dispatch, multi method call
						smlContainer->acceptAGuestVisitor(visitor);
						// The accept function was defined using the macro DEFINE_VISITABLE
						
						
						// Recursive invocation. Before evaluating the next element in this container,
						// we go one step down
						smlContainer->traverseAndVisit(visitor);
					}
				}
				// After having traversed "deeper" elements, we will now check on the next element in this container
				++smlTraverseIterator;
			}
		}


// ------------------------------------------------------------------------------------------------------------------------------
// 3. SmlChoice Factory functions


	//lint -save -e957  // No prototype	
	SmlElementBase* createSmlPublicOpenResponse(void) { return new SmlPublicOpenResponse; }
	SmlElementBase* createSmlGetListResponse(void) { return new SmlGetListResponse; }
	SmlElementBase* createSmlPublicCloseResponse(void){ return new SmlPublicCloseResponse; }

	SmlElementBase* createSmlSecIndex(void){ return new SmlSecIndex; }
	SmlElementBase* createSmlTimestamp(void){ return new SmlTimestamp; }
	//lint -restore

	ChoiceFactorySmlMessageBody::ChoiceFactorySmlMessageBody(void) : BaseClassFactory()
	{
		choice[0x0101UL] = &createSmlPublicOpenResponse;
		choice[0x0201UL] = &createSmlPublicCloseResponse;
		choice[0x0701UL] = &createSmlGetListResponse;
	}

	SmlElementBase* ChoiceFactorySmlMessageBody::createInstance(const u32 &selector)
	{
		SmlElementBase* sebp = BaseClassFactory<u32, SmlElementBase>::createInstance(selector);
		if (null<SmlElementBase*>() == sebp) 
		{
			// If we could not create a type from the requested selector, this would normally be an error
			// We use a special handling here. We assign then a SmlMessageBody any. NOTE:
			// This will of course only work for SmlCoice container used for SmlMessages.
			// So for other types, we postpone error detection, because we will rely on
			// an error reported by other parsing routines.

			// Very special handling. See above. Only usefull, if we use the SmlMessageBodyAny concept.
			// Otherwise this should be an error
			
			sebp = new SmlMessageBodyAny;
		}
		return sebp;
	}
	
	ChoiceFactorySmlTime::ChoiceFactorySmlTime(void) : BaseClassFactory()
	{
		choice[0x01UL] = &createSmlSecIndex;
		choice[0x02UL] = &createSmlTimestamp;
	}




}  // End of namespace ParserInternal

	prCode Parser::parse(const EhzDatabyte databyte,  const uint ehzIndex)
	{

		prCode rc = pr_PROCESSING;
		
		pc.token = &(scanner.scan(databyte));
		pc.crc16Calculator.update(databyte);

		
		if (pc.token->getType() != Token::CONDITION_NOT_YET_DETECTED)
		{
	
			
			//lint -e{641,911}
			if (DebugModeParseResult == globalDebugMode)
			{
			
				switch (pc.token->getType())
				{
					case Token::CONDITION_NOT_YET_DETECTED:
						break;
					case Token::START_OF_SML_FILE:
						ui[ehzIndex] << "Start File" << std::endl;
						break;
					case Token::END_OF_SML_FILE:
						ui[ehzIndex] << "End File" << std::endl;
						break;
					case Token::END_OF_MESSAGE:
						ui[ehzIndex] << "End Message" << std::endl;
						break;
					case Token::OPTIONAL:
						//ui[ehzIndex] << "Optional" << std::endl;
						break;
					case Token::BOOLEAN:
						ui[ehzIndex] << "Bool->   " <<  (pc.token->getBoolValue()?"true":"false") << std::endl;
						break;
					case Token::SIGNED_INTEGER:
						//lint -e{921}
						ui[ehzIndex] << "Signed->  " << static_cast<s32>(pc.token->getS64Value()) << std::endl;
						break;
					case Token::UNSIGNED_INTEGER:
						//lint -e{921}
						ui[ehzIndex] << "UnSigned-> " <<  static_cast<u32>(pc.token->getU64Value()) << std::endl;
						break;
					case Token::OCTET:
						{
							uint len;
							
							SmlByteString sbs;
							std::string outString;	

							pc.token->getValueForType(&sbs);
							len = sbs.length();

							convertSmlByteStringNonePrintableCharacters(sbs,outString);
							
							ui[ehzIndex] << "Octet (" << len << "): " << outString << std::endl;
							//ui[ehzIndex] << "Octet (" << len << "): " << sbs << std::endl;
						}
						break;
					case Token::LIST:
						ui[ehzIndex] << "List->" << pc.token->getLength() << std::endl;
						break;
					case Token::CONDITION_ERROR:
						ui[ehzIndex] <<"TOKEN Error" << std::endl;
						break;
					default:
						ui[ehzIndex] << "TOKEN FATAL ERROR "  << std::endl;
						break;
				}
			}	
			pc.ignoreRestOfSequence = false;
			rc = smlFile.parse(pc);
			
			
		}

		if (pr_DONE == rc)
		{
			//ui[ehzIndex] << "Parser done: " << static_cast<void *>(this) << std::endl;
		}
		//lint -e{641,911}
		if ((DebugModeError == globalDebugMode) && (ParserResult::ERROR == rc))
		{
			std::string now;
			getNowTime(now); 	//lint !e534
			ui[ehzIndex] << "PARSER RESULT ERROR" << "\n" << now << std::endl;
			scanner.reset();
		}
		return rc;
	}


void Parser::traverseAndEvaluate(ParserInternal::VisitorForSmlListEntry * const visitorForSmlListEntry )
{
	smlFile.traverseAndVisit(visitorForSmlListEntry);
}




