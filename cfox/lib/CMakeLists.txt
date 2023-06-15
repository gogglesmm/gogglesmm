include(CheckIncludeFileCXX)
include(CheckCXXSourceCompiles)
include(CheckCXXSymbolExists)
include(CMakePushCheckState)

option(WITH_XIM "Compile with X Input Method support" ON)
option(WITH_JPEG "Compile with JPEG support" ON)
option(WITH_PNG "Compile with PNG support" ON)
option(WITH_TIFF "Compile with TIFF support" ON)
option(WITH_ZLIB "Compile with zlib support" ON)
option(WITH_BZ2LIB "Compile with bz2lib support" ON)
option(WITH_WEBP "Compile with webp support" ON)
option(WITH_OPENJPEG "Compile with OpenJPEG support" ON)
option(WITH_OPENGL "Compile with OpenGL support" ON)

# libdir name to allow overriding to lib32 / lib64
set(CMAKE_INSTALL_LIBDIR "lib" CACHE STRING "Specify the name of the directory where libraries are meant to be installed")

set(HEADERS ../include/fx3d.h
            ../include/FX4Splitter.h
            ../include/FX7Segment.h
            ../include/FX885910Codec.h
            ../include/FX885911Codec.h
            ../include/FX885913Codec.h
            ../include/FX885914Codec.h
            ../include/FX885915Codec.h
            ../include/FX885916Codec.h
            ../include/FX88591Codec.h
            ../include/FX88592Codec.h
            ../include/FX88593Codec.h
            ../include/FX88594Codec.h
            ../include/FX88595Codec.h
            ../include/FX88596Codec.h
            ../include/FX88597Codec.h
            ../include/FX88598Codec.h
            ../include/FX88599Codec.h
            ../include/FXAccelTable.h
            ../include/FXApp.h
            ../include/FXArray.h
            ../include/FXArrowButton.h
            ../include/fxascii.h
            ../include/FXAtomic.h
            ../include/FXAutoPtr.h
            ../include/FXAutoThreadStorageKey.h
            ../include/FXBarrier.h
            ../include/FXBitmapFrame.h
            ../include/FXBitmap.h
            ../include/FXBitmapView.h
            ../include/FXBMPIcon.h
            ../include/FXBMPImage.h
            ../include/FXButton.h
            ../include/FXBZFileStream.h
            ../include/FXCalendar.h
            ../include/FXCalendarView.h
            ../include/FXCallback.h
            ../include/FXCanvas.h
            ../include/FXCheckButton.h
            ../include/FXChoiceBox.h
            ../include/FXColorBar.h
            ../include/FXColorDialog.h
            ../include/FXColorList.h
            ../include/FXColorRing.h
            ../include/FXColorSelector.h
            ../include/FXColors.h
            ../include/FXColorWell.h
            ../include/FXColorWheel.h
            ../include/FXComboBox.h
            ../include/FXCompletion.h
            ../include/FXComplexd.h
            ../include/FXComplexf.h
            ../include/FXComposeContext.h
            ../include/FXComposite.h
            ../include/FXCondition.h
            ../include/FXConsole.h
            ../include/FXCP1250Codec.h
            ../include/FXCP1251Codec.h
            ../include/FXCP1252Codec.h
            ../include/FXCP1253Codec.h
            ../include/FXCP1254Codec.h
            ../include/FXCP1255Codec.h
            ../include/FXCP1256Codec.h
            ../include/FXCP1257Codec.h
            ../include/FXCP1258Codec.h
            ../include/FXCP437Codec.h
            ../include/FXCP850Codec.h
            ../include/FXCP852Codec.h
            ../include/FXCP855Codec.h
            ../include/FXCP856Codec.h
            ../include/FXCP857Codec.h
            ../include/FXCP860Codec.h
            ../include/FXCP861Codec.h
            ../include/FXCP862Codec.h
            ../include/FXCP863Codec.h
            ../include/FXCP864Codec.h
            ../include/FXCP865Codec.h
            ../include/FXCP866Codec.h
            ../include/FXCP869Codec.h
            ../include/FXCP874Codec.h
            ../include/fxchar.h
            ../include/fxcpuid.h
            ../include/FXCURCursor.h
            ../include/FXCursor.h
            ../include/FXDataTarget.h
            ../include/FXDate.h
            ../include/FXDC.h
            ../include/FXDCPrint.h
            ../include/FXDCWindow.h
            ../include/FXDDSIcon.h
            ../include/FXDDSImage.h
            ../include/FXDebugTarget.h
            ../include/fxdefs.h
            ../include/FXDelegator.h
            ../include/FXDial.h
            ../include/FXDialogBox.h
            ../include/FXDictionary.h
            ../include/FXDictionaryOf.h
            ../include/FXDirBox.h
            ../include/FXDirDialog.h
            ../include/FXDir.h
            ../include/FXDirList.h
            ../include/FXDirSelector.h
            ../include/FXDirVisitor.h
            ../include/FXDirWatch.h
            ../include/FXDispatcher.h
            ../include/FXDisplay.h
            ../include/FXDLL.h
            ../include/FXDockBar.h
            ../include/FXDockHandler.h
            ../include/FXDockSite.h
            ../include/FXDockTitle.h
            ../include/FXDocument.h
            ../include/FXDragCorner.h
            ../include/FXDrawable.h
            ../include/FXDriveBox.h
            ../include/FXElement.h
            ../include/fxendian.h
            ../include/FXEvent.h
            ../include/FXEventDispatcher.h
            ../include/FXEventLoop.h
            ../include/FXException.h
            ../include/FXEXEIcon.h
            ../include/FXEXEImage.h
            ../include/FXExpression.h
            ../include/FXExtentd.h
            ../include/FXExtentf.h
            ../include/FXFileAssociations.h
            ../include/FXFileDialog.h
            ../include/FXFile.h
            ../include/FXFileList.h
            ../include/FXFileProgressDialog.h
            ../include/FXFileSelector.h
            ../include/FXFileStream.h
            ../include/FXFoldingList.h
            ../include/FXFontDialog.h
            ../include/FXFont.h
            ../include/FXFontSelector.h
            ../include/FXFrame.h
            ../include/FXGauge.h
            ../include/FXGIFCursor.h
            ../include/FXGIFIcon.h
            ../include/FXGIFImage.h
            ../include/FXGLCanvas.h
            ../include/FXGLContext.h
            ../include/FXGLObject.h
            ../include/FXGLViewer.h
            ../include/FXGLVisual.h
            ../include/FXGradientBar.h
            ../include/FXGroupBox.h
            ../include/FXGZFileStream.h
            ../include/fx.h
            ../include/FXhalf.h
            ../include/FXHash.h
            ../include/FXHashOf.h
            ../include/FXHeader.h
            ../include/FXHorizontalFrame.h
            ../include/FXICOIcon.h
            ../include/FXICOImage.h
            ../include/FXIconCache.h
            ../include/FXIcon.h
            ../include/FXIconList.h
            ../include/FXIconSource.h
            ../include/FXId.h
            ../include/FXIFFIcon.h
            ../include/FXIFFImage.h
            ../include/FXImageFrame.h
            ../include/FXImage.h
            ../include/FXImageView.h
            ../include/FXINI.h
            ../include/FXINIFile.h
            ../include/FXInputDialog.h
            ../include/FXIOBuffer.h
            ../include/FXIODevice.h
            ../include/FXIO.h
            ../include/FXJP2Icon.h
            ../include/FXJP2Image.h
            ../include/FXJPGIcon.h
            ../include/FXJPGImage.h
            ../include/FXJSON.h
            ../include/FXJSONFile.h
            ../include/FXJSONString.h
            ../include/fxkeys.h
            ../include/FXKnob.h
            ../include/FXKOI8RCodec.h
            ../include/FXLabel.h
            ../include/FXLFQueue.h
            ../include/FXListBox.h
            ../include/FXList.h
            ../include/FXLocale.h
            ../include/FXMainWindow.h
            ../include/FXMat2d.h
            ../include/FXMat2f.h
            ../include/FXMat3d.h
            ../include/FXMat3f.h
            ../include/FXMat4d.h
            ../include/FXMat4f.h
            ../include/fxmath.h
            ../include/FXMatrix.h
            ../include/FXMDIButton.h
            ../include/FXMDIChild.h
            ../include/FXMDIClient.h
            ../include/FXMemMap.h
            ../include/FXMemoryStream.h
            ../include/FXMenuBar.h
            ../include/FXMenuButton.h
            ../include/FXMenuCaption.h
            ../include/FXMenuCascade.h
            ../include/FXMenuCheck.h
            ../include/FXMenuCommand.h
            ../include/FXMenuPane.h
            ../include/FXMenuRadio.h
            ../include/FXMenuSeparator.h
            ../include/FXMenuTitle.h
            ../include/FXMessageBox.h
            ../include/FXMessageChannel.h
            ../include/FXMetaClass.h
            ../include/FXMutex.h
            ../include/FXObject.h
            ../include/FXObjectList.h
            ../include/FXOptionMenu.h
            ../include/FXPacker.h
            ../include/FXParallel.h
            ../include/FXParseBuffer.h
            ../include/FXPath.h
            ../include/FXPCXIcon.h
            ../include/FXPCXImage.h
            ../include/FXPicker.h
            ../include/FXPipe.h
            ../include/FXPNGIcon.h
            ../include/FXPNGImage.h
            ../include/FXPoint.h
            ../include/FXPopup.h
            ../include/FXPPMIcon.h
            ../include/FXPPMImage.h
            ../include/FXPrintDialog.h
            ../include/FXProcess.h
            ../include/FXProgressBar.h
            ../include/FXProgressDialog.h
            ../include/FXPtrList.h
            ../include/FXPtrQueue.h
            ../include/FXQuatd.h
            ../include/FXQuatf.h
            ../include/FXRadioButton.h
            ../include/FXRandom.h
            ../include/FXRanged.h
            ../include/FXRangef.h
            ../include/FXRangeSlider.h
            ../include/FXRASIcon.h
            ../include/FXRASImage.h
            ../include/FXReactor.h
            ../include/FXReadWriteLock.h
            ../include/FXRealSlider.h
            ../include/FXRealSpinner.h
            ../include/FXRecentFiles.h
            ../include/FXRectangle.h
            ../include/FXRefPtr.h
            ../include/FXRegion.h
            ../include/FXRegistry.h
            ../include/FXReplaceDialog.h
            ../include/FXReverseDictionary.h
            ../include/FXReverseDictionaryOf.h
            ../include/FXRex.h
            ../include/FXRGBIcon.h
            ../include/FXRGBImage.h
            ../include/FXRootWindow.h
            ../include/FXRuler.h
            ../include/FXRulerView.h
            ../include/FXRunnable.h
            ../include/FXScopedThread.h
            ../include/FXScrollArea.h
            ../include/FXScrollBar.h
            ../include/FXScrollPane.h
            ../include/FXScrollWindow.h
            ../include/FXSearchDialog.h
            ../include/FXSemaphore.h
            ../include/FXSemaQueue.h
            ../include/FXSeparator.h
            ../include/FXSettings.h
            ../include/FXShell.h
            ../include/FXShutter.h
            ../include/FXSize.h
            ../include/FXSlider.h
            ../include/FXSocket.h
            ../include/FXSphered.h
            ../include/FXSpheref.h
            ../include/FXSpinLock.h
            ../include/FXSpinner.h
            ../include/FXSplashWindow.h
            ../include/FXSplitter.h
            ../include/FXSpring.h
            ../include/FXStat.h
            ../include/FXStatusBar.h
            ../include/FXStatusLine.h
            ../include/FXStream.h
            ../include/FXStringDictionary.h
            ../include/FXString.h
            ../include/FXSwitcher.h
            ../include/FXSystem.h
            ../include/FXTabBar.h
            ../include/FXTabBook.h
            ../include/FXTabItem.h
            ../include/FXTable.h
            ../include/FXTaskGroup.h
            ../include/FXTextCodec.h
            ../include/FXTextField.h
            ../include/FXText.h
            ../include/FXTGAIcon.h
            ../include/FXTGAImage.h
            ../include/FXThread.h
            ../include/FXThreadPool.h
            ../include/FXTIFIcon.h
            ../include/FXTIFImage.h
            ../include/FXToggleButton.h
            ../include/FXToolBarGrip.h
            ../include/FXToolBar.h
            ../include/FXToolBarShell.h
            ../include/FXToolBarTab.h
            ../include/FXToolTip.h
            ../include/FXTopWindow.h
            ../include/FXTranslator.h
            ../include/FXTreeListBox.h
            ../include/FXTreeList.h
            ../include/FXTriStateButton.h
            ../include/FXUndoList.h
            ../include/fxunicode.h
            ../include/FXURL.h
            ../include/FXUTF16Codec.h
            ../include/FXUTF32Codec.h
            ../include/FXUTF8Codec.h
            ../include/FXVariantArray.h
            ../include/FXVariant.h
            ../include/FXVariantMap.h
            ../include/FXVec2d.h
            ../include/FXVec2f.h
            ../include/FXVec3d.h
            ../include/FXVec3f.h
            ../include/FXVec4d.h
            ../include/FXVec4f.h
            ../include/fxver.h
            ../include/FXVerticalFrame.h
            ../include/FXVisual.h
            ../include/FXWEBPIcon.h
            ../include/FXWEBPImage.h
            ../include/FXWindow.h
            ../include/FXWizard.h
            ../include/FXWorker.h
            ../include/FXWSQueue.h
            ../include/FXXBMIcon.h
            ../include/FXXBMImage.h
            ../include/FXXML.h
            ../include/FXXMLFile.h
            ../include/FXXMLString.h
            ../include/FXXPMIcon.h
            ../include/FXXPMImage.h
            ../include/xincs.h)

