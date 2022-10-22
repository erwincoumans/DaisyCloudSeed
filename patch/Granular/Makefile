# Project Name
TARGET = ex_granular

# Sources
CPP_SOURCES = granular_synth_app.cpp granular_synth_main.cpp  ../SamplePlayer/b3ReadWavFile.cpp

# Library Locations
LIBDAISY_DIR = ../../libdaisy
DAISYSP_DIR = ../../DaisySP
CLOUDSEED_DIR = ../../CloudSeed


C_SOURCES = $(FATFS_DIR)/diskio.c \
        $(FATFS_DIR)/ff.c \
        $(FATFS_DIR)/ff_gen_drv.c \
        $(FATFS_DIR)/option/ccsbcs.c



# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile


#C_INCLUDES += \
#-I../../CloudSeed \

#LIBS += -lcloudseed
#LIBDIR += -L../../CloudSeed/build


