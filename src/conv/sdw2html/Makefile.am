if BUILD_TOOLS

bin_PROGRAMS = sdw2html

AM_CXXFLAGS = -I$(top_srcdir)/inc $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

sdw2html_DEPENDENCIES = @SDW2HTML_WIN32_RESOURCE@

if STATIC_TOOLS

sdw2html_LDADD = \
	../../lib/@STAROFFICE_OBJDIR@/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.a \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @SDW2HTML_WIN32_RESOURCE@
sdw2html_LDFLAGS = -all-static

else	

sdw2html_LDADD = \
	../../lib/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @SDW2HTML_WIN32_RESOURCE@

endif

sdw2html_SOURCES = 			\
	sdw2html.cpp

if OS_WIN32

@SDW2HTML_WIN32_RESOURCE@ : sdw2html.rc $(sdw2html_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource sdw2html.rc @SDW2HTML_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	sdw2html.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	sdw2html.rc	 

endif