set(SOURCES FX4Splitter.cpp
            FX7Segment.cpp
            FX885910Codec.cpp
            FX885911Codec.cpp
            FX885913Codec.cpp
            FX885914Codec.cpp
            FX885915Codec.cpp
            FX885916Codec.cpp
            FX88591Codec.cpp
            FX88592Codec.cpp
            FX88593Codec.cpp
            FX88594Codec.cpp
            FX88595Codec.cpp
            FX88596Codec.cpp
            FX88597Codec.cpp
            FX88598Codec.cpp
            FX88599Codec.cpp
            FXAccelTable.cpp
            FXApp.cpp
            FXArray.cpp
            FXArrowButton.cpp
            fxascii.cpp
            FXAtomic.cpp
            FXAutoThreadStorageKey.cpp
            FXBarrier.cpp
            fxbase64.cpp
            fxbase85.cpp
            FXBitmap.cpp
            FXBitmapFrame.cpp
            FXBitmapView.cpp
            FXBMPIcon.cpp
            FXBMPImage.cpp
            fxbmpio.cpp
            FXButton.cpp
            FXBZFileStream.cpp
            FXCanvas.cpp
            fxchar.cpp
            FXCheckButton.cpp
            FXChoiceBox.cpp
            FXColorBar.cpp
            FXColorDialog.cpp
            FXColorList.cpp
            FXColorRing.cpp
            FXColors.cpp
            FXColorSelector.cpp
            FXColorWell.cpp
            FXColorWheel.cpp
            FXComboBox.cpp
            FXCompletion.cpp
            FXComplexd.cpp
            FXComplexf.cpp
            FXComposeContext.cpp
            fxcompose.cpp
            FXComposite.cpp
            FXCondition.cpp
            FXCP1250Codec.cpp
            FXCP1251Codec.cpp
            FXCP1252Codec.cpp
            FXCP1253Codec.cpp
            FXCP1254Codec.cpp
            FXCP1255Codec.cpp
            FXCP1256Codec.cpp
            FXCP1257Codec.cpp
            FXCP1258Codec.cpp
            FXCP437Codec.cpp
            FXCP850Codec.cpp
            FXCP852Codec.cpp
            FXCP855Codec.cpp
            FXCP856Codec.cpp
            FXCP857Codec.cpp
            FXCP860Codec.cpp
            FXCP861Codec.cpp
            FXCP862Codec.cpp
            FXCP863Codec.cpp
            FXCP864Codec.cpp
            FXCP865Codec.cpp
            FXCP866Codec.cpp
            FXCP869Codec.cpp
            FXCP874Codec.cpp
            fxcpuid.cpp
            FXCURCursor.cpp
            FXCursor.cpp
            FXDataTarget.cpp
            FXDate.cpp
            FXDC.cpp
            FXDCWindow.cpp
            FXDDSIcon.cpp
            FXDDSImage.cpp
            fxddsio.cpp
            FXDebugTarget.cpp
            FXDelegator.cpp
            FXDial.cpp
            FXDialogBox.cpp
            FXDictionary.cpp
            FXDirBox.cpp
            FXDir.cpp
            FXDirDialog.cpp
            FXDirList.cpp
            FXDirSelector.cpp
            FXDirVisitor.cpp
            FXDirWatch.cpp
            FXDispatcher.cpp
            FXDisplay.cpp
            FXDLL.cpp
            FXDockBar.cpp
            FXDockHandler.cpp
            FXDockSite.cpp
            FXDocument.cpp
            FXDragCorner.cpp
            FXDrawable.cpp
            FXDriveBox.cpp
            FXEventDispatcher.cpp
            FXEventLoop.cpp
            FXException.cpp
            FXEXEIcon.cpp
            FXEXEImage.cpp
            fxexeio.cpp
            FXExpression.cpp
            FXExtentd.cpp
            FXExtentf.cpp
            fxezquantize.cpp
            FXFileAssociations.cpp
            FXFile.cpp
            FXFileDialog.cpp
            FXFileList.cpp
            FXFileSelector.cpp
            FXFileStream.cpp
            FXFoldingList.cpp
            FXFont.cpp
            FXFontDialog.cpp
            FXFontSelector.cpp
            FXFrame.cpp
            fxfsquantize.cpp
            FXGIFCursor.cpp
            FXGIFIcon.cpp
            FXGIFImage.cpp
            fxgifio.cpp
            FXGLCanvas.cpp
            FXGLContext.cpp
            FXGLVisual.cpp
            FXGradientBar.cpp
            FXGroupBox.cpp
            FXGZFileStream.cpp
            FXhalf.cpp
            FXHash.cpp
            FXHeader.cpp
            FXHorizontalFrame.cpp
            FXICOIcon.cpp
            FXICOImage.cpp
            FXIconCache.cpp
            FXIcon.cpp
            FXIconList.cpp
            FXIconSource.cpp
            FXId.cpp
            FXIFFIcon.cpp
            FXIFFImage.cpp
            fxiffio.cpp
            FXImage.cpp
            FXImageFrame.cpp
            FXImageView.cpp
            FXINI.cpp
            FXINIFile.cpp
            FXInputDialog.cpp
            FXIOBuffer.cpp
            FXIO.cpp
            FXIODevice.cpp
            FXJP2Icon.cpp
            FXJP2Image.cpp
            fxjp2io.cpp
            fxjpegio.cpp
            FXJPGIcon.cpp
            FXJPGImage.cpp
            FXJSON.cpp
            FXJSONFile.cpp
            FXJSONString.cpp
            fxkeyboard.cpp
            fxkeysym.cpp
            FXKOI8RCodec.cpp
            FXLabel.cpp
            fxleaps.cpp
            FXLFQueue.cpp
            FXListBox.cpp
            FXList.cpp
            FXLocale.cpp
            FXMainWindow.cpp
            FXMappedFile.cpp
            fxmath.cpp
            FXMatrix.cpp
            FXMemoryStream.cpp
            FXMenuBar.cpp
            FXMenuButton.cpp
            FXMenuCaption.cpp
            FXMenuCascade.cpp
            FXMenuCheck.cpp
            FXMenuCommand.cpp
            FXMenuPane.cpp
            FXMenuRadio.cpp
            FXMenuSeparator.cpp
            FXMenuTitle.cpp
            FXMessageBox.cpp
            FXMessageChannel.cpp
            FXMetaClass.cpp
            FXMutex.cpp
            fxntp.cpp
            FXObject.cpp
            FXObjectList.cpp
            FXOptionMenu.cpp
            fxosver.cpp
            FXPacker.cpp
            FXParseBuffer.cpp
            fxparsegeometry.cpp
            FXPath.cpp
            FXPCXIcon.cpp
            FXPCXImage.cpp
            fxpcxio.cpp
            FXPicker.cpp
            FXPipe.cpp
            FXPNGIcon.cpp
            FXPNGImage.cpp
            fxpngio.cpp
            FXPoint.cpp
            FXPopup.cpp
            FXPPMIcon.cpp
            FXPPMImage.cpp
            fxppmio.cpp
            FXPrintDialog.cpp
            fxprintf.cpp
            fxpriv.cpp
            FXProcess.cpp
            FXProgressBar.cpp
            FXProgressDialog.cpp
            FXPtrList.cpp
            FXPtrQueue.cpp
            FXReactor.cpp
            FXRadioButton.cpp
            FXRandom.cpp
            FXRangeSlider.cpp
            FXRASIcon.cpp
            FXRASImage.cpp
            fxrasio.cpp
            FXReadWriteLock.cpp
            FXRealSlider.cpp
            FXRealSpinner.cpp
            FXRecentFiles.cpp
            FXRectangle.cpp
            FXRegion.cpp
            FXRegistry.cpp
            FXReplaceDialog.cpp
            FXReverseDictionary.cpp
            FXRex.cpp
            FXRGBIcon.cpp
            FXRGBImage.cpp
            fxrgbio.cpp
            FXRootWindow.cpp
            FXRuler.cpp
            FXRulerView.cpp
            fxscanf.cpp
            FXScopedThread.cpp
            FXScrollArea.cpp
            FXScrollBar.cpp
            FXScrollPane.cpp
            FXScrollWindow.cpp
            FXSearchDialog.cpp
            FXSemaphore.cpp
            FXSemaQueue.cpp
            FXSeparator.cpp
            FXSettings.cpp
            FXShell.cpp
            FXShutter.cpp
            FXSize.cpp
            FXSlider.cpp
            FXSpinLock.cpp
            FXSpinner.cpp
            FXSplashWindow.cpp
            FXSplitter.cpp
            FXSpring.cpp
            FXStat.cpp
            FXStatusBar.cpp
            FXStatusLine.cpp
            FXStream.cpp
            FXString.cpp
            FXStringDictionary.cpp
            fxstrtod.cpp
            fxstrtoll.cpp
            fxstrtoull.cpp
            FXSwitcher.cpp
            FXSystem.cpp
            FXSystemTime.cpp
            FXSystemTimeFormat.cpp
            FXSystemTimeParse.cpp
            fxtabbify.cpp
            FXTabBar.cpp
            FXTabBook.cpp
            FXTabItem.cpp
            FXTable.cpp
            fxtargaio.cpp
            FXTaskGroup.cpp
            FXTextCodec.cpp
            FXText.cpp
            FXTextField.cpp
            FXTGAIcon.cpp
            FXTGAImage.cpp
            FXThread.cpp
            FXThreadPool.cpp
            FXTIFIcon.cpp
            FXTIFImage.cpp
            fxtifio.cpp
            FXToggleButton.cpp
            FXToolBar.cpp
            FXToolBarGrip.cpp
            FXToolBarShell.cpp
            FXToolBarTab.cpp
            FXToolTip.cpp
            FXTopWindow.cpp
            FXTranslator.cpp
            FXTreeListBox.cpp
            FXTreeList.cpp
            FXTriStateButton.cpp
            FXUndoList.cpp
            fxunicode.cpp
            FXURL.cpp
            FXUTF16Codec.cpp
            FXUTF32Codec.cpp
            FXUTF8Codec.cpp
            fxutils.cpp
            FXVariantArray.cpp
            FXVariant.cpp
            FXVariantMap.cpp
            FXVec3f.cpp
            FXVec4f.cpp
            FXVerticalFrame.cpp
            FXVisual.cpp
            FXWEBPIcon.cpp
            FXWEBPImage.cpp
            fxwebpio.cpp
            FXWindow.cpp
            FXWorker.cpp
            FXWSQueue.cpp
            fxwuquantize.cpp
            FXXBMIcon.cpp
            FXXBMImage.cpp
            fxxbmio.cpp
            FXXML.cpp
            FXXMLFile.cpp
            FXXMLString.cpp
            FXXPMIcon.cpp
            FXXPMImage.cpp
            fxxpmio.cpp
            icons.cpp)


