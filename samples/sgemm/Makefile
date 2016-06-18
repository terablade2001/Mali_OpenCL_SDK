# This confidential and proprietary software may be used only as
# authorised by a licensing agreement from ARM Limited
#   (C) COPYRIGHT 2013 ARM Limited
#       ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorised
# copies and copies may only be made to the extent permitted
# by a licensing agreement from ARM Limited.

ROOT:=../..

include $(ROOT)/platform.mk

CFLAGS:=-c -Wall -I$(ROOT)/include -I$(ROOT)/common -I.

LDFLAGS:=-L$(ROOT)/lib -L$(ROOT)/common -lOpenCL -lCommon

SOURCES:=sgemm.cpp
HEADERS:=$(ROOT)/common/common.h $(ROOT)/common/image.h

OBJECTS:=$(SOURCES:.cpp=.o)

EXECUTABLE:=sgemm

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) libOpenCL libCommon
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

$(OBJECTS): $(HEADERS)

install: $(EXECUTABLE)
	-$(MKDIR) "$(ROOT)/bin/$(EXECUTABLE)/assets"
	$(CP) "$(EXECUTABLE)" "$(ROOT)/bin/$(EXECUTABLE)/$(EXECUTABLE)"
	cd assets $(CONCATENATE) $(CP) * "../$(ROOT)/bin/$(EXECUTABLE)/assets/"

.PHONY: clean libOpenCL libCommon

clean:
	$(RM) $(OBJECTS) $(EXECUTABLE)

libOpenCL:
	cd $(ROOT)/lib $(CONCATENATE) $(MAKE) libOpenCL.so

libCommon:
	cd $(ROOT)/common/ $(CONCATENATE) $(MAKE) libCommon.a