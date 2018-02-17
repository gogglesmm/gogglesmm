/********************************************************************************
*                                                                               *
*                   M a i n   F O X   I n c l u d e   F i l e                   *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
********************************************************************************/
#ifndef FX_H
#define FX_H

// Basic includes
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

// FOX defines
#include "fxver.h"
#include "fxdefs.h"
#include "fxkeys.h"
#include "fxmath.h"
#include "fxendian.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "fxcpuid.h"

// FOX classes
#include "FXhalf.h"
#include "FXException.h"
#include "FXAtomic.h"
//#include "FXAutoPtr.h"
#include "FXRefPtr.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXCallback.h"
#include "FXRandom.h"
#include "FXMutex.h"
#include "FXCondition.h"
#include "FXBarrier.h"
#include "FXSpinLock.h"
#include "FXSemaphore.h"
#include "FXCompletion.h"
#include "FXReadWriteLock.h"
#include "FXAutoThreadStorageKey.h"
#include "FXRunnable.h"
#include "FXThread.h"
#include "FXScopedThread.h"
#include "FXStream.h"
#include "FXPtrList.h"
#include "FXPtrQueue.h"
#include "FXIO.h"
#include "FXIOBuffer.h"
#include "FXIODevice.h"
#include "FXFile.h"
#include "FXPipe.h"
#include "FXSocket.h"
#include "FXMemMap.h"
#include "FXFileStream.h"
#include "FXMemoryStream.h"
#include "FXProcess.h"
#include "FXString.h"
#include "FXVariant.h"
#include "FXVariantArray.h"
#include "FXVariantMap.h"
#include "FXDictionary.h"
#include "FXDLL.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXColors.h"
#include "FXObject.h"
#include "FXDelegator.h"
#include "FXPath.h"
#include "FXSystem.h"
#include "FXStat.h"
#include "FXDir.h"
#include "FXDirVisitor.h"
#include "FXDate.h"
#include "FXURL.h"
#include "FXStringDictionary.h"
#include "FXJSON.h"
#include "FXJSONFile.h"
#include "FXXML.h"
#include "FXXMLFile.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXObjectList.h"
#include "FXAccelTable.h"
#include "FXRecentFiles.h"
#include "FXWorker.h"
#include "FXSemaQueue.h"
#include "FXLFQueue.h"
#include "FXThreadPool.h"
#include "FXCompletion.h"
#include "FXTaskGroup.h"
#include "FXParallel.h"
#include "FXFont.h"
#include "FXCursor.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXId.h"
#include "FXDrawable.h"
#include "FXBitmap.h"
#include "FXImage.h"
#include "FXIcon.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXMessageChannel.h"
#include "FXCURCursor.h"
#include "FXGIFCursor.h"
#include "FXBMPImage.h"
#include "FXGIFImage.h"
#include "FXICOImage.h"
#include "FXIFFImage.h"
#include "FXJPGImage.h"
#include "FXPCXImage.h"
#include "FXPNGImage.h"
#include "FXPPMImage.h"
#include "FXRASImage.h"
#include "FXRGBImage.h"
#include "FXTGAImage.h"
#include "FXTIFImage.h"
#include "FXXBMImage.h"
#include "FXXPMImage.h"
#include "FXDDSImage.h"
#include "FXJP2Image.h"
#include "FXWEBPImage.h"
#include "FXEXEImage.h"
#include "FXBMPIcon.h"
#include "FXGIFIcon.h"
#include "FXICOIcon.h"
#include "FXIFFIcon.h"
#include "FXJPGIcon.h"
#include "FXPCXIcon.h"
#include "FXPNGIcon.h"
#include "FXPPMIcon.h"
#include "FXRASIcon.h"
#include "FXRGBIcon.h"
#include "FXTGAIcon.h"
#include "FXTIFIcon.h"
#include "FXXBMIcon.h"
#include "FXXPMIcon.h"
#include "FXDDSIcon.h"
#include "FXJP2Icon.h"
#include "FXWEBPIcon.h"
#include "FXEXEIcon.h"
#include "FXRegion.h"
#include "FXDC.h"
#include "FXDCWindow.h"
#include "FXDCPrint.h"
#include "FXIconSource.h"
#include "FXIconCache.h"
#include "FXFileAssociations.h"
#include "FXFrame.h"
#include "FXSeparator.h"
#include "FXLabel.h"
#include "FX7Segment.h"
#include "FXDial.h"
#include "FXKnob.h"
#include "FXGauge.h"
#include "FXColorBar.h"
#include "FXColorRing.h"
#include "FXColorWell.h"
#include "FXColorWheel.h"
#include "FXTextField.h"
#include "FXButton.h"
#include "FXPicker.h"
#include "FXToggleButton.h"
#include "FXTriStateButton.h"
#include "FXCheckButton.h"
#include "FXRadioButton.h"
#include "FXArrowButton.h"
#include "FXMenuButton.h"
#include "FXComposite.h"
#include "FXPacker.h"
#include "FXHorizontalFrame.h"
#include "FXVerticalFrame.h"
#include "FXSpring.h"
#include "FXMatrix.h"
#include "FXSpinner.h"
#include "FXRealSpinner.h"
#include "FXRootWindow.h"
#include "FXCanvas.h"
#include "FXGroupBox.h"
#include "FXShell.h"
#include "FXToolTip.h"
#include "FXPopup.h"
#include "FXTopWindow.h"
#include "FXDialogBox.h"
#include "FXMainWindow.h"
#include "FXMenuPane.h"
#include "FXScrollPane.h"
#include "FXMenuCaption.h"
#include "FXMenuSeparator.h"
#include "FXMenuTitle.h"
#include "FXMenuCascade.h"
#include "FXMenuCommand.h"
#include "FXMenuCheck.h"
#include "FXMenuRadio.h"
#include "FXMenuBar.h"
#include "FXOptionMenu.h"
#include "FXSwitcher.h"
#include "FXTabBar.h"
#include "FXTabBook.h"
#include "FXTabItem.h"
#include "FXScrollBar.h"
#include "FXScrollArea.h"
#include "FXScrollWindow.h"
#include "FXList.h"
#include "FXComboBox.h"
#include "FXListBox.h"
#include "FXTreeList.h"
#include "FXTreeListBox.h"
#include "FXFoldingList.h"
#include "FXBitmapView.h"
#include "FXBitmapFrame.h"
#include "FXImageView.h"
#include "FXImageFrame.h"
#include "FXHeader.h"
#include "FXTable.h"
#include "FXDragCorner.h"
#include "FXStatusBar.h"
#include "FXStatusLine.h"
#include "FXChoiceBox.h"
#include "FXMessageBox.h"
#include "FXDirList.h"
#include "FXSlider.h"
#include "FXRealSlider.h"
#include "FXRangeSlider.h"
#include "FXSplitter.h"
#include "FX4Splitter.h"
#include "FXShutter.h"
#include "FXIconList.h"
#include "FXFileList.h"
#include "FXDirBox.h"
#include "FXDriveBox.h"
#include "FXDirSelector.h"
#include "FXDirDialog.h"
#include "FXFileSelector.h"
#include "FXFileDialog.h"
#include "FXColorSelector.h"
#include "FXColorDialog.h"
#include "FXFontSelector.h"
#include "FXFontDialog.h"
#include "FXUndoList.h"
#include "FXRex.h"
#include "FXExpression.h"
#include "FXText.h"
#include "FXDataTarget.h"
#include "FXProgressBar.h"
#include "FXReplaceDialog.h"
#include "FXRuler.h"
#include "FXRulerView.h"
#include "FXSearchDialog.h"
#include "FXInputDialog.h"
#include "FXProgressDialog.h"
#include "FXWizard.h"
#include "FXMDIButton.h"
#include "FXMDIClient.h"
#include "FXMDIChild.h"
#include "FXDocument.h"
#include "FXDockSite.h"
#include "FXDockBar.h"
#include "FXToolBar.h"
#include "FXDockHandler.h"
#include "FXDockTitle.h"
#include "FXToolBarGrip.h"
#include "FXToolBarShell.h"
#include "FXToolBarTab.h"
#include "FXPrintDialog.h"
#include "FXDebugTarget.h"
#include "FXCalendarView.h"
#include "FXCalendar.h"
#include "FXGradientBar.h"
#include "FXConsole.h"
#include "FXSplashWindow.h"


#ifndef FX_NO_GLOBAL_NAMESPACE
using namespace FX;
#endif


#endif
