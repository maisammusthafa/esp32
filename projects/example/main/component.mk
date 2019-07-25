#
# Main Makefile. This is basically the same as a component makefile.
#
ARDUINO_ULIBRARIES_LIST := $(patsubst $(COMPONENT_PATH)/libraries/%,%,$(wildcard $(COMPONENT_PATH)/libraries/*))

ARDUINO_SINGLE_ULIBRARY_FILES = $(patsubst $(COMPONENT_PATH)/%,%,$(sort $(dir $(wildcard $(COMPONENT_PATH)/libraries/$(MODULE)/*)) $(dir $(wildcard $(COMPONENT_PATH)/libraries/$(MODULE)/src/*/)) $(dir $(wildcard $(COMPONENT_PATH)/libraries/$(MODULE)/src/*/*/)) $(dir $(wildcard $(COMPONENT_PATH)/libraries/$(MODULE)/src/*/*/*/)) $(dir $(wildcard $(COMPONENT_PATH)/libraries/$(MODULE)/src/*/*/*/*/))))

ARDUINO_USER_LIBS := $(foreach MODULE,$(ARDUINO_ULIBRARIES_LIST), $(ARDUINO_SINGLE_ULIBRARY_FILES))

COMPONENT_ADD_INCLUDEDIRS := $(ARDUINO_USER_LIBS) main .
COMPONENT_SRCDIRS := $(ARDUINO_USER_LIBS) main .
