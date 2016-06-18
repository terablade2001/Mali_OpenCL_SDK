# This confidential and proprietary software may be used only as
# authorised by a licensing agreement from ARM Limited
#   (C) COPYRIGHT 2013 ARM Limited
#       ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorised
# copies and copies may only be made to the extent permitted
# by a licensing agreement from ARM Limited.

include ../platform.mk

SOFLAGS=-fpic -shared -I../include/

LIBRARY=libOpenCL.so

all: $(LIBRARY)

clean: 
	$(RM) $(LIBRARY)

$(LIBRARY): opencl_stubs.c
	$(CC) $(SOFLAGS) -o $(LIBRARY) opencl_stubs.c