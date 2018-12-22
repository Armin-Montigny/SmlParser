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
// Note: We will not call wrefresh everytime. "endl" (preffered) or "flush" must be uses for the 
// refreshing the screen.
//
// To get a better understanding I would recomend to read to books:
// - "Programmer's Guid to nCurses" by  Dan Gookin
// - "The C++ Standard Library, Second Edition" by Nicolai M. Josuttis
// - "Standard C++ IO Streams and Locales : Advanced Programmers Guide and Reference" by Angelika Langer

#include "userinterface.hpp"
#include "ehzconfig.hpp"


// ----------------------------------------------------------------------------------------------------------------------------------------------
// 1. Helper functions


	namespace UserinterfaceInternal
	{
		extern const uint NumberOfSubWindows = EhzInternal::MyNumberOfEhz;


		
		// -----------------------------------------------------------------------------------------------
		// 1.1 POD for storing all window dimensions
	
		// This is a my program specific variable which gives the number of debug screens
		// and the number of status screens


		// Temporary storage for the dimensions of the window that we ant to create later
		struct WindowDimensions
		{
			public:
				// Initialize all values
				WindowDimensions(void);
				// Calculate the windows dimensions according to the needs of this special application
				void initialize(void); // Screent Part in Percent
				
				// Console
				// Dimension of console output window
				sint OutputConsoleWidth;	// Columns
				sint OutputConsoleHeight;	// Rows

				// Status Window. Is Initilized as window.
				sint LogWindowHeight;		// Rows
				sint LogWindowWidth;  		// Columns
				sint LogWindowX;			// Position, row
				sint LogWindowY;			// NCurses startes with 0,0 in the upper left edge of the screen

				//  Result Window
				sint ResultWindowHeight;		// Rows
				sint ResultWindowWidth;			// Columns
				sint ResultWindowY;				// Position, row
				sint ResultWindowX;				// NCurses startes with 0,0 in the upper left edge of the screen

				//  Debug Window
				sint DebugWindowHeight;			// Rows
				sint DebugWindowWidth;			// Columns
				sint DebugWindowY;				// Position, row
				sint DebugWindowX;				// NCurses startes with 0,0 in the upper left edge of the screen
		};


		// -----------------------------------------------------------------------------------------------
		// 1.2 Constructor for Window Dimensions

		// Constructor for the window dimensions
		// First set everything to 0 and if ncurses has been started
		// via initscreen, the values are calculated
		WindowDimensions::WindowDimensions(void) : 	OutputConsoleWidth(0),
													OutputConsoleHeight(0),
													LogWindowHeight(0),
													LogWindowWidth(0),
													LogWindowX(0),
													LogWindowY(0),
													ResultWindowHeight(0),
													ResultWindowWidth(0),
													ResultWindowY(0),
													ResultWindowX(0),
													DebugWindowHeight(0),
													DebugWindowWidth(0),
													DebugWindowY(0),
													DebugWindowX(0)
		{
			// Calculate all window coordinates and initialze variables
			initialize();

		}


		// -----------------------------------------------------------------------------------------------
		// 1.3 Calculate Window Dimensions


		// Calculate the windo dimensions
		// This is compeltely application specific
		// In our case we want to have 6 debug windows at the top, side by side,
		// below that, 6 result windows side by side
		// and at the bottome on window for general status output for full screen width
		
		
		// Tons of magic numbers;
		void WindowDimensions::initialize(void)
		{
			mdouble logWindowScreenPart = 0.65;
			switch (globalDebugMode)
			{

				case DebugModeHeaderOnly:
					logWindowScreenPart = 0.85;
					break;
				case DebugModeError:
					logWindowScreenPart = 0.65;
					break;
				case DebugModeObis:
					logWindowScreenPart = 0.4;
					break;
				case DebugModeParseResult:
					logWindowScreenPart = 0.1;
					break;
				default:
					logWindowScreenPart = 0.65;
					break;
			}
			//lint --e{909,911,1963,9050,909}
			// Console
			
			// Get Size of standard screen
			getmaxyx(stdscr,OutputConsoleHeight,OutputConsoleWidth);

			// ----------------------------------------
			// Status Window. At the bottom. Full width
			//lint -e{912,922}
			// 0,65 is a migic number. We want to have 65% of the screen for the status window
			// That value will determin the hight of the rest
			LogWindowHeight = static_cast<sint>(logWindowScreenPart*OutputConsoleHeight);		// Rows
			LogWindowWidth = OutputConsoleWidth;  												// Columns
			LogWindowY = OutputConsoleHeight-LogWindowHeight;									// Position Row
			LogWindowX = 0;																		// Position Column

			// -----------------------------------------------------
			//  Result Window. In the middle. 6 windows side by side
			//lint -e{921}
			// Height is 2 rows more than the number of results that we want to show
			// So we have a fixed hight. Position will be chosen on top of the status window
			ResultWindowHeight = static_cast<sint>(NumberOfEhzMeasuredData)+2;
			//lint -e{573,921,737,912}
			ResultWindowWidth = static_cast<sint>(OutputConsoleWidth / NumberOfSubWindows);
			ResultWindowY = LogWindowY - ResultWindowHeight;
			ResultWindowX = 0;

			// -----------------------------------------------
			// Debug Window at the top. 6 windows side by side
			// The height is the rest of the availbale space
			// Position is on top of screen
			DebugWindowHeight = (OutputConsoleHeight - ResultWindowHeight) - LogWindowHeight;
			//lint -e{573,921,737,912}
			DebugWindowWidth = static_cast<sint>(OutputConsoleWidth / NumberOfSubWindows);
			DebugWindowY = ResultWindowY - DebugWindowHeight;
			DebugWindowX = 0;
		}

	}

	
	
	

