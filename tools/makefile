ROOT_DIR = /home/pidata/SmlParser

SOURCE_DIR = $(ROOT_DIR)/source
INCLUDE_DIR = $(ROOT_DIR)/include
OBJECT_DIR = $(ROOT_DIR)/objects
TOOL_DIR = $(ROOT_DIR)/tools
SQLITE_LIB = /usr/local/lib
BACKUP_DIR = $(ROOT_DIR)/backup

DPENDENCIES_DIR = $(TOOL_DIR)/dependencies

MAIN_TARGET = $(ROOT_DIR)/ehz

#  -H forInclude file output
# make -B > txt.tx 2>&1

COMPILER_FLAGS = -I$(INCLUDE_DIR)  -g -fverbose-asm -Wall -Wextra -pedantic -Wno-long-long -DROOT_DIRECTORY=\"$(ROOT_DIR)\"  -c 

CC = g++


MAIN_INCLUDES = $(INCLUDE_DIR)/mytypes.hpp  $(TOOL_DIR)/makefile $(TOOL_DIR)/includedependencies.txt

OBJECTFILES = \
$(OBJECT_DIR)/main.o \
$(OBJECT_DIR)/parsetreevisitor.o \
$(OBJECT_DIR)/database.o \
$(OBJECT_DIR)/ehz.o \
$(OBJECT_DIR)/acceptorconnector.o \
$(OBJECT_DIR)/tcpconnection.o \
$(OBJECT_DIR)/ehzmeasureddata.o \
$(OBJECT_DIR)/crc16.o \
$(OBJECT_DIR)/parser.o \
$(OBJECT_DIR)/scanner.o \
$(OBJECT_DIR)/eventhandler.o \
$(OBJECT_DIR)/server.o \
$(OBJECT_DIR)/transfer.o \
$(OBJECT_DIR)/bytestring.o \
$(OBJECT_DIR)/reactor.o \
$(OBJECT_DIR)/proactor.o \
$(OBJECT_DIR)/escanalysis.o \
$(OBJECT_DIR)/userinterface.o \
$(OBJECT_DIR)/serial.o\
$(OBJECT_DIR)/typelengthfield.o


#/usr/local/lib/libsqlite3.a

ehz : $(MAIN_TARGET)
	-@mkdir -p $(BACKUP_DIR)
	-@cd $(INCLUDE_DIR);mv ./*.bak $(BACKUP_DIR)  2>/dev/null   ||:
	-@cd $(SOURCE_DIR);mv ./*.bak $(BACKUP_DIR)   2>/dev/null   ||:
	-@cd $(TOOL_DIR);mv ./*.bak $(BACKUP_DIR)   2>/dev/null   ||:
	@echo Done



$(MAIN_TARGET): $(OBJECTFILES)
	@echo Linking $@
	@$(CC)  $(OBJECTFILES)  -Wl,-rpath=/usr/local/gcc-6.1.0/lib -L$(SQLITE_LIB) -lpthread -lsqlite3 -lncurses -lrt -o$@ 


#  C Source Files
#
#objdump -d -S -l  $@ > $@.odd
#
#
include $(TOOL_DIR)/includedependencies.txt
#	
	
	
