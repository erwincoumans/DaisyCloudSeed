# Project Name
TARGET = daisy_bouquet

# FatFs
FATFS_DIR = $(LIBDAISY_DIR)/Middlewares/Third_Party/FatFs/src

# Sources
CPP_SOURCES = daisy_bouquet_main.cpp dummy_app.cpp Granular/granular_synth_app.cpp SamplePlayer/sample_player_app.cpp SamplePlayer/b3ReadWavFile.cpp 


C_SOURCES = $(FATFS_DIR)/diskio.c \
	$(FATFS_DIR)/ff.c \
	$(FATFS_DIR)/ff_gen_drv.c \
	$(FATFS_DIR)/option/ccsbcs.c
	
# Library Locations
LIBDAISY_DIR = ../libdaisy
DAISYSP_DIR = ../DaisySP
CLOUDSEED_DIR = ../CloudSeed

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# FATFS dir contains fatfs itself, libdaisy sys folder contains ffconf 
C_INCLUDES += -I$(FATFS_DIR) -I$(LIBDAISY_DIR)/src/sys