// ----------------------------------------------------------------------------------------------------------------------------------------------
// 2. NCursesUserinterface

	// -----------------------------------------------------------------------------------------------
	// 2.1 Constructor for Userinterface
	// Startup ncurses
	// Create all windows
	
	void NCursesUserinterface::initialize(void)
	{
		// ncurses is a macrco catastrophy. Lint vomits tons of messages. We will ignore them
		//lint --e{534,917,1786,921,835,912,737,713,1732,1733,423,429}
		
		// ----------------------------------------------
		// Initialize all ncurses functionality. Start up
		initscr();		
			
		noecho();		// Dont show data entered by user
		start_color();	// We want to use colors
		curs_set(0);	// Make the cursor invisible
		
		// Define colors schemes for our windows
		init_pair(1,COLOR_BLACK,COLOR_CYAN);
		init_pair(2,COLOR_WHITE,COLOR_BLUE);
		init_pair(3,COLOR_BLACK,COLOR_GREEN);
		

		UserinterfaceInternal::WindowDimensions windowDimensions;

		// Update buffers of stdscr
		refresh();	// Calculate Window dimensions

		// ---------------------------------
		// Create the new main status window
		thisWindow = newwin(windowDimensions.LogWindowHeight,windowDimensions.LogWindowWidth,windowDimensions.LogWindowY,windowDimensions.LogWindowX);
		scrollok(thisWindow,TRUE);			// We want automatic scolling
		wbkgd(thisWindow,COLOR_PAIR(3));	// Set the color
		wmove(thisWindow,0,0);				// Set the cursoe position

		
		wrefresh(thisWindow);				// Show window on terminal

		
		// ----------------------------------------------
		// Create 2*6 windows for debug output and result
		for (uint i = 0U; i< UserinterfaceInternal::NumberOfSubWindows; i++)
		{
			// ----------------------------------
			// Create result windows in this loop
			UserinterfaceInternal::NcursesWindow windowTemp = newwin(	windowDimensions.ResultWindowHeight,
																		windowDimensions.ResultWindowWidth,
																		windowDimensions.ResultWindowY,
																		// Set new windows side by side
																		(i*windowDimensions.ResultWindowWidth) + windowDimensions.ResultWindowX);
			scrollok(windowTemp,TRUE);				// We want automatic scolling
			wbkgd(windowTemp,COLOR_PAIR((i%2)+1));	// Set the color. Different color for each 2nd  windows
			wrefresh(windowTemp);					// Show the window on the terminal
			
			// Create a new ostream for that window
			UserinterfaceInternal::NCursesWindowStream *tempWindowStream = new UserinterfaceInternal::NCursesWindowStream(windowTemp);
			// And store verything in the associated vector
			resultWindowsStream.push_back(tempWindowStream);
			
			// ---------------------------------
			// Create debug windows in this loop
			windowTemp = newwin(	windowDimensions.DebugWindowHeight,
									windowDimensions.DebugWindowWidth,
									windowDimensions.DebugWindowY,
									// Set new windows side by side
									(i*windowDimensions.DebugWindowWidth) + windowDimensions.DebugWindowX);

									scrollok(windowTemp,TRUE);					// We want automatic scolling
			wbkgd(windowTemp,COLOR_PAIR(2-(i%2)));		// Set the color. Different color for each 2nd  windows
			wrefresh(windowTemp);						// Show the window on the terminal	
			
			// Create a new ostream for that window
			tempWindowStream = new UserinterfaceInternal::NCursesWindowStream(windowTemp);
			// And store verything in the associated vector
			debugWindowsStream.push_back(tempWindowStream);
		}
		this->setWindowHeaders();
	}
	
	
	NCursesUserinterface::NCursesUserinterface(void) : NCursesWindowStream(thisWindow), debugWindowsStream(), resultWindowsStream(), eventHandlerSIGWINCH()
	{
		this->initialize();
	}




	// -----------------------------------------------------------------------------------------------
	// 2.2 Destructor
	// Delete the ostreams that are stored in a vector
	
	void NCursesUserinterface::deInitialize(void)
	{
		// Iterate through ostreams for result windows
		for (std::vector<UserinterfaceInternal::NCursesWindowStream *>::iterator it = resultWindowsStream.begin(); it != resultWindowsStream.end(); ++it) 
		{
			// Delete the ostream and delete the window
			delete (*it);
		}
		resultWindowsStream.clear();
		// Iterate through ostreams for debug windows
		for (std::vector<UserinterfaceInternal::NCursesWindowStream *>::iterator it = debugWindowsStream.begin(); it != debugWindowsStream.end(); ++it) 
		{
			// Delete the ostream and delte the winodw
			delete (*it);
		}
		debugWindowsStream.clear();
		{
			//lint --e{534}
			delwin(thisWindow);
			endwin();	
			refresh();
			endwin();
		}
	}
	
	NCursesUserinterface::~NCursesUserinterface(void)
	{
		try
		{
			this->deInitialize();
		}
		catch(...)
		{
			// Constructor of global variable, we dont care at this point in time
		}
	}


	// -----------------------------------------------------------------------------------------------
	// 2.3 Getters for ostream contained in User Interface

	
	// --------------------------------------------------
	// 2.3.1 Retrieve ostream for debugWindow from vector

	// With boundary check
	UserinterfaceInternal::NCursesWindowStream &NCursesUserinterface::operator [] (const uint windowNumber)
	{
		// Temporary function return varaible
		UserinterfaceInternal::NCursesWindowStream *nCursesWindowStream;
		// Boundary check
		if (windowNumber < UserinterfaceInternal::NumberOfSubWindows)
		{
			// If in bounds, then return stream for debug window
			nCursesWindowStream = debugWindowsStream[windowNumber];
		}
		else 
		{
			// If out of bounds, return the main status window
			nCursesWindowStream = this; 
		}
		return *nCursesWindowStream;
	}

	// ---------------------------------------------------
	// 2.3.4 Retrieve ostream for resultWindow from vector

	// With boundary check
	UserinterfaceInternal::NCursesWindowStream &NCursesUserinterface::operator () (const uint windowNumber)
	{
		// Temporary function return varaible
		UserinterfaceInternal::NCursesWindowStream *nCursesWindowStream;
		// Boundary check
		if (windowNumber < UserinterfaceInternal::NumberOfSubWindows)
		{
			// If in bounds, then return stream for result window
			nCursesWindowStream = resultWindowsStream[windowNumber];
		}
		else 
		{
			// If out of bounds, return the main status window
			nCursesWindowStream = this; 
		}
		return *nCursesWindowStream;
	}


	void NCursesUserinterface::resizeWindows(void)
	{
		//lint --e{534}
		UserinterfaceInternal::WindowDimensions windowDimensions;
		windowDimensions.initialize();


		wresize(thisWindow, windowDimensions.LogWindowHeight,windowDimensions.LogWindowWidth);
		mvwin(thisWindow,windowDimensions.LogWindowY,windowDimensions.LogWindowX);
		wrefresh(thisWindow);

		for (uint i = 0U; i< UserinterfaceInternal::NumberOfSubWindows; i++)
		{
			//lint --e{912,737,713,917,534}
			wresize(resultWindowsStream[i]->getWindow(),windowDimensions.ResultWindowHeight, windowDimensions.ResultWindowWidth);
			mvwin(resultWindowsStream[i]->getWindow(), windowDimensions.ResultWindowY,(i*windowDimensions.ResultWindowWidth) + windowDimensions.ResultWindowX);
			wrefresh(resultWindowsStream[i]->getWindow());

			debugWindowsStream[i]->setCursorPosition(0,0);
			wresize(debugWindowsStream[i]->getWindow(),windowDimensions.DebugWindowHeight, windowDimensions.DebugWindowWidth);
			mvwin(debugWindowsStream[i]->getWindow(), windowDimensions.DebugWindowY,(i*windowDimensions.DebugWindowWidth) + windowDimensions.DebugWindowX);
			wrefresh(debugWindowsStream[i]->getWindow());
		}
		this->setWindowHeaders();
		refresh();
	}

	void NCursesUserinterface::setWindowHeaders(void) const
	{
		for (uint i = 0U; i< UserinterfaceInternal::NumberOfSubWindows; i++)
		{
			//lint -e{1963,9050,1901}
			ui[i] << cls << SetPos(0,0) << EhzInternal::myEhzConfigDefinition[i].EhzName << std::endl;
		}
	}
