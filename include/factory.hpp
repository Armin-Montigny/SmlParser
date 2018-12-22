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
// Very simple implementation of design pattern factory

// The factory stores a selector and a function pointer that has to
// create the desired class, depending on the selector
// This normally done dynamically but can also be done statically of course

// Derived functions should initialze the "choice" element through
// assigning selector values and pointers to functions that create classes 

// Templated base class for factories
//
// The createInstance member function will call the assciated function pointer 
// for the given selector
// Key and function pointer will be stored in a std::map

// There is a second factory class available. Here, the createInstance functions
// takes an additional parameter that will be passed to the constructor of thw
// class to create
//

#ifndef FACTORY_HPP
#define FACTORY_HPP


#include "mytypes.hpp"

#include <map>

 
 


// ------------------------------------------------------------------------------------------------------------------------------------------------
// 1. Simple Factory class

	// -----------------------------------------------------------------------
	// 1.1 Definition of simple Factory 


	// There is a Key, the selector and a function pointer to a function that will create a new Element dynamically
	template<class KeyClass, class BaseClass>
	class BaseClassFactory
	{
		// To make handling of function pointers more easy. 
		// This is a pointer to a function that creates the class
		// and returns a pointer to it
		typedef BaseClass* (*ElementCreator)(void);
		
		public:
			// Empty default constructor and destructor
			BaseClassFactory(void) : choice() {}
			virtual ~BaseClassFactory(void) {}
			
			// Create the required class and retrieve a pointer to it
			// The new class is created through the Element Creator function
			virtual BaseClass* createInstance(const KeyClass &selector);
			
		protected:
			// And here we store the selector and the associated Creator Class
			std::map<KeyClass, ElementCreator> choice;
	};

	// -----------------------------------------------------------------------
	// 1.2 Creator function for simple Factory
	
	
	// Get an instance of the desired class
	// This means: Create it through its creation function and return a pointer to it. Returns NULL in case of error
	template<class KeyClass, class BaseClass>
	BaseClass* BaseClassFactory<KeyClass, BaseClass>::createInstance(const KeyClass &selector) // selector will determin which class has to be created
	{
		// Initialze to 0, in case we cannot find a selector
		BaseClass* pBase = null<BaseClass*>();
		// Search for the selector
		if (!(choice.find(selector) == choice.end()))
		{
			// OK, selector could be found. 
			// Now call the stored function via its function pointer
			// This function must create the class (or reuse a created class)
			// and return a pointer to it
			pBase = (*(choice[selector]))();
		}
		return pBase;
	}


// ------------------------------------------------------------------------------------------------------------------------------------------------
// 2. Factory class with parameter for constructor for the class to create

	// --------------------------------------------------------------------------------------
	// 2.1 Definition of Factory class with parameter for constructor for the class to create


	template<class KeyClass, class BaseClass, typename ContructorParameterType>
	class FactoryWithConstructorParameter : BaseClassFactory<KeyClass, BaseClass>
	{
		public:
			// To make handling of function pointers more easy. 
			// This is a pointer to a function that creates the class
			// and returns a pointer to it
			typedef BaseClass* (*ElementCreator)(ContructorParameterType constructorParameter);		// Empty default constructor and destructor
			
			FactoryWithConstructorParameter(void) : BaseClassFactory<KeyClass, BaseClass>() {}
			virtual ~FactoryWithConstructorParameter(void) {}
			
			// Create the required class and retrieve a pointer to it
			// The new class is created thorugh the Element Creator function
			virtual BaseClass* createInstance(const KeyClass &selector, const ContructorParameterType &constructorParameter);
		protected:
			std::map<KeyClass, ElementCreator> choice;
	};

		
	// ----------------------------------------------------------------------------------------------
	// 2.2 Creator function for Factory class with parameter for constructor for the class to create
		
		
	template<class KeyClass, class BaseClass, typename ContructorParameterType>
	BaseClass* FactoryWithConstructorParameter<KeyClass, BaseClass, ContructorParameterType>::createInstance(const KeyClass &selector, const ContructorParameterType &constructorParameter) 
	{
		// Initialze to 0, in case we cannot find a selector
		BaseClass* pBase = null<BaseClass*>();
		// Search for the selector
		if (!(choice.find(selector) == choice.end()))
		{
			// OK, selector could be found. 
			// Now call the stored function via its function pointer
			// This function must create the class (or reuse a created class)
			// and return a pointer to it
			pBase = (*(choice[selector]))(constructorParameter);
		}
		return pBase;
	}



#endif
