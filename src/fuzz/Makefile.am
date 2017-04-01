noinst_PROGRAMS = sdcfuzzer

AM_CXXFLAGS = -I$(top_srcdir)/inc \
	$(REVENGE_GENERATORS_CFLAGS) \
	$(REVENGE_CFLAGS) \
	$(REVENGE_STREAM_CFLAGS) \
	$(DEBUG_CXXFLAGS)

sdcfuzzer_LDADD = \
	$(top_builddir)/src/lib/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.la \
	$(REVENGE_GENERATORS_LIBS) \
	$(REVENGE_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	-lFuzzingEngine

sdcfuzzer_SOURCES = \
	sdcfuzzer.cpp