set(ICONS
arrownext.gif
arrowprev.gif
bigapp.gif
bigcdrom.gif
bigcomputer.gif
bigdesktop.gif
bigdoc.gif
bigfloppy.gif
bigfloppy3.gif
bigfloppy5.gif
bigfolder.gif
bigfolderopen.gif
bigharddisk.gif
bignetdrive.gif
bignethood.gif
bookclr_gif.gif
bookdel_gif.gif
bookmrk_gif.gif
bookset_gif.gif
clearentry.gif
cmymode.gif
dialmode.gif
dirup_gif.gif
docktop.gif
dockbottom.gif
dockleft.gif
dockright.gif
dockfree.gif
dockflip.gif
entericon.gif
erroricon.gif
eyedrop.gif
filecancel.gif
fileaccept.gif
filecopy.gif
filemove.gif
filehidden.gif
filelink.gif
filerename.gif
filedelete.gif
fileshown.gif
foldernew.gif
hammer_gif.gif
gotohome.gif
gotowork.gif
hsvmode.gif
infoicon.gif
landscape.gif
listmode.gif
miniapp.gif
minicdrom.gif
minicomputer.gif
minidesktop.gif
minidoc.gif
minifloppy.gif
minifloppy3.gif
minifloppy5.gif
minifolder.gif
minifolderopen.gif
miniharddisk.gif
mininetdrive.gif
mininethood.gif
minizipdrive.gif
newfolder.bmp
portrait.gif
questionicon.gif
redo_gif.gif
rgbmode.gif
searchicon.gif
showbigicons.gif
showdetails.gif
showsmallicons.gif
sorting.bmp
undo_gif.gif
warningicon.gif
winclose.gif
winmaximize.gif
winminimize.gif
winrestore.gif)

