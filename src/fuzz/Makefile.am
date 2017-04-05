noinst_PROGRAMS = sdafuzzer sdcfuzzer sddfuzzer sdwfuzzer

AM_CXXFLAGS = -I$(top_srcdir)/inc \
	$(REVENGE_GENERATORS_CFLAGS) \
	$(REVENGE_CFLAGS) \
	$(REVENGE_STREAM_CFLAGS) \
	$(DEBUG_CXXFLAGS)

commonfuzzer_ldadd = \
	$(top_builddir)/src/lib/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.la \
	$(REVENGE_GENERATORS_LIBS) \
	$(REVENGE_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	-lFuzzingEngine

sdafuzzer_LDADD = $(commonfuzzer_ldadd)

sdafuzzer_SOURCES = \
	sdafuzzer.cpp

sdcfuzzer_LDADD = $(commonfuzzer_ldadd)

sdcfuzzer_SOURCES = \
	sdcfuzzer.cpp

sddfuzzer_LDADD = $(commonfuzzer_ldadd)

sddfuzzer_SOURCES = \
	sddfuzzer.cpp

sdwfuzzer_LDADD = $(commonfuzzer_ldadd)

sdwfuzzer_SOURCES = \
	sdwfuzzer.cpp
