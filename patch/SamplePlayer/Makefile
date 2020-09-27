# Project Name
TARGET = ex_sampleplayer


# FatFs
FATFS_DIR = $(LIBDAISY_DIR)/Middlewares/Third_Party/FatFs/src

# Sources
CPP_SOURCES = ex_sampleplayer.cpp b3ReadWavFile.cpp

C_SOURCES = $(FATFS_DIR)/diskio.c \
	$(FATFS_DIR)/ff.c \
	$(FATFS_DIR)/ff_gen_drv.c \
	$(FATFS_DIR)/option/ccsbcs.c

# Library Locations
LIBDAISY_DIR = ../../libdaisy
DAISYSP_DIR = ../../DaisySP


# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

DAISYSP_DIR = ../../DaisySP



# FATFS dir contains fatfs itself, libdaisy sys folder contains ffconf 
C_INCLUDES += -I$(FATFS_DIR) -I$(LIBDAISY_DIR)/src/sys


#C_INCLUDES += \
#-I../../CloudSeed \

#LIBS += -lcloudseed
#LIBDIR += -L../../CloudSeed/build


