if BUILD_TOOLS
bin_PROGRAMS = sdc2csv

AM_CXXFLAGS = -I$(top_srcdir)/inc $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

sdc2csv_DEPENDENCIES = @SDC2CSV_WIN32_RESOURCE@

if STATIC_TOOLS

sdc2csv_LDADD = \
	../../lib/@STAROFFICE_OBJDIR@/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.a \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @SDC2CSV_WIN32_RESOURCE@
sdc2csv_LDFLAGS = -all-static

else	

sdc2csv_LDADD = \
	../../lib/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @SDC2CSV_WIN32_RESOURCE@
endif

sdc2csv_SOURCES = \
	sdc2csv.cpp

if OS_WIN32

@SDC2CSV_WIN32_RESOURCE@ : sdc2csv.rc $(sdc2csv_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource sdc2csv.rc @SDC2CSV_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(sdc2csv_SOURCES)	\
	sdc2csv.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	sdc2csv.rc	 

endif
