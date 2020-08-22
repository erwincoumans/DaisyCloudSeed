# Project Name
TARGET = ex_cloudseed

# Sources
CPP_SOURCES = ex_cloudseed.cpp

# Library Locations
LIBDAISY_DIR = ../../libdaisy
DAISYSP_DIR = ../../DaisySP
CLOUDSEED_DIR = ../../CloudSeed

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile


C_INCLUDES += \
-I../../CloudSeed \

LIBS += -lcloudseed
LIBDIR += -L../../CloudSeed/build


