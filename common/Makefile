# This confidential and proprietary software may be used only as
# authorised by a licensing agreement from ARM Limited
#   (C) COPYRIGHT 2013 ARM Limited
#       ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorised
# copies and copies may only be made to the extent permitted
# by a licensing agreement from ARM Limited.

include ../platform.mk

CFLAGS=-c -Wall -I../include -Iinclude

LDFLAGS=

SOURCES=common.cpp image.cpp
HEADERS=common.h image.h

OBJECTS=$(SOURCES:.cpp=.o)

LIBRARY=libCommon.a

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(AR) rcs $(LDFLAGS) $(LIBRARY) $(OBJECTS)

$(OBJECTS): $(HEADERS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	$(RM) $(OBJECTS) $(LIBRARY)
