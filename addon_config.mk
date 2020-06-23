meta:
    ADDON_NAME = ofxVimba
    ADDON_DESCRIPTION = Interface for Allied Vision cameras using Vimba API
    ADDON_AUTHOR = Jean Jacques Warmerdam & Matthias Oostrik
    ADDON_TAGS = "camera" "gigE" "Allied Vision"
    ADDON_URL = http://github.com/moostrik/ofxVimba

common:
    ADDON_INCLUDES =
    ADDON_INCLUDES += src
    ADDON_INCLUDES += libs/Vimba/include
    ADDON_DEPENDENCIES = ofxXmlSettings
    ADDON_CPPFLAGS = -D_x64

linux64:
    ADDON_LIBS =
    ADDON_LIBS += libs/Vimba/lib/linux64/libVimbaC.so
    ADDON_LIBS += libs/Vimba/lib/linux64/libVimbaCPP.so
    ADDON_CPPFLAGS += -D_LINUX
