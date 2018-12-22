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

// Userinterface on Terminal using NCurses
//
// This Application uses a terminal window to show some status or debug information for each Device
// and has a general output screen.
// So overall 6+6+1 windows will be created using ncurses
//
// Everything will be wrapped in standard c++ ostreams.
// The implementation is very basic and has only few output functions, but can be extended easily
//
// We will use a streambuf with direct unbuffered output.
// Aditionally some basic manipulators are defined.
//
// Note: We will not call wrefresh everytime. "endl"  or "flush" must be used for refreshing the screen.
//
// Reference books:
// - "Programmer's Guide to nCurses" by  Dan Gookin
// - "The C++ Standard Library, Second Edition" by Nicolai M. Josuttis
// - "Standard C++ IO Streams and Locales : Advanced Programmers Guide and Reference" by Angelika Langer
//

#ifndef USERINTERFACE_HPP
#define USERINTERFACE_HPP

#include "mytypes.hpp"
#include "curses.h"
#include "eventhandler.hpp"

#include <vector>
#include <ostream>

// ----------------------------------------------------------------------------------------------------------------------------------------------
// 
	namespace UserinterfaceInternal
	{
		// Make more coenvient name for ncurses window pointer
		typedef WINDOW *NcursesWindow;

// ----------------------------------------------------------------------------------------------------------------------------------------------
// 2. Output Stream (ostream) for ncurses window + output streambuffer
		

		//lint -e{1790}
		// This is the stream class for output to your terminal
		class NCursesWindowStream : public std::ostream, private std::streambuf
		{
			public:
				// Explicit constructor
				// Needs a WINDOW *, provided by ncurses througn the newwin function
				// and stores the WINDOW* from the given parameter in an internal buffer
				explicit NCursesWindowStream(const NcursesWindow windowP) : std::ostream(this), thisWindow(windowP) {}
				
				//lint -e{1540} Pointer member neither freed nor zeroed by destructor -- Effective C++ #6
				// Destructor does nothing. Pointer will be deleted by ncurses endwin fonction
				virtual ~NCursesWindowStream(void) { }
				
				// UserInterface specific functions. All realized via nCurses. Can be called directly or via manipulator
				//lint -e{534}
				// Set the cursor in this window
				virtual void setCursorPosition(const sint row, const sint col) { wmove(thisWindow,row,col);}
				
				//lint -e{534}
				// clear the screen for this window
				virtual void clearScreen(void) { wclear(thisWindow); }
				
				NcursesWindow getWindow(void)  { return thisWindow; }  //lint !e1962
			protected:

				// And a local copy of the WINDOW *
				NcursesWindow thisWindow;
			
				// Write one character to the screen. Called by sputc
				//lint -e{921,952,1961,1960} 
				virtual std::streambuf::int_type overflow (std::streambuf::int_type c) { return waddch(thisWindow, static_cast<chtype>(c));}
				
				// Write multiple characters to the screen. Called by sputn
				//lint -e{534,952,1961,1960} 
				virtual std::streamsize xsputn(const mchar* s, std::streamsize num) {waddnstr(thisWindow, s, num);return num;} 
				
				// Refresh the screen (Called by std::endl or std::flush)
				//lint -e{1961}
				virtual sint sync(void) { return wrefresh(thisWindow);  }
		};
	} // End of namespace


// ----------------------------------------------------------------------------------------------------------------------------------------------
// 3. Output Stream (ostream) for Userinterface

	// Adding additional functionality and windows
	// What we want to do:
	//
	// We want to output data to the main window via:  ui << "Hello" << std::endl;
	//// and we want to address the 2 subwindow array via operator [] and ()  
	// so:   ui[i] << "Hello" << std::endl;   and    ui(i) << "Hello" << std::endl;
	// Yes, I know, I misuse the () operator
	// All other functionality will be derived from the base class
	class NCursesUserinterface : public UserinterfaceInternal::NCursesWindowStream
	{
		public:
			// Start up all nCurses and create all windows
			NCursesUserinterface(void);
			// Delete sub window ostreams
			virtual ~NCursesUserinterface(void);
			
			virtual void resizeWindows(void);
			virtual void reInitialize(void){ this->deInitialize(); this->initialize(); }
			
			// Retrieve ostream of sub windows
			UserinterfaceInternal::NCursesWindowStream &operator [] (const uint windowNumber);
			UserinterfaceInternal::NCursesWindowStream &operator () (const uint windowNumber);
		protected:
		
		
			virtual void setWindowHeaders(void) const;
			virtual void initialize(void);
			virtual void deInitialize(void);
			
		// Vector for ostreams for subwindows
			std::vector<UserinterfaceInternal::NCursesWindowStream *> debugWindowsStream;
			std::vector<UserinterfaceInternal::NCursesWindowStream *> resultWindowsStream;

			EventHandlerSIGWINCH eventHandlerSIGWINCH;
	};


// ----------------------------------------------------------------------------------------------------------------------------------------------
// 4. Manipulators for ostream for Userinterface

	// ----------------------------------
	// 4.1 Manipulator for Clear Screen

	//lint -e{1929}
	inline std::ostream &cls(std::ostream &os)  
	{ 
		//lint -e{1920,929,1912,909}
		if (UserinterfaceInternal::NCursesWindowStream &ws = dynamic_cast<UserinterfaceInternal::NCursesWindowStream &>(os))
		{
			ws.clearScreen();
		}
		return os; 
	} 

	// ------------------------------------------
	// 4.1  Manipulator for Set Cursor Position
	class SetPos
	{
		public:
			explicit SetPos(const sint rowP, const sint colP) : row(rowP), col(colP) {}
			SetPos(void) : row(null<sint>()), col(null<sint>()) {}
			friend std::ostream &operator <<(std::ostream &os, const SetPos &setPos) 
			{ 
				//lint -e{1920,929,1912,909}
				if (UserinterfaceInternal::NCursesWindowStream &ws = dynamic_cast<UserinterfaceInternal::NCursesWindowStream &>(os))
				{
					ws.setCursorPosition(setPos.row, setPos.col);
				}
				return os; 
			}
		protected:
			sint row;
			sint col;	
	};



// ----------------------------------------------------------------------------------------------------------------------------------------------
// 5. Global reference for UI.

extern NCursesUserinterface ui;
inline void waitForKeyPress(void) {(void)getch();}

#define SHOWINFO   ui << " --> "<< __FILE__ << " / " << __FUNCTION__ << " / " << __LINE__ << std::endl; //lint !e773


#endif
