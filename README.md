# SmlParser
SML Parser. Parser for the Smart Message Language used by electronic electricity meters

This software is used by me to read data from 6 electronic elektricity meters, display them, 
store the data in a SQLite Database and provide the info to others via TCP connections.

The target platform is a Raspberry 3B+. The language is C++98. Static Code analysis has been 
performed with PC Lint.

The Architecture makes heavy use of all kind of design patterns. Including the non-mainstream patterns:
- Reactor, Proactor, Asynchronous Completion Token for handle based eventhandling
- Acceptor, Connector for network communication
- Interpreter, Composite, Visitor for parsing the raw SML data stream
- Obeserver (Publisher, Subscriber) for inter class communication
- and many more patterns known from the GOF

Additionally you can find a nano HTML server and full dynmic handling of SQLITE.

The software has been devloped for my private use. It is not a universal datalogger or something.
But those of you who know about C++ programming may find good ideas for their own projects.

In case of questions or other needed support, I am happy to help.

Have fun.

