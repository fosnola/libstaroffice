if BUILD_TOOLS

bin_PROGRAMS = sd2text

AM_CXXFLAGS = -I$(top_srcdir)/inc $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

sd2text_DEPENDENCIES = @SD2TEXT_WIN32_RESOURCE@

if STATIC_TOOLS

sd2text_LDADD = \
	../../lib/@STAROFFICE_OBJDIR@/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.a \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @SD2TEXT_WIN32_RESOURCE@
sd2text_LDFLAGS = -all-static

else	

sd2text_LDADD = \
	../../lib/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @SD2TEXT_WIN32_RESOURCE@

endif

sd2text_SOURCES = \
	sd2text.cpp

if OS_WIN32

@SD2TEXT_WIN32_RESOURCE@ : sd2text.rc $(sd2text_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource sd2text.rc @SD2TEXT_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(sd2text_SOURCES)	\
	sd2text.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	sd2text.rc	 

endif
