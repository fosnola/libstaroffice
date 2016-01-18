if BUILD_TOOLS
bin_PROGRAMS = sd2svg

AM_CXXFLAGS = -I$(top_srcdir)/inc/ $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

sd2svg_DEPENDENCIES = @SD2SVG_WIN32_RESOURCE@

if STATIC_TOOLS

sd2svg_LDADD = \
	../../lib/@STAROFFICE_OBJDIR@/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.a \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @SD2SVG_WIN32_RESOURCE@
sd2svg_LDFLAGS = -all-static

else	

sd2svg_LDADD = \
	../../lib/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @SD2SVG_WIN32_RESOURCE@

endif

sd2svg_SOURCES = \
	sd2svg.cpp

if OS_WIN32

@SD2SVG_WIN32_RESOURCE@ : sd2svg.rc $(sd2svg_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource sd2svg.rc @SD2SVG_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(sd2svg_SOURCES)	\
	sd2svg.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	sd2svg.rc	 

endif
