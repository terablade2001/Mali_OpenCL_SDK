# This confidential and proprietary software may be used only as
# authorised by a licensing agreement from ARM Limited
#   (C) COPYRIGHT 2013 ARM Limited
#       ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorised
# copies and copies may only be made to the extent permitted
# by a licensing agreement from ARM Limited.

CC:=arm-none-linux-gnueabi-g++
AR=arm-none-linux-gnueabi-ar

# Test to see if the platform is Windows (it's Windows if the shell has the .exe extention).
ifeq (.exe, $(suffix $(SHELL)))
	RM:=del /f
	CONCATENATE:=&
	MKDIR:=mkdir
	CP:=copy
else
	RM:=rm -f
	CONCATENATE:=;
	MKDIR:=mkdir -p
	CP:=cp
endif	