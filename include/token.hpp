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
// As in classical Compiler Design we will use the term Token here.

// The lexical analyzer for the SML file will read the text and split into Tokens. 
// The Token has a Type, an attribute/value) and a length (for the attribute/value).
// The tokens will be handed over to the Parser. The Parser will check the sequence of tokens
// against the defined grammar (SML). The Parser will also build a parse tree with all the
// tokens in it. After the tree is finalized, it can be traversed and all required data can
// be retrieved from the stored token data.

// This module defines only 3 data members) Type, Value, Length and a lot of functions to access 
// that data in various ways.

// SML uses a Type-Length Field. Coded in one byte. One nibble contains the type and the other the length
// We will use a similar approach for the token here 

#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "escanalysis.hpp"



 
 


// ------------------------------------------------------------------------------------------------------------------------------------------------
// 1. The token class

	class Token
	{
		public: 
			// Definition of all Token Types
			enum TokenType
			{
				START_OF_SML_FILE,				// Start of SML File (an ESC sequence) has been found
				END_OF_SML_FILE,				// End of SML File (an ESC sequence) has been found
				END_OF_MESSAGE,					// End of a SML Message in a SML file detected
				OPTIONAL,						// The grammar allows for optional values. We found something
				BOOLEAN,						// A boolean value has been read
				SIGNED_INTEGER,					// Signed integer
				UNSIGNED_INTEGER,				// Unsigned integer
				OCTET,							// Octed or Multibyte Octed found
				LIST,							// A list, a compound statement has been detected
												// Special Tokens for procedural behaviour
				CONDITION_NOT_YET_DETECTED,		// Scan ongoing. Token not yet produced
				CONDITION_ERROR					// Some error happened
			};

			// Set Type of Token
			void setTlType(TokenType t)  { tokenType = t; }
			// Set length of token (Depends a little bit on token type)
			void setTlLength(TokenLength l)  { tokenLength = l; }
			// Set both Token Type and Token Length
			void setTokenTypeAndLength(TokenType t, TokenLength l) { tokenType = t; tokenLength = l;}

			// Get the Type of the token
			TokenType getType(void) const { return tokenType; }
			// Get teh length of the token
			TokenLength getLength(void) const { return tokenLength; }

			// This are functions to set associated values beside type and length
			// Add/Append a value (char) to the SMLByteString
			void setValue(EhzDatabyte ehzDatabyte);
			// setValue with now parameter will clear all values. Reset to 0
			void setValue(void) { tokenValue.smlByteString.clear(); tokenValue.boolValue = false; tokenValue.s64Value = 0L; tokenValue.u64Value = 0ULL;}
			// Store the boolean value
			void setValue(boolean b) { tokenValue.boolValue = b; }
			// Any kind of signed integer. Length field determines which
			void setValue(s64 s) { tokenValue.s64Value = s; }
			// Any kind of unsigned integer. Length field determines which
			void setValue(u64 u) { tokenValue.u64Value = u; }
			// Set the value for the SML file end data. Checksum and Pad bytes
			void setValue(const EscSmlFileEndData &fed) { tokenValue.escSmlFileEndData = fed; }

			// Getter functions for all data types
			boolean getBoolValue(void) const { return tokenValue.boolValue;} // boolean value
			s64 getS64Value (void) const { return tokenValue.s64Value;} // signed integer
			u64 getU64Value (void) const { return tokenValue.u64Value;} // unsigned integer
			mdouble getDoubleValue(void) const; // Signed or unsigned value converted to double
			// And, the SML FIle End Data
			void getEscSmlFileEndData(EscSmlFileEndData &fed) const { fed = tokenValue.escSmlFileEndData; }
			
			// Static polymorphism. Get the value. Deduce the data type from the parameter
			void getValueForType(u8 *data) const { *data =  static_cast<u8>(tokenValue.u64Value) ;}
			void getValueForType(u16 *data) const { *data =  static_cast<u16>(tokenValue.u64Value) ;}
			void getValueForType(u32 *data) const { *data =  static_cast<u32>(tokenValue.u64Value) ;}
			void getValueForType(u64 *data) const { *data =  tokenValue.u64Value ;}
			void getValueForType(s8 *data) const { *data =  static_cast<s8>(tokenValue.s64Value) ;}
			void getValueForType(s16 *data) const { *data =  static_cast<s16>(tokenValue.s64Value) ;}
			void getValueForType(s32 *data) const { *data =  static_cast<s32>(tokenValue.s64Value) ;}
			void getValueForType(s64 *data) const { *data =  tokenValue.s64Value ;}
			void getValueForType(boolean *data) const { *data =  tokenValue.boolValue ;}
			void getValueForType(SmlByteString *sbs) const;
			void getValueForType(EscSmlFileEndData *data) const { *data = tokenValue.escSmlFileEndData; }
			void getValueForType(SmlListLength *data) const { data->lenght = static_cast<u8>(tokenLength); }
			
		protected:
			// Associated Values
			// Type
			TokenType tokenType;
			// Length
			TokenLength tokenLength;
			// Value
			// Definition
			struct TokenValue 
			{
				boolean boolValue;
				s64 s64Value;
				u64 u64Value;
				EscSmlFileEndData escSmlFileEndData;
				SmlByteString smlByteString;
			};
			// Instantiation
			TokenValue tokenValue;
	};

	
	
// ------------------------------------------------------------------------------------------------------------------------------------------------
// 2. Inline functions


	// -----------------------------------------------------------------------
	// 2.1 Get a double value from the token

	// The token stores only integer types.
	// This will convert, depending on the sign, an integer to a double
	inline mdouble Token::getDoubleValue(void) const
	{
		// Initialize result value
		mdouble rc = 0.0;
		// Depending on the sign. Oi unsigned
		if (UNSIGNED_INTEGER == tokenType)
		{
			// convert from unsigned value
			rc = mdouble(tokenValue.u64Value);
		}
		else
		{
			// if signed, then convert from signed value
			rc = mdouble(tokenValue.s64Value);
		}
		// And return the converted double
		return rc;
	}

	// -----------------------------------------------------------------------
	// 2.2 Append a byte to the SmlByteString which is a std::string
	inline void Token::setValue(EhzDatabyte ehzDatabyte)
	{
		// SmlByte string is limited in length
		if (static_cast<TokenLength>(MAX_SML_STRING_LEN - 2) > tokenValue.smlByteString.length() )
		{
			// Append a byte to the string
			tokenValue.smlByteString += static_cast<mchar>(ehzDatabyte);  
		}
	}

	// -----------------------------------------------------------------------
	// 2.3 Get string from token
	inline void Token::getValueForType(SmlByteString *sbs) const
	{  
		// Copy the SmlByteSTring which is a std::string
		*sbs = tokenValue.smlByteString;
	}

 	
	
#endif
