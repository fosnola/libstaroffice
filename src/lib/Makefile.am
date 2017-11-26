if OS_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version
else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
endif

lib_LTLIBRARIES = libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.la $(target_libstaroffice_stream)

AM_CXXFLAGS =  -I$(top_srcdir)/inc $(REVENGE_CFLAGS) $(DEBUG_CXXFLAGS) $(ZLIB_CFLAGS) -DBUILD_STAROFFICE=1

libstaroffice_@STAROFFICE_MAJOR_VERSION@_@STAROFFICE_MINOR_VERSION@_la_LIBADD  = $(REVENGE_LIBS) $(ZLIB_LIBS) @LIBSTAROFFICE_WIN32_RESOURCE@
libstaroffice_@STAROFFICE_MAJOR_VERSION@_@STAROFFICE_MINOR_VERSION@_la_DEPENDENCIES = @LIBSTAROFFICE_WIN32_RESOURCE@
libstaroffice_@STAROFFICE_MAJOR_VERSION@_@STAROFFICE_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic -no-undefined
libstaroffice_@STAROFFICE_MAJOR_VERSION@_@STAROFFICE_MINOR_VERSION@_la_SOURCES = \
	SDAParser.cxx				\
	SDAParser.hxx				\
	SDCParser.cxx				\
	SDCParser.hxx				\
	SDGParser.cxx				\
	SDGParser.hxx				\
	SDWParser.cxx				\
	SDWParser.hxx				\
	SDXParser.cxx				\
	SDXParser.hxx				\
	STOFFCell.cxx				\
	STOFFCell.hxx				\
	STOFFCellStyle.cxx			\
	STOFFCellStyle.hxx			\
	STOFFChart.cxx				\
	STOFFChart.hxx				\
	STOFFDebug.cxx				\
	STOFFDebug.hxx				\
	STOFFDocument.cxx			\
	STOFFEntry.hxx				\
	STOFFEntry.cxx				\
	STOFFFont.cxx				\
	STOFFFont.hxx				\
	STOFFFrameStyle.cxx			\
	STOFFFrameStyle.hxx			\
	STOFFGraphicDecoder.cxx			\
	STOFFGraphicDecoder.hxx			\
	STOFFGraphicEncoder.cxx			\
	STOFFGraphicEncoder.hxx			\
	STOFFGraphicListener.hxx		\
	STOFFGraphicListener.cxx		\
	STOFFGraphicShape.hxx			\
	STOFFGraphicShape.cxx			\
	STOFFGraphicStyle.cxx			\
	STOFFGraphicStyle.hxx			\
	STOFFHeader.cxx				\
	STOFFHeader.hxx				\
	STOFFInputStream.cxx			\
	STOFFInputStream.hxx			\
	STOFFList.cxx				\
	STOFFList.hxx				\
	STOFFListener.hxx			\
	STOFFListener.cxx			\
	STOFFOLEParser.cxx			\
	STOFFOLEParser.hxx			\
	STOFFPageSpan.cxx			\
	STOFFPageSpan.hxx			\
	STOFFParagraph.cxx			\
	STOFFParagraph.hxx			\
	STOFFParser.cxx				\
	STOFFParser.hxx				\
	STOFFPosition.hxx			\
	STOFFPosition.cxx			\
	STOFFPropertyHandler.cxx		\
	STOFFPropertyHandler.hxx		\
	STOFFSection.cxx			\
	STOFFSection.hxx			\
	STOFFSpreadsheetDecoder.cxx		\
	STOFFSpreadsheetDecoder.hxx		\
	STOFFSpreadsheetEncoder.cxx		\
	STOFFSpreadsheetEncoder.hxx		\
	STOFFSpreadsheetListener.cxx		\
	STOFFSpreadsheetListener.hxx		\
	STOFFStarMathToMMLConverter.cxx		\
	STOFFStarMathToMMLConverter.hxx		\
	STOFFStringStream.cxx			\
	STOFFStringStream.hxx			\
	STOFFSubDocument.cxx			\
	STOFFSubDocument.hxx			\
	STOFFTable.cxx				\
	STOFFTable.hxx				\
	STOFFTextListener.cxx			\
	STOFFTextListener.hxx			\
	StarAttribute.cxx			\
	StarAttribute.hxx			\
	StarBitmap.cxx				\
	StarBitmap.hxx				\
	StarCellAttribute.cxx			\
	StarCellAttribute.hxx			\
	StarCellFormula.cxx			\
	StarCellFormula.hxx			\
	StarCharAttribute.cxx			\
	StarCharAttribute.hxx			\
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
	StarFormatManager.cxx			\
	StarFormatManager.hxx			\
	StarFrameAttribute.cxx			\
	StarFrameAttribute.hxx			\
	StarGraphicAttribute.cxx		\
	StarGraphicAttribute.hxx		\
	StarGraphicStruct.cxx			\
	StarGraphicStruct.hxx			\
	StarItem.cxx				\
	StarItem.hxx				\
	StarItemPool.cxx			\
	StarItemPool.hxx			\
	StarLanguage.cxx			\
	StarLanguage.hxx			\
	StarLayout.cxx				\
	StarLayout.hxx				\
	StarObject.cxx				\
	StarObject.hxx				\
	StarObjectChart.cxx			\
	StarObjectChart.hxx			\
	StarObjectDraw.cxx			\
	StarObjectDraw.hxx			\
	StarObjectMath.cxx			\
	StarObjectMath.hxx			\
	StarObjectModel.cxx			\
	StarObjectModel.hxx			\
	StarObjectNumericRuler.cxx		\
	StarObjectNumericRuler.hxx		\
	StarObjectPageStyle.cxx			\
	StarObjectPageStyle.hxx			\
	StarObjectSmallGraphic.cxx		\
	StarObjectSmallGraphic.hxx		\
	StarObjectSmallText.cxx			\
	StarObjectSmallText.hxx			\
	StarObjectSpreadsheet.cxx		\
	StarObjectSpreadsheet.hxx		\
	StarObjectText.cxx			\
	StarObjectText.hxx			\
	StarPageAttribute.cxx			\
	StarPageAttribute.hxx			\
	StarParagraphAttribute.cxx		\
	StarParagraphAttribute.hxx		\
	StarState.cxx				\
	StarState.hxx				\
	StarTable.cxx				\
	StarTable.hxx				\
	StarWriterStruct.cxx			\
	StarWriterStruct.hxx			\
	StarZone.cxx				\
	StarZone.hxx				\
	SWFieldManager.cxx			\
	SWFieldManager.hxx			\
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
