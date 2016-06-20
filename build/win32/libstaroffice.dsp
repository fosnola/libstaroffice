# Microsoft Developer Studio Project File - Name="libstaroffice" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libstaroffice - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libstaroffice.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libstaroffice.mak" CFG="libstaroffice - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libstaroffice - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libstaroffice - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libstaroffice - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /YX /FD " /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /D "NDEBUG" /D "WIN32" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\lib\libstaroffice-0.0.lib"

!ELSEIF  "$(CFG)" == "libstaroffice - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "DEBUG" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /GR /GX /ZI /Od /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\lib\libstaroffice-0.0.lib"

!ENDIF 

# Begin Target

# Name "libstaroffice - Win32 Release"
# Name "libstaroffice - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\lib\SDAParser.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\SDCParser.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\SDGParser.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\SDWParser.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFCell.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFCellStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFChart.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFDebug.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFDocument.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFEntry.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFFont.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFGraphicDecoder.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFGraphicListener.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFGraphicShape.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFGraphicStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFHeader.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFInputStream.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFList.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFListener.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFOLEParser.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFPageSpan.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFParagraph.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFParser.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFPosition.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFPropertyHandler.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFSection.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFSpreadsheetDecoder.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFSpreadsheetListener.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFStringStream.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFSubDocument.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFTable.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarAttribute.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarBitmap.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarCellAttribute.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarCellFormula.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarCharAttribute.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncodingChinese.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncodingJapanese.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncodingKorean.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncodingOtherKorean.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncodingTradChinese.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncoding.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncryption.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarFileManager.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarFormatManager.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarGraphicAttribute.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarGraphicStruct.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarItem.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarItemPool.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarLanguage.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObject.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectChart.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectDraw.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectModel.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectSmallGraphic.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectSmallText.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectSpreadsheet.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectText.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarPageAttribute.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarParagraphAttribute.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarZone.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\SWFieldManager.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\libstaroffice_internal.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\inc\libstaroffice\libstaroffice.hxx
# End Source File
# Begin Source File

SOURCE=..\..\inc\libstaroffice\STOFFDocument.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\SDAParser.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\SDCParser.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\SDGParser.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\SDWParser.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFCell.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFCellStyle.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFChart.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFDebug.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFEntry.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFFont.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFGraphicDecoder.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFGraphicListener.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFGraphicShape.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFGraphicStyle.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFHeader.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFInputStream.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFList.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFListener.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFOLEParser.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFPageSpan.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFParagraph.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFParser.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFPosition.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFPropertyHandler.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFSection.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFSpreadsheetDecoder.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFSpreadsheetListener.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFStringStream.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFSubDocument.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\STOFFTable.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarAttribute.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarBitmap.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarCellAttribute.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarCellFormula.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarCharAttribute.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncodingChinese.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncodingJapanese.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncodingKorean.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncodingOtherKorean.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncodingTradChinese.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncoding.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarEncryption.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarFileManager.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarFormatManager.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarGraphicAttribute.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarGraphicStruct.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarItem.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarItemPool.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarLanguage.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObject.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectChart.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectDraw.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectModel.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectSmallGraphic.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectSmallText.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectSpreadsheet.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarObjectText.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarPageAttribute.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarParagraphAttribute.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\StarZone.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\SWFieldManager.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\libstaroffice_internal.hxx
# End Source File
# End Group
# End Target
# End Project
