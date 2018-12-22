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
// Definition of Design Pattern Singleton
//
// Please check the internet for an explanation of this Design pattern
//
// We are using a Singleton with lazy (late) initialisation

// There are 2 variants:

// Dynamic singleton with lazy initialisation and
// Static singleton with lazy initialisation

// We will use the static variant here


#ifndef SINGLETON_HPP
#define SINGLETON_HPP

 
 



// --------------------------------------------------------------------------------------------------------------------------------
// 1. Static Singelton

	// Using static thread safe singleton pattern with lazy initialisation

	// Please note. This is none portable code. GCC4.6 guarantees thread safe initialization of static locals
	// It has been checked and validated to work with gcc 4.6 on a raspberry Pi B+ and other raspberries

	#define DEFINE_STATIC_SINGLETON_FUNCTION(instance)  \
	template <typename SingletonState>  \
	static SingletonState *instance(void)  \
	{  \
		static SingletonState singletonState;  \
		return &singletonState;	\
	}

	#define SINGLETON_FOR_CLASS(classname) \
	static classname *getInstance(void)  \
	{  \
		static classname singleton;  \
		return &singleton;	\
	}

 

	
#endif
