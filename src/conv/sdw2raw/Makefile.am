if BUILD_TOOLS

bin_PROGRAMS = sdw2raw

AM_CXXFLAGS = -I$(top_srcdir)/inc $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

sdw2raw_DEPENDENCIES = @SDW2RAW_WIN32_RESOURCE@

if STATIC_TOOLS

sdw2raw_LDADD = \
	../../lib/@STAROFFICE_OBJDIR@/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.a \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @SDW2RAW_WIN32_RESOURCE@
sdw2raw_LDFLAGS = -all-static

else	

sdw2raw_LDADD = \
	../../lib/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS)  @SDW2RAW_WIN32_RESOURCE@

endif

sdw2raw_SOURCES = \
	sdw2raw.cpp

if OS_WIN32

@SDW2RAW_WIN32_RESOURCE@ : sdw2raw.rc $(sdw2raw_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource sdw2raw.rc @SDW2RAW_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(sdw2raw_SOURCES)	\
	sdw2raw.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	sdw2raw.rc	 

endif
