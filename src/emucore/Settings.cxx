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
// $Id$
//============================================================================

#include <cassert>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "bspf.hxx"

#include "OSystem.hxx"
#include "Version.hxx"

#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings(OSystem* osystem)
  : myOSystem(osystem)
{
  // Add this settings object to the OSystem
  myOSystem->attach(this);

  // Add options that are common to all versions of Stella
  setInternal("video", "soft");

  // OpenGL specific options
  setInternal("gl_filter", "nearest");
  setInternal("gl_aspectn", "100");
  setInternal("gl_aspectp", "100");
  setInternal("gl_fsmax", "false");
  setInternal("gl_lib", "libGL.so");
  setInternal("gl_vsync", "false");
  setInternal("gl_texrect", "false");

  // Framebuffer-related options
  setInternal("tia_filter", "zoom2x");
#ifdef WII
  setInternal("fullscreen", "true");
#else
  setInternal("fullscreen", "false");
#endif
  setInternal("fullres", "auto");
  setInternal("center", "true");
  setInternal("grabmouse", "false");
  setInternal("palette", "standard");
  setInternal("colorloss", "false");
  setInternal("timing", "sleep");

  // Sound options
  setInternal("sound", "true");
  setInternal("fragsize", "512");
#ifdef WII
  setInternal("freq", "32000");
  setInternal("tiafreq", "32000");
#else
  setInternal("freq", "31400");
  setInternal("tiafreq", "31400");
#endif  
  setInternal("volume", "100");
  setInternal("clipvol", "true");

  // Input event options
  setInternal("keymap", "");
  setInternal("joymap", "");
  setInternal("joyaxismap", "");
  setInternal("joyhatmap", "");
  setInternal("joydeadzone", "0");
  setInternal("pspeed", "6");
  setInternal("sa1", "left");
  setInternal("sa2", "right");

  // Snapshot options
  setInternal("ssdir", "");
  setInternal("sssingle", "false");
  setInternal("ss1x", "false");

  // Config files and paths
#ifdef WII
  setInternal("romdir", "");
#else
  setInternal("romdir", "~");
#endif
  setInternal("statedir", "");
  setInternal("cheatfile", "");
  setInternal("palettefile", "");
  setInternal("propsfile", "");
  setInternal("eepromdir", "");

  // ROM browser options
  setInternal("launcherres", "640x480");
  setInternal("launcherfont", "medium");
  setInternal("launcherexts", "allfiles");
  setInternal("romviewer", "0");
  setInternal("lastrom", "");

  // UI-related options
  setInternal("debuggerres", "1030x690");
  setInternal("uipalette", "0");
  setInternal("listdelay", "300");
  setInternal("mwheel", "4");

  // Misc options
  setInternal("autoslot", "false");
  setInternal("showinfo", "false");
  setInternal("tiafloat", "true");
  setInternal("avoxport", "");
  setInternal("stats", "false");
  setInternal("audiofirst", "true");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::~Settings()
{
  myInternalSettings.clear();
  myExternalSettings.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::loadConfig()
{
  string line, key, value;
  string::size_type equalPos, garbage;

  ifstream in(myOSystem->configFile().c_str());
  if(!in || !in.is_open())
  {
    cout << "ERROR: Couldn't load settings file\n";
    return;
  }

  while(getline(in, line))
  {
    // Strip all whitespace and tabs from the line
    while((garbage = line.find("\t")) != string::npos)
      line.erase(garbage, 1);

    // Ignore commented and empty lines
    if((line.length() == 0) || (line[0] == ';'))
      continue;

    // Search for the equal sign and discard the line if its not found
    if((equalPos = line.find("=")) == string::npos)
      continue;

    // Split the line into key/value pairs and trim any whitespace
    key   = line.substr(0, equalPos);
    value = line.substr(equalPos + 1, line.length() - key.length() - 1);
    key   = trim(key);
    value = trim(value);

    // Check for absent key or value
    if((key.length() == 0) || (value.length() == 0))
      continue;

    // Only settings which have been previously set are valid
    if(int idx = getInternalPos(key) != -1)
      setInternal(key, value, idx, true);
  }

  in.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Settings::loadCommandLine(int argc, char** argv)
{
  for(int i = 1; i < argc; ++i)
  {
    // strip off the '-' character
    string key = argv[i];
    if(key[0] == '-')
    {
      key = key.substr(1, key.length());

      // Take care of the arguments which are meant to be executed immediately
      // (and then Stella should exit)
      if(key == "help" || key == "listrominfo")
      {
        setExternal(key, "true");
        return "";
      }

      // Take care of arguments without an option
      if(key == "rominfo" || key == "debug" || key == "holdreset" ||
         key == "holdselect" || key == "holdbutton0" || key == "takesnapshot")
      {
        setExternal(key, "true");
        continue;
      }

      if(++i >= argc)
      {
        cerr << "Missing argument for '" << key << "'" << endl;
        return "";
      }
      string value = argv[i];

      // Settings read from the commandline must not be saved to 
      // the rc-file, unless they were previously set
      if(int idx = getInternalPos(key) != -1)
        setInternal(key, value, idx);   // don't set initialValue here
      else
        setExternal(key, value);
    }
    else
      return key;
  }

  return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::validate()
{
  string s;
  int i;

  s = getString("video");
  if(s != "soft" && s != "gl")
    setInternal("video", "soft");

  s = getString("timing");
  if(s != "sleep" && s != "busy")
    setInternal("timing", "sleep");

#ifdef DISPLAY_OPENGL
  s = getString("gl_filter");
  if(s != "linear" && s != "nearest")
    setInternal("gl_filter", "nearest");

  i = getInt("gl_aspectn");
  if(i < 80 || i > 120)
    setInternal("gl_aspectn", "100");

  i = getInt("gl_aspectp");
  if(i < 80 || i > 120)
    setInternal("gl_aspectp", "100");
#endif

#ifdef SOUND_SUPPORT
  i = getInt("volume");
  if(i < 0 || i > 100)
    setInternal("volume", "100");
  i = getInt("freq");
  if(i < 0 || i > 48000)
    setInternal("freq", "31400");
  i = getInt("tiafreq");
  if(i < 0 || i > 48000)
    setInternal("tiafreq", "31400");
#endif

  i = getInt("joydeadzone");
  if(i < 0)
    setInternal("joydeadzone", "0");
  else if(i > 29)
    setInternal("joydeadzone", "29");

  i = getInt("pspeed");
  if(i < 1)
    setInternal("pspeed", "1");
  else if(i > 15)
    setInternal("pspeed", "15");

  s = getString("palette");
  if(s != "standard" && s != "z26" && s != "user")
    setInternal("palette", "standard");

  s = getString("launcherfont");
  if(s != "small" && s != "medium" && s != "large")
    setInternal("launcherfont", "medium");

  i = getInt("romviewer");
  if(i < 0)
    setInternal("romviewer", "0");
  else if(i > 2)
    setInternal("romviewer", "2");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::usage()
{
#ifndef MAC_OSX
  cout << endl
    << "Stella version " << STELLA_VERSION << endl
    << endl
    << "Usage: stella [options ...] romfile" << endl
    << "       Run without any options or romfile to use the ROM launcher" << endl
    << "       Consult the manual for more in-depth information" << endl
    << endl
    << "Valid options are:" << endl
    << endl
    << "  -video        <type>         Type is one of the following:\n"
    << "                 soft            SDL software mode\n"
  #ifdef DISPLAY_OPENGL
    << "                 gl              SDL OpenGL mode\n"
    << endl
    << "  -gl_lib       <name>         Specify the OpenGL library\n"
    << "  -gl_filter    <type>         Type is one of the following:\n"
    << "                 nearest         Normal scaling (GL_NEAREST)\n"
    << "                 linear          Blurred scaling (GL_LINEAR)\n"
    << "  -gl_aspectn   <number>       Scale the TIA width by the given percentage in NTSC mode\n"
    << "  -gl_aspectp   <number>       Scale the TIA width by the given percentage in PAL mode\n"
    << "  -gl_fsmax     <1|0>          Stretch GL image in fullscreen emulation mode\n"
    << "  -gl_vsync     <1|0>          Enable synchronize to vertical blank interrupt\n"
    << "  -gl_texrect   <1|0>          Enable GL_TEXTURE_RECTANGLE extension\n"
//    << "  -gl_accel     <1|0>          Enable SDL_GL_ACCELERATED_VISUAL\n"
    << endl
  #endif
    << "  -tia_filter   <filter>       Use the specified filter in emulation mode\n"
    << "  -fullscreen   <1|0>          Play the game in fullscreen mode\n"
    << "  -fullres      <auto|WxH>     The resolution to use in fullscreen mode\n"
    << "  -center       <1|0>          Centers game window (if possible)\n"
    << "  -grabmouse    <1|0>          Keeps the mouse in the game window\n"
    << "  -palette      <standard|     Use the specified color palette\n"
    << "                 z26|\n"
    << "                 user>\n"
    << "  -colorloss    <1|0>          Enable PAL color-loss effect\n"
    << "  -framerate    <number>       Display the given number of frames per second (0 to auto-calculate)\n"
    << "  -timing       <sleep|busy>   Use the given type of wait between frames\n"
    << endl
  #ifdef SOUND_SUPPORT
    << "  -sound        <1|0>          Enable sound generation\n"
    << "  -fragsize     <number>       The size of sound fragments (must be a power of two)\n"
    << "  -freq         <number>       Set sound sample output frequency (0 - 48000)\n"
    << "  -tiafreq      <number>       Set sound sample generation frequency (0 - 48000)\n"
    << "  -volume       <number>       Set the volume (0 - 100)\n"
    << "  -clipvol      <1|0>          Enable volume clipping (eliminates popping)\n"
    << endl
  #endif
    << "  -cheat        <code>         Use the specified cheatcode (see manual for description)\n"
    << "  -showinfo     <1|0>          Shows some game info on commandline\n"
    << "  -joydeadzone  <number>       Sets 'deadzone' area for analog joysticks (0-29)\n"
    << "  -pspeed       <number>       Speed of digital emulated paddle movement (1-15)\n"
    << "  -sa1          <left|right>   Stelladaptor 1 emulates specified joystick port\n"
    << "  -sa2          <left|right>   Stelladaptor 2 emulates specified joystick port\n"
    << "  -autoslot     <1|0>          Automatically switch to next save slot when state saving\n"
    << "  -audiofirst   <1|0>          Initial audio before video (required for some ATI video cards)\n"
    << "  -ssdir        <path>         The directory to save snapshot files to\n"
    << "  -sssingle     <1|0>          Generate single snapshot instead of many\n"
    << "  -ss1x         <1|0>          Generate TIA snapshot in 1x mode (ignore scaling)\n"
    << endl
    << "  -rominfo      <rom>          Display detailed information for the given ROM\n"
    << "  -listrominfo                 Display contents of stella.pro, one line per ROM entry\n"
    << "  -launcherres  <WxH>          The resolution to use in ROM launcher mode\n"
    << "  -launcherfont <small|medium| Use the specified font in the ROM launcher\n"
    << "                 large>\n"
    << "  -launcherexts <allfiles|     Show files with the given extensions in ROM launcher\n"
    << "                 allroms|        (exts is a ':' separated list of extensions\n"
    << "                 exts\n"
    << "  -romviewer    <0|1|2>        Show ROM info viewer at given zoom level in ROM launcher (0 for off)\n"
    << "  -uipalette    <1|2>          Used the specified palette for UI elements\n"
    << "  -listdelay    <delay>        Time to wait between keypresses in list widgets (300-1000)\n"
    << "  -mwheel       <lines>        Number of lines the mouse wheel will scroll in UI\n"
    << "  -statedir     <dir>          Directory in which to save state files\n"
    << "  -cheatfile    <file>         Full pathname of cheatfile database\n"
    << "  -palettefile  <file>         Full pathname of user-defined palette file\n"
    << "  -propsfile    <file>         Full pathname of ROM properties file\n"
    << "  -eepromdir    <dir>          Directory in which to save EEPROM files\n"
    << "  -avoxport     <name>         The name of the serial port where an AtariVox is connected\n"
    << "  -help                        Show the text you're now reading\n"
  #ifdef DEBUGGER_SUPPORT
    << endl
    << " The following options are meant for developers\n"
    << " Arguments are more fully explained in the manual\n"
    << endl
    << "   -debuggerres  <WxH>         The resolution to use in debugger mode\n"
    << "   -break        <address>     Set a breakpoint at 'address'\n"
    << "   -debug                      Start in debugger mode\n"
    << "   -holdreset                  Start the emulator with the Game Reset switch held down\n"
    << "   -holdselect                 Start the emulator with the Game Select switch held down\n"
    << "   -holdbutton0                Start the emulator with the left joystick button held down\n"
    << "   -stats        <1|0>         Overlay console info during emulation\n"
    << "   -tiafloat     <1|0>         Set unused TIA pins floating on a read/peek\n"
    << endl
    << "   -bs          <arg>          Sets the 'Cartridge.Type' (bankswitch) property\n"
    << "   -type        <arg>          Same as using -bs\n"
    << "   -channels    <arg>          Sets the 'Cartridge.Sound' property\n"
    << "   -ld          <arg>          Sets the 'Console.LeftDifficulty' property\n"
    << "   -rd          <arg>          Sets the 'Console.RightDifficulty' property\n"
    << "   -tv          <arg>          Sets the 'Console.TelevisionType' property\n"
    << "   -sp          <arg>          Sets the 'Console.SwapPorts' property\n"
    << "   -lc          <arg>          Sets the 'Controller.Left' property\n"
    << "   -rc          <arg>          Sets the 'Controller.Right' property\n"
    << "   -bc          <arg>          Same as using both -lc and -rc\n"
    << "   -cp          <arg>          Sets the 'Controller.SwapPaddles' property\n"
    << "   -format      <arg>          Sets the 'Display.Format' property\n"
    << "   -ystart      <arg>          Sets the 'Display.YStart' property\n"
    << "   -height      <arg>          Sets the 'Display.Height' property\n"
    << "   -pp          <arg>          Sets the 'Display.Phosphor' property\n"
    << "   -ppblend     <arg>          Sets the 'Display.PPBlend' property\n"
  #endif
    << endl;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::saveConfig()
{
  // Do a quick scan of the internal settings to see if any have
  // changed.  If not, we don't need to save them at all.
  bool settingsChanged = false;
  for(unsigned int i = 0; i < myInternalSettings.size(); ++i)
  {
    if(myInternalSettings[i].value != myInternalSettings[i].initialValue)
    {
      settingsChanged = true;
      break;
    }
  }

  if(!settingsChanged)
    return;

  ofstream out(myOSystem->configFile().c_str());
  if(!out || !out.is_open())
  {
    cout << "ERROR: Couldn't save settings file\n";
    return;
  }

  out << ";  Stella configuration file" << endl
      << ";" << endl
      << ";  Lines starting with ';' are comments and are ignored." << endl
      << ";  Spaces and tabs are ignored." << endl
      << ";" << endl
      << ";  Format MUST be as follows:" << endl
      << ";    command = value" << endl
      << ";" << endl
      << ";  Commmands are the same as those specified on the commandline," << endl
      << ";  without the '-' character." << endl
      << ";" << endl
      << ";  Values are the same as those allowed on the commandline." << endl
      << ";  Boolean values are specified as 1 (or true) and 0 (or false)" << endl
      << ";" << endl;

  // Write out each of the key and value pairs
  for(unsigned int i = 0; i < myInternalSettings.size(); ++i)
  {
    out << myInternalSettings[i].key << " = " <<
           myInternalSettings[i].value << endl;
  }

  out.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setInt(const string& key, const int value)
{
  ostringstream stream;
  stream << value;

  if(int idx = getInternalPos(key) != -1)
    setInternal(key, stream.str(), idx);
  else
    setExternal(key, stream.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setFloat(const string& key, const float value)
{
  ostringstream stream;
  stream << value;

  if(int idx = getInternalPos(key) != -1)
    setInternal(key, stream.str(), idx);
  else
    setExternal(key, stream.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setBool(const string& key, const bool value)
{
  ostringstream stream;
  stream << value;

  if(int idx = getInternalPos(key) != -1)
    setInternal(key, stream.str(), idx);
  else
    setExternal(key, stream.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setString(const string& key, const string& value)
{
  if(int idx = getInternalPos(key) != -1)
    setInternal(key, value, idx);
  else
    setExternal(key, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::getSize(const string& key, int& x, int& y) const
{
  char c;
  string size = getString(key);
  istringstream buf(size);
  buf >> x >> c >> y;
  if(c != 'x')
    x = y = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::getInt(const string& key) const
{
  // Try to find the named setting and answer its value
  int idx = -1;
  if((idx = getInternalPos(key)) != -1)
    return (int) atoi(myInternalSettings[idx].value.c_str());
  else if((idx = getExternalPos(key)) != -1)
    return (int) atoi(myExternalSettings[idx].value.c_str());
  else
    return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float Settings::getFloat(const string& key) const
{
  // Try to find the named setting and answer its value
  int idx = -1;
  if((idx = getInternalPos(key)) != -1)
    return (float) atof(myInternalSettings[idx].value.c_str());
  else if((idx = getExternalPos(key)) != -1)
    return (float) atof(myExternalSettings[idx].value.c_str());
  else
    return -1.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Settings::getBool(const string& key) const
{
  // Try to find the named setting and answer its value
  int idx = -1;
  if((idx = getInternalPos(key)) != -1)
  {
    const string& value = myInternalSettings[idx].value;
    if(value == "1" || value == "true")
      return true;
    else if(value == "0" || value == "false")
      return false;
    else
      return false;
  }
  else if((idx = getExternalPos(key)) != -1)
  {
    const string& value = myExternalSettings[idx].value;
    if(value == "1" || value == "true")
      return true;
    else if(value == "0" || value == "false")
      return false;
    else
      return false;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& Settings::getString(const string& key) const
{
  // Try to find the named setting and answer its value
  int idx = -1;
  if((idx = getInternalPos(key)) != -1)
    return myInternalSettings[idx].value;
  else if((idx = getExternalPos(key)) != -1)
    return myExternalSettings[idx].value;
  else
    return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setSize(const string& key, const int value1, const int value2)
{
  ostringstream buf;
  buf << value1 << "x" << value2;
  setString(key, buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::getInternalPos(const string& key) const
{
  for(unsigned int i = 0; i < myInternalSettings.size(); ++i)
    if(myInternalSettings[i].key == key)
      return i;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::getExternalPos(const string& key) const
{
  for(unsigned int i = 0; i < myExternalSettings.size(); ++i)
    if(myExternalSettings[i].key == key)
      return i;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::setInternal(const string& key, const string& value,
                          int pos, bool useAsInitial)
{
  int idx = -1;

  if(pos != -1 && pos >= 0 && pos < (int)myInternalSettings.size() &&
     myInternalSettings[pos].key == key)
  {
    idx = pos;
  }
  else
  {
    for(unsigned int i = 0; i < myInternalSettings.size(); ++i)
    {
      if(myInternalSettings[i].key == key)
      {
        idx = i;
        break;
      }
    }
  }

  if(idx != -1)
  {
    myInternalSettings[idx].key   = key;
    myInternalSettings[idx].value = value;
    if(useAsInitial) myInternalSettings[idx].initialValue = value;

    /*cerr << "modify internal: key = " << key
         << ", value  = " << value
         << ", ivalue = " << myInternalSettings[idx].initialValue
         << " @ index = " << idx
         << endl;*/
  }
  else
  {
    Setting setting;
    setting.key   = key;
    setting.value = value;
    if(useAsInitial) setting.initialValue = value;

    myInternalSettings.push_back(setting);
    idx = myInternalSettings.size() - 1;

    /*cerr << "insert internal: key = " << key
         << ", value  = " << value
         << ", ivalue = " << setting.initialValue
         << " @ index = " << idx
         << endl;*/
  }

  return idx;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::setExternal(const string& key, const string& value,
                          int pos, bool useAsInitial)
{
  int idx = -1;

  if(pos != -1 && pos >= 0 && pos < (int)myExternalSettings.size() &&
     myExternalSettings[pos].key == key)
  {
    idx = pos;
  }
  else
  {
    for(unsigned int i = 0; i < myExternalSettings.size(); ++i)
    {
      if(myExternalSettings[i].key == key)
      {
        idx = i;
        break;
      }
    }
  }

  if(idx != -1)
  {
    myExternalSettings[idx].key   = key;
    myExternalSettings[idx].value = value;
    if(useAsInitial) myExternalSettings[idx].initialValue = value;

    /*cerr << "modify external: key = " << key
         << ", value = " << value
         << " @ index = " << idx
         << endl;*/
  }
  else
  {
    Setting setting;
    setting.key   = key;
    setting.value = value;
    if(useAsInitial) setting.initialValue = value;

    myExternalSettings.push_back(setting);
    idx = myExternalSettings.size() - 1;

    /*cerr << "insert external: key = " << key
         << ", value = " << value
         << " @ index = " << idx
         << endl;*/
  }

  return idx;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings(const Settings&)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings& Settings::operator = (const Settings&)
{
  assert(false);

  return *this;
}
