if OS_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version
else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
endif

lib_LTLIBRARIES = libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.la $(target_libstaroffice_stream)
libstaroffice_@STAROFFICE_MAJOR_VERSION@_@STAROFFICE_MINOR_VERSION@_includedir = $(includedir)/libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@/libstaroffice
libstaroffice_@STAROFFICE_MAJOR_VERSION@_@STAROFFICE_MINOR_VERSION@_include_HEADERS = \
	$(top_srcdir)/inc/libstaroffice/libstaroffice.hxx \
	$(top_srcdir)/inc/libstaroffice/STOFFDocument.hxx

AM_CXXFLAGS =  -I$(top_srcdir)/inc $(REVENGE_CFLAGS) $(DEBUG_CXXFLAGS) $(ZLIB_CFLAGS) -DBUILD_STAROFFICE=1

libstaroffice_@STAROFFICE_MAJOR_VERSION@_@STAROFFICE_MINOR_VERSION@_la_LIBADD  = $(REVENGE_LIBS) $(ZLIB_LIBS) @LIBSTAROFFICE_WIN32_RESOURCE@
libstaroffice_@STAROFFICE_MAJOR_VERSION@_@STAROFFICE_MINOR_VERSION@_la_DEPENDENCIES = @LIBSTAROFFICE_WIN32_RESOURCE@
libstaroffice_@STAROFFICE_MAJOR_VERSION@_@STAROFFICE_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic -no-undefined
libstaroffice_@STAROFFICE_MAJOR_VERSION@_@STAROFFICE_MINOR_VERSION@_la_SOURCES = \
	SDCParser.cxx				\
	SDCParser.hxx				\
	SDWParser.cxx				\
	SDWParser.hxx				\
	STOFFCell.cxx				\
	STOFFCell.hxx				\
	STOFFChart.cxx				\
	STOFFChart.hxx				\
	STOFFDebug.cxx				\
	STOFFDebug.hxx				\
	STOFFDocument.cxx			\
	STOFFEntry.hxx				\
	STOFFFont.cxx				\
	STOFFFont.hxx				\
	STOFFGraphicDecoder.cxx			\
	STOFFGraphicDecoder.hxx			\
	STOFFHeader.cxx				\
	STOFFHeader.hxx				\
	STOFFInputStream.cxx			\
	STOFFInputStream.hxx			\
	STOFFList.cxx				\
	STOFFList.hxx				\
	STOFFListener.hxx			\
	STOFFOLEParser.cxx			\
	STOFFOLEParser.hxx			\
	STOFFPageSpan.cxx			\
	STOFFPageSpan.hxx			\
	STOFFParagraph.cxx			\
	STOFFParagraph.hxx			\
	STOFFParser.cxx				\
	STOFFParser.hxx				\
	STOFFPosition.hxx			\
	STOFFPropertyHandler.cxx		\
	STOFFPropertyHandler.hxx		\
	STOFFSection.cxx			\
	STOFFSection.hxx			\
	STOFFSpreadsheetDecoder.cxx		\
	STOFFSpreadsheetDecoder.hxx		\
	STOFFSpreadsheetListener.cxx		\
	STOFFSpreadsheetListener.hxx		\
	STOFFStringStream.cxx			\
	STOFFStringStream.hxx			\
	STOFFSubDocument.cxx			\
	STOFFSubDocument.hxx			\
	STOFFTable.cxx				\
	STOFFTable.hxx				\
	StarAttribute.cxx			\
	StarAttribute.hxx			\
	StarBitmap.cxx				\
	StarBitmap.hxx				\
	StarCellFormula.cxx			\
	StarCellFormula.hxx			\
	StarEncodingChinese.cxx			\
	StarEncodingChinese.hxx			\
	StarEncodingJapanese.cxx		\
	StarEncodingJapanese.hxx		\
	StarEncodingKorean.cxx			\
	StarEncodingKorean.hxx			\
	StarEncodingOtherKorean.cxx		\
	StarEncodingOtherKorean.hxx		\
	StarEncodingTradChinese.cxx		\
	StarEncodingTradChinese.hxx		\
	StarEncoding.cxx			\
	StarEncoding.hxx			\
	StarEncryption.cxx			\
	StarEncryption.hxx			\
	StarFileManager.cxx			\
	StarFileManager.hxx			\
	StarItemPool.cxx			\
	StarItemPool.hxx			\
	StarObject.cxx				\
	StarObject.hxx				\
	StarObjectChart.cxx			\
	StarObjectChart.hxx			\
	StarObjectDraw.cxx			\
	StarObjectDraw.hxx			\
	StarObjectSpreadsheet.cxx		\
	StarObjectSpreadsheet.hxx		\
	StarObjectText.cxx			\
	StarObjectText.hxx			\
	StarZone.cxx				\
	StarZone.hxx				\
	SWFieldManager.cxx			\
	SWFieldManager.hxx			\
	SWFormatManager.cxx			\
	SWFormatManager.hxx			\
	libstaroffice_internal.cxx		\
	libstaroffice_internal.hxx

if OS_WIN32

@LIBSTAROFFICE_WIN32_RESOURCE@ : libstaroffice.rc $(libstaroffice_@STAROFFICE_MAJOR_VERSION@_@STAROFFICE_MINOR_VERSION@_la_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource libstaroffice.rc @LIBSTAROFFICE_WIN32_RESOURCE@

endif

EXTRA_DIST = \
	libstaroffice.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	libstaroffice.rc
