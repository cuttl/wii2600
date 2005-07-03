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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Debugger.cxx,v 1.50 2005-07-03 08:15:31 urchlay Exp $
//============================================================================

#include "bspf.hxx"

#include <sstream>

#include "Version.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "DebuggerDialog.hxx"
#include "DebuggerParser.hxx"

#include "Console.hxx"
#include "System.hxx"
#include "M6502.hxx"
#include "D6502.hxx"

#include "Debugger.hxx"
#include "EquateList.hxx"
#include "TIADebug.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::Debugger(OSystem* osystem)
  : DialogContainer(osystem),
    myConsole(NULL),
    mySystem(NULL),
    myParser(NULL),
    myDebugger(NULL),
    equateList(NULL),
    breakPoints(NULL),
    readTraps(NULL),
    writeTraps(NULL),
    myTIAdebug(NULL)
{
  // Init parser
  myParser = new DebuggerParser(this);
  equateList = new EquateList();
  breakPoints = new PackedBitArray(0x10000);
  readTraps = new PackedBitArray(0x10000);
  writeTraps = new PackedBitArray(0x10000);

  oldA = oldX = oldY = oldS = oldP = oldPC = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::~Debugger()
{
  delete myParser;
  delete myDebugger;
  delete myTIAdebug;
  delete equateList;
  delete breakPoints;
  delete readTraps;
  delete writeTraps;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::initialize()
{
  // Calculate the actual pixels required for the # of lines
  // This is currently a bit of a hack, since it uses pixel
  // values that it shouldn't know about (font and tab height, etc)
  int userHeight = myOSystem->settings().getInt("debugheight");
  if(userHeight < kDebuggerLines)
    userHeight = kDebuggerLines;
  userHeight = (userHeight + 3) * kDebuggerLineHeight - 8;

  int x = 0,
      y = myConsole->mediaSource().height(),
      w = kDebuggerWidth,
      h = userHeight;

  delete myBaseDialog;
  DebuggerDialog *dd = new DebuggerDialog(myOSystem, this, x, y, w, h);
  myPrompt = dd->prompt();
  myBaseDialog = dd;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::initializeVideo()
{
  // Calculate the actual pixels required for entire screen
  // This is currently a bit of a hack, since it uses pixel
  // values that it shouldn't know about (font and tab height, etc)
  int userHeight = myOSystem->settings().getInt("debugheight");
  if(userHeight < kDebuggerLines)
    userHeight = kDebuggerLines;
  userHeight = (userHeight + 3) * kDebuggerLineHeight - 8 +
               myConsole->mediaSource().height();

  string title = string("Stella ") + STELLA_VERSION + ": Debugger mode";
  myOSystem->frameBuffer().initialize(title, kDebuggerWidth, userHeight, false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setConsole(Console* console)
{
  assert(console);

  // Keep pointers to these items for efficiency
  myConsole = console;
  mySystem = &(myConsole->system());

  // Create a new TIA debugger for this console
  // This code is somewhat ugly, since we derive a TIA from the MediaSource
  // for no particular reason.  Maybe it's better to make the TIA be the
  // base class and entirely eliminate MediaSource class??
  delete myTIAdebug;
  myTIAdebug = new TIADebug((TIA*)&myConsole->mediaSource());
  myTIAdebug->setDebugger(this);

  // Create a new 6502 debugger for this console
  delete myDebugger;
  myDebugger = new D6502(mySystem);

  autoLoadSymbols(myOSystem->romFile());

  for(int i=0; i<0x80; i++)
    myOldRAM[i] = readRAM(i);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::autoLoadSymbols(string fileName) {
	string file = fileName;

	string::size_type pos;
	if( (pos = file.find_last_of('.')) != string::npos ) {
		file.replace(pos, file.size(), ".sym");
	} else {
		file += ".sym";
	}
	string ret = equateList->loadFile(file);
	//	cerr << "loading syms from file " << file << ": " << ret << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::run(const string& command)
{
  return myParser->run(command);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::valueToString(int value, BaseFormat outputBase)
{
  char rendered[32];

  if(outputBase == kBASE_DEFAULT)
    outputBase = myParser->base();

  switch(outputBase)
  {
    case kBASE_2:
      if(value < 0x100)
        sprintf(rendered, Debugger::to_bin_8(value));
      else
        sprintf(rendered, Debugger::to_bin_16(value));
      break;

    case kBASE_10:
      if(value < 0x100)
        sprintf(rendered, "%3d", value);
      else
        sprintf(rendered, "%5d", value);
      break;

    case kBASE_16:
      default:
        if(value < 0x100)
          sprintf(rendered, Debugger::to_hex_8(value));
        else
          sprintf(rendered, Debugger::to_hex_16(value));
        break;
  }

  return string(rendered);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::invIfChanged(int reg, int oldReg) {
	string ret;

	bool changed = !(reg == oldReg);
	if(changed) ret += "\177";
	ret += valueToString(reg);
	if(changed) ret += "\177";

	return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::state()
{
  string result;
  char buf[255];

  //cerr << "state(): pc is " << myDebugger->pc() << endl;
  result += "\nPC=";
  result += invIfChanged(myDebugger->pc(), oldPC);
  result += " A=";
  result += invIfChanged(myDebugger->a(), oldA);
  result += " X=";
  result += invIfChanged(myDebugger->x(), oldX);
  result += " Y=";
  result += invIfChanged(myDebugger->y(), oldY);
  result += " S=";
  result += invIfChanged(myDebugger->sp(), oldS);
  result += " P=";
  result += invIfChanged(myDebugger->ps(), oldP);
  result += "/";
  formatFlags(myDebugger->ps(), buf);
  result += buf;
  result += "\n  Cyc:";
  sprintf(buf, "%d", mySystem->cycles());
  result += buf;
  result += " Scan:";
  sprintf(buf, "%d", myTIAdebug->scanlines());
  result += buf;
  result += " Frame:";
  sprintf(buf, "%d", myTIAdebug->frameCount());
  result += buf;
  result += " 6502Ins:";
  sprintf(buf, "%d", mySystem->m6502().totalInstructionCount());
  result += buf;
  result += "\n  ";

  result += disassemble(myDebugger->pc(), 1);
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/* The timers, joysticks, and switches can be read via peeks, so
   I didn't write a separate RIOTDebug class. */
const string Debugger::riotState() {
	string ret;

	// TODO: inverse video for changed regs. Core needs to track this.
	// TODO: keyboard controllers?

	for(int i=0x280; i<0x284; i++) {
		ret += valueToString(i);
		ret += "/";
		ret += equates()->getFormatted(i, 2);
		ret += "=";
		ret += valueToString(mySystem->peek(i));
		ret += " ";
	}
	ret += "\n";

	// These are squirrely: some symbol files will define these as
	// 0x284-0x287. Doesn't actually matter, these registers repeat
   // every 16 bytes.
	ret += valueToString(0x294);
	ret += "/TIM1T=";
	ret += valueToString(mySystem->peek(0x294));
	ret += " ";

	ret += valueToString(0x295);
	ret += "/TIM8T=";
	ret += valueToString(mySystem->peek(0x295));
	ret += " ";

	ret += valueToString(0x296);
	ret += "/TIM64T=";
	ret += valueToString(mySystem->peek(0x296));
	ret += " ";

	ret += valueToString(0x297);
	ret += "/TIM1024T=";
	ret += valueToString(mySystem->peek(0x297));
	ret += "\n";

	ret += "Left/P0diff: ";
	ret += (mySystem->peek(0x282) & 0x40) ? "hard/A" : "easy/B";
	ret += "   ";

	ret += "Right/P1diff: ";
	ret += (mySystem->peek(0x282) & 0x80) ? "hard/A" : "easy/B";
	ret += "\n";

	ret += "TVType: ";
	ret += (mySystem->peek(0x282) & 0x8) ? "Color" : "B&W";
	ret += "   Switches: ";
	ret += (mySystem->peek(0x282) & 0x2) ? "-" : "+";
	ret += "select  ";
	ret += (mySystem->peek(0x282) & 0x1) ? "-" : "+";
	ret += "reset";
	ret += "\n";

	// Yes, the fire buttons are in the TIA, but we might as well
	// show them here for convenience.
	ret += "Left/P0 stick:  ";
	ret += (mySystem->peek(0x280) & 0x80) ? "" : "right ";
	ret += (mySystem->peek(0x280) & 0x40) ? "" : "left ";
	ret += (mySystem->peek(0x280) & 0x20) ? "" : "down ";
	ret += (mySystem->peek(0x280) & 0x10) ? "" : "up ";
	ret += ((mySystem->peek(0x280) & 0xf0) == 0xf0) ? "(no directions) " : "";
	ret += (mySystem->peek(0x03c) & 0x80) ? "" : "(button) ";
	ret += "\n";
	ret += "Right/P1 stick: ";
	ret += (mySystem->peek(0x280) & 0x08) ? "" : "right ";
	ret += (mySystem->peek(0x280) & 0x04) ? "" : "left ";
	ret += (mySystem->peek(0x280) & 0x02) ? "" : "down ";
	ret += (mySystem->peek(0x280) & 0x01) ? "" : "up ";
	ret += ((mySystem->peek(0x280) & 0x0f) == 0x0f) ? "(no directions) " : "";
	ret += (mySystem->peek(0x03d) & 0x80) ? "" : "(button) ";

	//ret += "\n"; // caller will add

	return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::reset() {
	int pc = myDebugger->dPeek(0xfffc);
	myDebugger->pc(pc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::formatFlags(int f, char *out) {
	// NV-BDIZC

	if(f & 128)
		out[0] = 'N';
	else
		out[0] = 'n';

	if(f & 64)
		out[1] = 'V';
	else
		out[1] = 'v';

	out[2] = '-';

	if(f & 16)
		out[3] = 'B';
	else
		out[3] = 'b';

	if(f & 8)
		out[4] = 'D';
	else
		out[4] = 'd';

	if(f & 4)
		out[5] = 'I';
	else
		out[5] = 'i';

	if(f & 2)
		out[6] = 'Z';
	else
		out[6] = 'z';

	if(f & 1)
		out[7] = 'C';
	else
		out[7] = 'c';

	out[8] = '\0';
}

/* Danger: readRAM() and writeRAM() take an *offset* into RAM, *not* an
   actual address. This means you don't get to use these to read/write
   outside of the RIOT RAM. It also means that e.g. to read location 0x80,
   you pass 0 (because 0x80 is the 0th byte of RAM).

   However, setRAM() actually uses addresses, not offsets. This means that
   setRAM() can poke anywhere in the address space. However, it still can't
   change ROM: you use patchROM() for that. setRAM() *can* trigger a bank
   switch, if you poke to the "hot spot" for the cartridge.
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Debugger::readRAM(uInt16 offset)
{
  offset &= 0x7f; // there are only 128 bytes
  return mySystem->peek(offset + kRamStart);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::writeRAM(uInt16 offset, uInt8 value)
{
  offset &= 0x7f; // there are only 128 bytes
  mySystem->poke(offset + kRamStart, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/* Element 0 of args is the address. The remaining elements are the data
   to poke, starting at the given address.
*/
const string Debugger::setRAM(IntArray args) {
  char buf[10];

  int count = args.size();
  int address = args[0];
  for(int i=1; i<count; i++)
    mySystem->poke(address++, args[i]);

  string ret = "changed ";
  sprintf(buf, "%d", count-1);
  ret += buf;
  ret += " location";
  if(count != 0)
    ret += "s";
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Debugger::oldRAM(uInt8 offset)
{
  offset &= 0x7f;
  return myOldRAM[offset];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::ramChanged(uInt8 offset)
{
  offset &= 0x7f;
  return (myOldRAM[offset] != readRAM(offset));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/* Warning: this method really is for dumping *RAM*, not ROM or I/O! */
const string Debugger::dumpRAM(uInt8 start, uInt8 len)
{
  string result;
  char buf[128];
  int bytesPerLine;

  switch(myParser->base())
  {
    case kBASE_16:
    case kBASE_10:
      bytesPerLine = 0x10;
      break;

    case kBASE_2:
      bytesPerLine = 0x04;
      break;

    case kBASE_DEFAULT:
    default:
      return DebuggerParser::red("invalid base, this is a BUG");
  }

  for (uInt8 i = 0x00; i < len; i += bytesPerLine)
  {
    sprintf(buf, "%.2x: ", start+i);
    result += buf;

    for (uInt8 j = 0; j < bytesPerLine; j++)
    {
      int byte = mySystem->peek(start+i+j);

      result += invIfChanged(byte, myOldRAM[i+j]);
      result += " ";

      if(j == 0x07) result += " ";
    }
    result += "\n";
  }

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::dumpTIA()
{
  string result;
  char buf[128];

  sprintf(buf, "%.2x: ", 0);
  result += buf;
  for (uInt8 j = 0; j < 0x010; j++)
  {
    sprintf(buf, "%.2x ", mySystem->peek(j));
    result += buf;

    if(j == 0x07) result += "- ";
  }

  result += "\n";
  result += myTIAdebug->state();

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Start the debugger if we aren't already in it.
// Returns false if we were already in the debugger.
bool Debugger::start()
{
  if(myOSystem->eventHandler().state() != EventHandler::S_DEBUGGER) {
    myOSystem->eventHandler().enterDebugMode();
    myPrompt->printPrompt();
    return true;
  } else {
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::quit()
{
  if(myOSystem->eventHandler().state() == EventHandler::S_DEBUGGER) {
    // execute one instruction on quit, IF we're
    // sitting at a breakpoint. This will get us past it.
    // Somehow this feels like a hack to me, but I don't know why
    if(breakPoints->isSet(myDebugger->pc()))
      mySystem->m6502().execute(1);
    myOSystem->eventHandler().leaveDebugMode();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::saveState(int state)
{
  myOSystem->eventHandler().saveState(state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::loadState(int state)
{
  myOSystem->eventHandler().loadState(state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::step()
{
  saveRegs();

  int cyc = mySystem->cycles();
  mySystem->m6502().execute(1);
  myTIAdebug->updateTIA();
  myOSystem->frameBuffer().refreshTIA(true);
  return mySystem->cycles() - cyc;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// trace is just like step, except it treats a subroutine call as one
// instruction.

// This implementation is not perfect: it just watches the program counter,
// instead of tracking (possibly) nested JSR/RTS pairs. In particular, it
// will fail for recursive subroutine calls. However, with 128 bytes of RAM
// to share between stack and variables, I doubt any 2600 games will ever
// use recursion...

// FIXME: TIA framebuffer should be updated during tracing!

int Debugger::trace()
{
  // 32 is the 6502 JSR instruction:
  if(mySystem->peek(myDebugger->pc()) == 32) {
    saveRegs();

    int cyc = mySystem->cycles();
    int targetPC = myDebugger->pc() + 3; // return address
    while(myDebugger->pc() != targetPC)
      mySystem->m6502().execute(1);
    myTIAdebug->updateTIA();
    myOSystem->frameBuffer().refreshTIA(true);
    return mySystem->cycles() - cyc;
  } else {
    return step();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setA(int a) {
  myDebugger->a(a);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setX(int x) {
  myDebugger->x(x);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setY(int y) {
  myDebugger->y(y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setSP(int sp) {
  myDebugger->sp(sp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setPC(int pc) {
  myDebugger->pc(pc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // NV-BDIZC
void Debugger::toggleC() {
  myDebugger->ps( myDebugger->ps() ^ 0x01 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleZ() {
  myDebugger->ps( myDebugger->ps() ^ 0x02 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleI() {
  myDebugger->ps( myDebugger->ps() ^ 0x04 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleD() {
  myDebugger->ps( myDebugger->ps() ^ 0x08 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleB() {
  myDebugger->ps( myDebugger->ps() ^ 0x10 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleV() {
  myDebugger->ps( myDebugger->ps() ^ 0x40 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleN() {
  myDebugger->ps( myDebugger->ps() ^ 0x80 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // NV-BDIZC
void Debugger::setC(bool value) {
	myDebugger->ps( set_bit(myDebugger->ps(), 0, value) );
  //	myDebugger->ps( myDebugger->ps() ^ 0x01 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setZ(bool value) {
	myDebugger->ps( set_bit(myDebugger->ps(), 1, value) );
  //	myDebugger->ps( myDebugger->ps() ^ 0x02 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setI(bool value) {
	myDebugger->ps( set_bit(myDebugger->ps(), 2, value) );
  //	myDebugger->ps( myDebugger->ps() ^ 0x04 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setD(bool value) {
	myDebugger->ps( set_bit(myDebugger->ps(), 3, value) );
  //	myDebugger->ps( myDebugger->ps() ^ 0x08 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setB(bool value) {
	myDebugger->ps( set_bit(myDebugger->ps(), 4, value) );
  //	myDebugger->ps( myDebugger->ps() ^ 0x10 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setV(bool value) {
	myDebugger->ps( set_bit(myDebugger->ps(), 6, value) );
  //	myDebugger->ps( myDebugger->ps() ^ 0x40 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setN(bool value) {
	myDebugger->ps( set_bit(myDebugger->ps(), 7, value) );
  //	myDebugger->ps( myDebugger->ps() ^ 0x80 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EquateList *Debugger::equates() {
  return equateList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleBreakPoint(int bp) {
  mySystem->m6502().setBreakPoints(breakPoints);
  if(bp < 0) bp = myDebugger->pc();
  breakPoints->toggle(bp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::breakPoint(int bp) {
  if(bp < 0) bp = myDebugger->pc();
  return breakPoints->isSet(bp) != 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleReadTrap(int t) {
  mySystem->m6502().setTraps(readTraps, writeTraps);
  readTraps->toggle(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleWriteTrap(int t) {
  mySystem->m6502().setTraps(readTraps, writeTraps);
  writeTraps->toggle(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleTrap(int t) {
  toggleReadTrap(t);
  toggleWriteTrap(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::readTrap(int t) {
  return readTraps->isSet(t) != 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::writeTrap(int t) {
  return writeTraps->isSet(t) != 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::getPC() {
  return myDebugger->pc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::getA() {
  return myDebugger->a();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::getX() {
  return myDebugger->x();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::getY() {
  return myDebugger->y();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::getSP() {
  return myDebugger->sp();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::getPS() {
  return myDebugger->ps();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::cycles() {
  return mySystem->cycles();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::disassemble(int start, int lines) {
  char buf[255], bbuf[255];
  string result;

  do {
    char *label = equateList->getFormatted(start, 4);

    result += label;
    result += ": ";

    int count = myDebugger->disassemble(start, buf, equateList);

    for(int i=0; i<count; i++) {
      sprintf(bbuf, "%02x ", peek(start++));
      result += bbuf;
    }

    if(count < 3) result += "   ";
    if(count < 2) result += "   ";

    result += " ";
    result += buf;
	 result += "\n";
  } while(--lines > 0);

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::nextFrame(int frames) {
  saveRegs();
  myOSystem->frameBuffer().advance(frames);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::clearAllBreakPoints() {
  delete breakPoints;
  breakPoints = new PackedBitArray(0x10000);
  mySystem->m6502().setBreakPoints(NULL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::clearAllTraps() {
  delete readTraps;
  delete writeTraps;
  readTraps = new PackedBitArray(0x10000);
  writeTraps = new PackedBitArray(0x10000);
  mySystem->m6502().setTraps(NULL, NULL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::peek(int addr) {
  return mySystem->peek(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::dpeek(int addr) {
  return mySystem->peek(addr) | (mySystem->peek(addr+1) << 8);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::setHeight(int height)
{
  myOSystem->settings().getInt("debugheight");

  if(height == 0)
    height = kDebuggerLines;

  if(height < kDebuggerLines)
    return false;

  myOSystem->settings().setInt("debugheight", height);

  // Restart the debugger subsystem
  myOSystem->resetDebugger();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::showWatches() {
	return myParser->showWatches();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::addLabel(string label, int address) {
	equateList->addEquate(label, address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::reloadROM() {
  myOSystem->createConsole( myOSystem->romFile() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::setBank(int bank) {
  if(myConsole->cartridge().bankCount() > 1) {
    myConsole->cartridge().bank(bank);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::getBank() {
  return myConsole->cartridge().bank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::bankCount() {
  return myConsole->cartridge().bankCount();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char *Debugger::getCartType() {
  return myConsole->cartridge().name();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::patchROM(int addr, int value) {
  return myConsole->cartridge().patch(addr, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::saveRegs() {
	for(int i=0; i<0x80; i++) {
		myOldRAM[i] = readRAM(i);
	}
	oldA = getA();
	oldX = getX();
	oldY = getY();
	oldS = getSP();
	oldP = getPS();
	oldPC = getPC();
}
