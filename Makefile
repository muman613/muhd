###############################################################################
#	MODULE		:	Makefile
#	PROJECT		:	Hex dump utility
#	AUTHOR 		:	Michael A. Uman
#	DATE 		:	April 8, 2013
###############################################################################

TARGET_EXE=muhd
CPP_SOURCES=main.cpp
TARGET_TYPE=exe

GCC=c++
LIBS+=-lpopt

include build/buildsys.mk


# DO NOT DELETE
