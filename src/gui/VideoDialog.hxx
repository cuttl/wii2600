//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: VideoDialog.hxx,v 1.30 2009-01-24 17:32:29 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef VIDEO_DIALOG_HXX
#define VIDEO_DIALOG_HXX

class CommandSender;
class DialogContainer;
class EditTextWidget;
class PopUpWidget;
class SliderWidget;
class StaticTextWidget;
class CheckboxWidget;

#include "OSystem.hxx"
#include "Dialog.hxx"
#include "bspf.hxx"

class VideoDialog : public Dialog
{
  public:
    VideoDialog(OSystem* osystem, DialogContainer* parent, const GUI::Font& font);
    ~VideoDialog();

  private:
    void loadConfig();
    void saveConfig();
    void setDefaults();

    void handleFullscreenChange(bool enable);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    EditTextWidget*   myRenderer;
    PopUpWidget*      myRendererPopup;
    PopUpWidget*      myTIAFilterPopup;
    PopUpWidget*      myTIAPalettePopup;
    PopUpWidget*      myFSResPopup;
    PopUpWidget*      myFrameTimingPopup;
    PopUpWidget*      myGLFilterPopup;
    SliderWidget*     myNAspectRatioSlider;
    StaticTextWidget* myNAspectRatioLabel;
    SliderWidget*     myPAspectRatioSlider;
    StaticTextWidget* myPAspectRatioLabel;

    SliderWidget*     myFrameRateSlider;
    StaticTextWidget* myFrameRateLabel;
    CheckboxWidget*   myFullscreenCheckbox;
    CheckboxWidget*   myColorLossCheckbox;
    CheckboxWidget*   myGLStretchCheckbox;
    CheckboxWidget*   myUseVSyncCheckbox;
    CheckboxWidget*   myCenterCheckbox;
    CheckboxWidget*   myGrabmouseCheckbox;

    enum {
      kNAspectRatioChanged = 'VDan',
      kPAspectRatioChanged = 'VDap',
      kFrameRateChanged    = 'VDfr',
      kFullScrChanged      = 'VDfs'
    };
};

#endif