add_library(cfox ${SOURCES})

find_package(PkgConfig)
find_package(Threads)
find_package(X11)
find_package(Freetype)
pkg_check_modules(FONTCONFIG fontconfig)

if(WITH_OPENGL)
  find_package(OpenGL)
endif()

check_cxx_source_compiles( "#include <immintrin.h>
int main(int argc,char *argv) { return 0; }" HAVE_IMMINTRIN_H)
if(HAVE_IMMINTRIN_H)
  add_definitions(-DHAVE_IMMINTRIN_H)
endif()


check_cxx_symbol_exists(localtime_r "time.h" HAVE_LOCALTIME_R)
if(HAVE_LOCALTIME_R)
  add_definitions(-DHAVE_LOCALTIME_R)
endif()

check_cxx_symbol_exists(clock_gettime "time.h" HAVE_CLOCK_GETTIME_NORT)
if(NOT HAVE_CLOCK_GETTIME_NORT)
  cmake_push_check_state()
  set(CMAKE_REQUIRED_LIBRARIES -lrt)
  check_cxx_symbol_exists(clock_gettime "time.h" HAVE_CLOCK_GETTIME)
  if(HAVE_CLOCK_GETTIME)
    list(APPEND LIBRARIES -lrt)
  endif()
  cmake_pop_check_state()
endif()

cmake_push_check_state()
  check_include_file_cxx(dlfcn.h HAVE_DLFCN_H)
  if(HAVE_DLFCN_H)
    add_definitions(-DHAVE_DLFCN_H)
  endif()
  set(CMAKE_REQUIRED_LIBRARIES -ldl)
  check_cxx_symbol_exists(dlopen "dlfcn.h" HAVE_DLOPEN)
  if(HAVE_DLOPEN)
    list(APPEND LIBRARIES -ldl)
  endif()
cmake_pop_check_state()


cmake_push_check_state()
set(CMAKE_REQUIRED_LIBRARIES -ldld)
check_cxx_symbol_exists(shl_load "" HAVE_SHL_LOAD)
if(HAVE_DLOPEN_NODL)
  add_definitions(-DHAVE_SHL_LOAD)
  list(APPEND LIBRARIES -ldld)
endif()
cmake_pop_check_state()

check_cxx_symbol_exists(gmtime_r "time.h" HAVE_GMTIME_R)
if(HAVE_GMTIME_R)
  add_definitions(-DHAVE_GMTIME_R)
endif()

check_cxx_symbol_exists(readdir_r "dirent.h" HAVE_READDIR_R)
if(HAVE_READDIR_R)
  add_definitions(-DHAVE_READDIR_R)
endif()

check_cxx_symbol_exists(getpwuid_r "sys/types.h;pwd.h" HAVE_GETPWUID_R)
if(HAVE_GETPWUID_R)
  add_definitions(-DHAVE_GETPWUID_R)
endif()

check_cxx_symbol_exists(getpwnam_r "sys/types.h;pwd.h" HAVE_GETPWNAM_R)
if(HAVE_GETPWNAM_R)
  add_definitions(-DHAVE_GETPWNAM_R)
endif()

check_cxx_symbol_exists(getgrgid_r "sys/types.h;grp.h" HAVE_GETGRGID_R)
if(HAVE_GETGRGID_R)
  add_definitions(-DHAVE_GETGRGID_R)
endif()

check_cxx_symbol_exists(uname "sys/utsname.h" HAVE_SYS_UTSNAME_H)
if(HAVE_SYS_UTSNAME_H)
  add_definitions(-DHAVE_SYS_UTSNAME_H)
endif()


cmake_push_check_state()
  set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
  set(CMAKE_REQUIRED_LIBRARIES -pthread)
  set(CMAKE_REQUIRED_FLAGS -pthread)
  check_cxx_symbol_exists(pthread_setaffinity_np "pthread.h" HAVE_PTHREAD_SETAFFINITY_NP)
  if(HAVE_PTHREAD_SETAFFINITY_NP)
    add_definitions(-DHAVE_PTHREAD_SETAFFINITY_NP)
  endif()
cmake_pop_check_state()

cmake_push_check_state()
  set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
  check_cxx_symbol_exists(pipe2 "fcntl.h;unistd.h" HAVE_PIPE2)
  if(HAVE_PIPE2)
    add_definitions(-DHAVE_PIPE2)
  endif()
cmake_pop_check_state()

check_cxx_symbol_exists(sched_getcpu "sched.h" HAVE_SCHED_GETCPU)
if(HAVE_SCHED_GETCPU)
  add_definitions(-DHAVE_SCHED_GETCPU)
endif()

check_cxx_symbol_exists(epoll_create1 "sys/epoll.h" HAVE_EPOLL_CREATE1)
if(HAVE_EPOLL_CREATE1)
  add_definitions(-DHAVE_EPOLL_CREATE1)
endif()

check_cxx_symbol_exists(timerfd_create "sys/timerfd.h" HAVE_TIMERFD_CREATE)
if(HAVE_TIMERFD_CREATE)
  add_definitions(-DHAVE_TIMERFD_CREATE)
endif()

check_cxx_symbol_exists(inotify_init1 "sys/inotify.h" HAVE_INOTIFY_INIT1)
if(HAVE_INOTIFY_INIT1)
  add_definitions(-DHAVE_INOTIFY_INIT1)
endif()

check_cxx_symbol_exists(statvfs "sys/statvfs.h" HAVE_STATVFS)
if(HAVE_STATVFS)
  add_definitions(-DHAVE_STATVFS)
endif()

if(FREETYPE_FOUND)
  include_directories(${FREETYPE_INCLUDE_DIRS})
endif()

if(X11_XShm_FOUND)
  add_definitions(-DHAVE_XSHM_H)
endif()

if(X11_Xcursor_FOUND)
  add_definitions(-DHAVE_XCURSOR_H)
  list(APPEND LIBRARIES ${X11_Xcursor_LIB})
endif()

if(X11_Xft_FOUND AND FREETYPE_FOUND AND FONTCONFIG_FOUND)
  add_definitions(-DHAVE_XFT_H)
  include_directories(${X11_Xft_INCLUDE_PATH})
  list(APPEND LIBRARIES ${X11_Xft_LIB} ${FONTCONFIG_LIBRARIES} ${FREETYPE_LIBRARIES})
endif()

if(X11_Xshape_FOUND)
  add_definitions(-DHAVE_XSHAPE_H)
  list(APPEND LIBRARIES ${X11_Xshape_LIB})
endif()

if(X11_Xrandr_FOUND)
  add_definitions(-DHAVE_XRANDR_H)
  list(APPEND LIBRARIES ${X11_Xrandr_LIB})
endif()

if(X11_Xfixes_FOUND)
  add_definitions(-DHAVE_XFIXES_H)
  list(APPEND LIBRARIES ${X11_Xfixes_LIB})
endif()

if(X11_Xrender_FOUND)
  add_definitions(-DHAVE_XRENDER_H)
  list(APPEND LIBRARIES ${X11_Xrender_LIB})
endif()

# check for xinput2
# find_path(X11_Xi2_INCLUDE_PATH X11/extensions/XInput2.h ${X11_INC_SEARCH_PATH})
# if (X11_Xi2_INCLUDE_PATH AND X11_Xi_LIB)
#  set(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xi2_INCLUDE_PATH})
#  list(REMOVE_DUPLICATES X11_INCLUDE_DIR)
#  add_definitions(-DHAVE_XINPUT2_H)
#  list(APPEND LIBRARIES ${X11_Xi_LIB})
#endif ()

#
# AC_HEADER_DIRENT
# This macro is obsolescent, as all current systems with directory libraries have <dirent.h>.
# New programs need not use this macro.
#
# Not checking for the SCO Xenix dir and x libraries.
#
check_cxx_source_compiles("#include <dirent.h>\nint main(int argc,char *argv[]) { DIR * d; return 0; }" HAVE_DIRENT_H)
if(HAVE_DIRENT_H)
  add_definitions(-DHAVE_DIRENT_H)
else()
  check_cxx_source_compiles("#include <sys/ndir.h>\nint main(int argc,char *argv[]) { DIR * d; return 0; }" HAVE_SYS_NDIR_H)
  if(HAVE_SYS_NDIR_H)
    add_definitions(-DHAVE_SYS_NDIR_H)
  else()
    check_cxx_source_compiles("#include <sys/dir.h>\nint main(int argc,char *argv[]) { DIR * d; return 0; }" HAVE_SYS_DIR_H)
    if(NOT HAVE_SYS_DIR_H)
      add_definitions(-DHAVE_SYS_DIR_H)
    else()
      check_cxx_source_compiles("#include <ndir.h>\nint main(int argc,char *argv[]) { DIR * d; return 0; }" HAVE_NDIR_H)
      if(HAVE_NDIR_H)
        add_definitions(-DHAVE_NDIR_H)
      endif()
    endif()
  endif()
endif()

#
# AC_HEADER_TIME
#
# This macro is obsolescent, as current systems can include both files when they exist.
# New programs need not use this macro.
#
check_include_file_cxx(sys/time.h HAVE_SYS_TIME_H)
if(HAVE_SYS_TIME_H)
  add_definitions(-DHAVE_SYS_TIME_H)
  check_cxx_source_compiles("#include <sys/time.h>\n#include <time.h>\nint main(int arc,char * argv[]) { return 0; }" TIME_WITH_SYS_TIME)
  if(TIME_WITH_SYS_TIME)
    add_definitions(-DTIME_WITH_SYS_TIME)
  endif()
endif()

#
# AC_HEADER_SYS_WAIT
#
# If sys/wait.h is not Posix compatible, then instead of including it, define the Posix macros with their usual interpretations.
# This macro is obsolescent, as current systems are compatible with Posix. New programs need not use this macro.
# New programs need not use this macro.
#
# cmake: only check for header file since FOX doesn't define the alternative macros
check_include_file_cxx(sys/wait.h HAVE_SYS_WAIT_H)
if(HAVE_SYS_WAIT_H)
  add_definitions(-DHAVE_SYS_WAIT_H)
endif()

check_include_file_cxx(semaphore.h HAVE_SEMAPHORE_H)
if(HAVE_SEMAPHORE_H)
  add_definitions(-DHAVE_SEMAPHORE_H)
endif()

check_include_file_cxx(unistd.h HAVE_UNISTD_H)
if(HAVE_UNISTD_H)
  add_definitions(-DHAVE_UNISTD_H)
endif()

check_include_file_cxx(sys/dir.h HAVE_SYS_DIR_H)
if(HAVE_SYS_DIR_H)
  add_definitions(-DHAVE_SYS_DIR_H)
endif()

check_include_file_cxx(sys/filio.h HAVE_SYS_FILIO_H)
if(HAVE_SYS_FILIO_H)
  add_definitions(-DHAVE_SYS_FILIO_H)
endif()

check_include_file_cxx(sys/mman.h HAVE_SYS_MMAN_H)
if(HAVE_SYS_MMAN_H)
  add_definitions(-DHAVE_SYS_MMAN_H)
endif()

check_include_file_cxx(sys/mount.h HAVE_SYS_MOUNT_H)
if(HAVE_SYS_MOUNT_H)
  add_definitions(-DHAVE_SYS_MOUNT_H)
endif()

check_include_file_cxx(sys/param.h HAVE_SYS_PARAM_H)
if(HAVE_SYS_PARAM_H)
  add_definitions(-DHAVE_SYS_PARAM_H)
endif()

check_include_file_cxx(sys/select.h HAVE_SYS_SELECT_H)
if(HAVE_SYS_SELECT_H)
  add_definitions(-DHAVE_SYS_SELECT_H)
endif()

check_include_file_cxx(sys/epoll.h HAVE_SYS_EPOLL_H)
if(HAVE_SYS_EPOLL_H)
  add_definitions(-DHAVE_SYS_EPOLL_H)
endif()

check_include_file_cxx(sys/shm.h HAVE_SYS_SHM_H)
if(HAVE_SYS_SHM_H)
  add_definitions(-DHAVE_SYS_SHM_H)
endif()

check_include_file_cxx(sys/statvfs.h HAVE_SYS_STATVFS_H)
if(HAVE_SYS_STATVFS_H)
  add_definitions(-DHAVE_SYS_STATVFS_H)
endif()

check_include_file_cxx(sys/pstat.h HAVE_SYS_PSTAT_H)
if(HAVE_SYS_PSTAT_H)
  add_definitions(-DHAVE_SYS_PSTAT_H)
endif()

check_include_file_cxx(sys/inotify.h HAVE_SYS_INOTIFY_H)
if(HAVE_SYS_INOTIFY_H)
  add_definitions(-DHAVE_SYS_INOTIFY_H)
endif()


if(WITH_JPEG)
  find_package(JPEG)
  if(JPEG_FOUND)
    list(APPEND LIBRARIES ${JPEG_LIBRARIES})
    add_definitions(-DHAVE_JPEG_H)
  endif()
endif()

if(WITH_PNG)
  find_package(PNG)
  if(TARGET PNG::PNG)
    list(APPEND LIBRARIES PNG::PNG)
    add_definitions(-DHAVE_PNG_H)
  endif()
endif()

if(WITH_ZLIB)
  find_package(ZLIB)
  if(TARGET ZLIB::ZLIB)
    list(APPEND LIBRARIES ZLIB::ZLIB)
    add_definitions(-DHAVE_ZLIB_H)
  endif()
endif()

if(WITH_BZ2LIB)
  find_package(BZip2)
  if(TARGET BZip2::BZip2)
    list(APPEND LIBRARIES BZip2::BZip2)
    add_definitions(-DHAVE_BZ2LIB_H)
  endif()
endif()

if(WITH_TIFF)
  find_package(TIFF)
  if(TARGET TIFF::TIFF)
    list(APPEND LIBRARIES TIFF::TIFF)
    add_definitions(-DHAVE_TIFF_H)
  endif()
endif()

if(WITH_WEBP)
  pkg_check_modules(WEBP libwebp)
  if(WEBP_FOUND)
    list(APPEND LIBRARIES ${WEBP_LIBRARIES})
    add_definitions(-DHAVE_WEBP_H)
  endif()
endif()

if(WITH_OPENJPEG)
  pkg_check_modules(OPENJPEG libopenjpeg1)
  if(OPENJPEG_FOUND)
    list(APPEND LIBRARIES ${OPENJPEG_LIBRARIES})
    add_definitions(-DHAVE_JP2_H)
    include_directories(${OPENJPEG_INCLUDE_DIRS})

  else()
    pkg_check_modules(OPENJPEG libopenjpeg)
    if(OPENJPEG_FOUND)
      list(APPEND LIBRARIES ${OPENJPEG_LIBRARIES})
      add_definitions(-DHAVE_JP2_H)
      include_directories(${OPENJPEG_INCLUDE_DIRS})
    endif()
  endif()
endif()


if(WITH_OPENGL)
  pkg_check_modules(EPOXY REQUIRED epoxy)
  target_compile_definitions(cfox PUBLIC -DHAVE_GL_H)
  list(APPEND LIBRARIES ${EPOXY_LIBRARIES})
  add_definitions(-DHAVE_EPOXY)
endif()

if(NOT WITH_XIM)
  add_definitions(-DNO_XIM)
endif()

target_include_directories(cfox PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(cfox PRIVATE ${X11_LIBRARIES} ${LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons.cpp ${CMAKE_CURRENT_SOURCE_DIR}/icons.h
  COMMAND reswrap -nc -h -n FX -o icons.h ${ICONS}
  COMMAND reswrap -nc -i icons.h -n FX -o icons.cpp ${ICONS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS reswrap
  VERBATIM
  )

set_target_properties(cfox PROPERTIES VERSION ${cfox_VERSION} SOVERSION ${cfox_VERSION_PATCH})
set_target_properties(cfox PROPERTIES OUTPUT_NAME cfox-${cfox_VERSION_MAJOR}.${cfox_VERSION_MINOR})

# don't distribute for cfox
# install(TARGETS cfox
#        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        )
# install(FILES ${HEADERS} DESTINATION include/cfox-${cfox_VERSION_MAJOR}.${cfox_VERSION_MINOR})
