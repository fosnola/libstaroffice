if BUILD_TOOLS

bin_PROGRAMS = sdw2text

AM_CXXFLAGS = -I$(top_srcdir)/inc $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

sdw2text_DEPENDENCIES = @SDW2TEXT_WIN32_RESOURCE@

if STATIC_TOOLS

sdw2text_LDADD = \
	../../lib/@STAROFFICE_OBJDIR@/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.a \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @SDW2TEXT_WIN32_RESOURCE@
sdw2text_LDFLAGS = -all-static

else	

sdw2text_LDADD = \
	../../lib/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @SDW2TEXT_WIN32_RESOURCE@

endif

sdw2text_SOURCES = \
	sdw2text.cpp

if OS_WIN32

@SDW2TEXT_WIN32_RESOURCE@ : sdw2text.rc $(sdw2text_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource sdw2text.rc @SDW2TEXT_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(sdw2text_SOURCES)	\
	sdw2text.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	sdw2text.rc	 

endif
