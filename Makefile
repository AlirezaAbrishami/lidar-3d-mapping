DIR_ROOT  = ../

CC 				= gcc
CX 				= g++
CCFLAGS		= -Wall -c
CXFLAGS 	= -Wall -c
CP     		= cp
CP_ALL  	= cp -r
RM        = rm
RM_ALL    = rm -f
SRCS			= src/lds_driver.cpp src/locator.cpp
OBJS			= build/lds_driver.o build/locator.o
LIB_DIRS	= -L/usr/lib/arm-linux-gnueabihf/
INC_DIRS  = -I/usr/include -Isrc/include
LIBS      = -lboost_system -lpthread -lwiringPi
TARGET 	  = map

all : $(TARGET)
	$(CXX) -o $(TARGET) $(OBJS) $(INC_DIRS) $(LIB_DIRS) $(LIBS) 
 
$(TARGET) :
	$(CXX) -c $(SRCS) $(INC_DIRS) $(LIB_DIRS) $(LIBS) 
	mv lds_driver.o locator.o build/

clean:
	$(RM_ALL) $(OBJS) $(TARGET)
