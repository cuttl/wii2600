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
// Copyright (c) 1995-2013 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef DEBUGGER_DIALOG_HXX
#define DEBUGGER_DIALOG_HXX

class Debugger;
class OSystem;
class DialogContainer;
class ButtonWidget;
class CpuWidget;
class PromptWidget;
class RamWidget;
class RomWidget;
class TabWidget;
class EditTextWidget;
class TiaInfoWidget;
class TiaOutputWidget;
class TiaZoomWidget;
class CartDebugWidget;

#include "Dialog.hxx"
#include "MessageBox.hxx"

class DebuggerDialog : public Dialog
{
  public:
    DebuggerDialog(OSystem* osystem, DialogContainer* parent,
                          int x, int y, int w, int h);
    ~DebuggerDialog();

    PromptWidget& prompt()       { return *myPrompt;       }
    TiaInfoWidget& tiaInfo()     { return *myTiaInfo;      }
    TiaOutputWidget& tiaOutput() { return *myTiaOutput;    }
    TiaZoomWidget& tiaZoom()     { return *myTiaZoom;      }
    RomWidget& rom()             { return *myRom;          }
    CartDebugWidget& cartDebug() { return *myCartDebug;    }
    EditTextWidget& message()    { return *myMessageBox;   }
    ButtonWidget& rewindButton() { return *myRewindButton; }

    void loadConfig();
    void handleKeyDown(StellaKey key, StellaMod mod, char ascii);
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void showFatalMessage(const string& msg);

  private:
    enum {
      kDDStepCmd      = 'DDst',
      kDDTraceCmd     = 'DDtr',
      kDDAdvCmd       = 'DDav',
      kDDSAdvCmd      = 'DDsv',
      kDDRewindCmd    = 'DDrw',
      kDDExitCmd      = 'DDex',
      kDDExitFatalCmd = 'DDer'
    };

    TabWidget *myTab, *myRomTab;

    PromptWidget*    myPrompt;
    TiaInfoWidget*   myTiaInfo;
    TiaOutputWidget* myTiaOutput;
    TiaZoomWidget*   myTiaZoom;
    CpuWidget*       myCpu;
    RamWidget*       myRam;
    RomWidget*       myRom;
    CartDebugWidget* myCartDebug;
    EditTextWidget*  myMessageBox;
    ButtonWidget*    myRewindButton;
    GUI::MessageBox* myFatalError;

  private:
    void addTiaArea();
    void addTabArea();
    void addStatusArea();
    void addRomArea();

    void doStep();
    void doTrace();
    void doScanlineAdvance();
    void doAdvance();
    void doRewind();
    void doExitDebugger();
    void doExitRom();
};

#endif
