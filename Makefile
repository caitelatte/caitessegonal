# Compiler Info ('g++-4.0 --version')
# i686-apple-darwin10-g++-4.0.1 (GCC) 4.0.1 (Apple Inc. build 5494)
# Copyright (C) 2005 Free Software Foundation, Inc.
# This is free software; see the source for copying conditions.  There is NO
# warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# End Compiler Info Output
NDKDIR ?= /Applications/Nuke7.0v9/Nuke7.0v9.app/Contents/MacOS
MYCXX ?= g++
LINK ?= g++
CXXFLAGS ?= -g -c -DUSE_GLEW -I$(NDKDIR)/include -isysroot ./MacOSX10.6.sdk -arch x86_64
LINKFLAGS ?= -L$(NDKDIR) -Wl,-syslibroot,./MacOSX10.6.sdk -arch x86_64
LIBS ?= -lDDImage -lGLEW
LINKFLAGS += -bundle
FRAMEWORKS ?= -framework QuartzCore -framework IOKit -framework CoreFoundation -framework Carbon -framework ApplicationServices -framework OpenGL -framework AGL 
all: Tessegonal2D.dylib
.PRECIOUS : %.os
%.os: %.cpp
	$(MYCXX) $(CXXFLAGS) -o $(@) $<
%.dylib: %.os
	$(LINK) $(LINKFLAGS) $(LIBS) $(FRAMEWORKS) -o $(@) $<
clean:
	rm -rf *.os Tessegonal2D.dylib
