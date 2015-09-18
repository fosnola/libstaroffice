if BUILD_TOOLS

bin_PROGRAMS = sd2raw

AM_CXXFLAGS = -I$(top_srcdir)/inc $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

sd2raw_DEPENDENCIES = @SD2RAW_WIN32_RESOURCE@

if STATIC_TOOLS

sd2raw_LDADD = \
	../../lib/@STAROFFICE_OBJDIR@/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.a \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @SD2RAW_WIN32_RESOURCE@
sd2raw_LDFLAGS = -all-static

else	

sd2raw_LDADD = \
	../../lib/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS)  @SD2RAW_WIN32_RESOURCE@

endif

sd2raw_SOURCES = \
	sd2raw.cpp

if OS_WIN32

@SD2RAW_WIN32_RESOURCE@ : sd2raw.rc $(sd2raw_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource sd2raw.rc @SD2RAW_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(sd2raw_SOURCES)	\
	sd2raw.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	sd2raw.rc	 

endif
