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
// $Id: FSNodePOSIX.cxx,v 1.21 2009-01-16 19:37:29 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "FSNode.hxx"

#ifdef MACOSX
  #include <sys/types.h>
#endif

#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include <sstream>

/*
 * Implementation of the Stella file system API based on POSIX (for Linux and OSX)
 *
 * Parts of this class are documented in the base interface class, AbstractFilesystemNode.
 */
class WiiFilesystemNode : public AbstractFilesystemNode
{
  public:
    /**
     * Creates a WiiFilesystemNode with the root node as path.
     */
    WiiFilesystemNode();

    /**
     * Creates a WiiFilesystemNode for a given path.
     *
     * @param path String with the path the new node should point to.
     * @param verify true if the isValid and isDirectory flags should be verified during the construction.
     */
    WiiFilesystemNode(const string& path, bool verify);

    virtual bool exists() const { return true; }
    virtual string getDisplayName() const { return _displayName; }
    virtual string getName() const   { return _displayName; }
    virtual string getPath() const   { return _path; }
    virtual bool isDirectory() const { return _isDirectory; }
    virtual bool isReadable() const  { return true; }
    virtual bool isWritable() const  { return true; }

    virtual bool getChildren(AbstractFSList& list, ListMode mode, bool hidden) const;
    virtual AbstractFilesystemNode* getParent() const;

  protected:
    string _displayName;
    string _path;
    bool _isDirectory;
    bool _isValid;
    bool _isReadable;
    bool _isWritable;

  private:
    /**
     * Tests and sets the _isValid and _isDirectory flags, using the stat() function.
     */
    virtual void setFlags();
};

/**
 * Returns the last component of a given path.
 *
 * Examples:
 *			/foo/bar.txt would return /bar.txt
 *			/foo/bar/    would return /bar/
 *
 * @param str String containing the path.
 * @return Pointer to the first char of the last component inside str.
 */
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* lastPathComponent(const string& str)
{
  if(str.empty())
    return "";

  const char *start = str.c_str();
  const char *cur = start + str.size() - 2;

  while (cur >= start && *cur != '/')
    --cur;

  return cur + 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WiiFilesystemNode::setFlags()
{
  struct stat st;

  _isValid = (0 == stat(_path.c_str(), &st));
  _isDirectory = _isValid ? S_ISDIR(st.st_mode) : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WiiFilesystemNode::WiiFilesystemNode()
{
  // The root dir.
  _path = "/";
  _displayName = _path;
  _isValid = true;
  _isDirectory = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WiiFilesystemNode::WiiFilesystemNode(const string& p, bool verify)
{
  if (p == "" || p == "/") {
    // The root dir.
    _path = "/";
    _displayName = _path;
    _isValid = true;
    _isDirectory = true;
    return;
  }

  // Expand "./" to the current directory
  if ( p.length() >= 2 && p[0] == '.' && p[1] == '/')
  {
    char buf[MAXPATHLEN];
    char* ret = getcwd(buf, MAXPATHLEN);
    if (ret == buf)
    {
      _path = buf;
      // Skip over the tilda.  We know that p contains at least
      // two chars, so this is safe:
      _path += p.c_str() + 1;
    }
  }
  else
    _path = p;

  _displayName = lastPathComponent(_path);

  if (verify)
    setFlags();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WiiFilesystemNode::getChildren(AbstractFSList& myList, ListMode mode,
                                      bool hidden) const
{
  assert(_isDirectory);

  if (_path == "" || _path == "/") {
	const char* roots[] = { "sd:/", "usb:/" };
	for (const char** c = roots; c < roots + sizeof(roots) / sizeof(char*); c++) {
	  WiiFilesystemNode* n = new WiiFilesystemNode(string(*c), false);
	  n->_isValid = true;
	  n->_isDirectory = true;
	  myList.push_back(n);
	}
    return true;
  }

  DIR *dirp = opendir(_path.c_str());
  struct dirent *dp;

  if (dirp == NULL)
    return false;

  // loop over dir entries using readdir
  while ((dp = readdir(dirp)) != NULL)
  {
    // Skip 'invisible' files if necessary
    if (dp->d_name[0] == '.' && !hidden)
      continue;

    // Skip '.' and '..' to avoid cycles
    if ((dp->d_name[0] == '.' && dp->d_name[1] == 0) || (dp->d_name[0] == '.' && dp->d_name[1] == '.'))
      continue;

    string newPath(_path);
    if (newPath.length() > 0 && newPath[newPath.length()-1] != '/')
      newPath += '/';
    newPath += dp->d_name;

    WiiFilesystemNode entry(newPath, false);

#if defined(SYSTEM_NOT_SUPPORTING_D_TYPE)
    /* TODO: d_type is not part of POSIX, so it might not be supported
     * on some of our targets. For those systems where it isn't supported,
     * add this #elif case, which tries to use stat() instead.
     *
     * The d_type method is used to avoid costly recurrent stat() calls in big
     * directories.
     */
    entry.setFlags();
#else
    if (dp->d_type == DT_UNKNOWN)
    {
      // Fall back to stat()
      entry.setFlags();
    }
    else
    {
      entry._isValid = (dp->d_type == DT_DIR) || (dp->d_type == DT_REG) || (dp->d_type == DT_LNK);
      if (dp->d_type == DT_LNK)
      {
        struct stat st;
        if (stat(entry._path.c_str(), &st) == 0)
          entry._isDirectory = S_ISDIR(st.st_mode);
        else
          entry._isDirectory = false;
      }
      else
        entry._isDirectory = (dp->d_type == DT_DIR);
    }
#endif

    // Skip files that are invalid for some reason (e.g. because we couldn't
    // properly stat them).
    if (!entry._isValid)
      continue;

    // Honor the chosen mode
    if ((mode == FilesystemNode::kListFilesOnly && entry._isDirectory) ||
        (mode == FilesystemNode::kListDirectoriesOnly && !entry._isDirectory))
      continue;

    if (entry._isDirectory)
      entry._path += "/";

    myList.push_back(new WiiFilesystemNode(entry));
  }
  closedir(dirp);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode* WiiFilesystemNode::getParent() const
{
  if (_path == "" || _path == "/")
    return 0;

  const char *start = _path.c_str();
  const char *end = lastPathComponent(_path);

  return new WiiFilesystemNode(string(start, end - start), true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode* AbstractFilesystemNode::makeRootFileNode()
{
  return new WiiFilesystemNode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode* AbstractFilesystemNode::makeCurrentDirectoryFileNode()
{
  return new WiiFilesystemNode("./", true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode* AbstractFilesystemNode::makeHomeDirectoryFileNode()
{
  return new WiiFilesystemNode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode* AbstractFilesystemNode::makeFileNodePath(const string& path)
{
  return new WiiFilesystemNode(path, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AbstractFilesystemNode::makeDir(const string& path)
{
  // Reject paths like "/" or "sd:"
  if (lastPathComponent(path) < path.c_str() + 1)
    return false;
  else
    return mkdir(path.c_str(), 0777) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AbstractFilesystemNode::renameFile(const string& oldfile,
                                        const string& newfile)
{
  // Reject paths like "/" or "sd:"
  if (lastPathComponent(oldfile) < oldfile.c_str() + 1
		  || lastPathComponent(newfile) < newfile.c_str() + 1)
    return false;
  else
	return rename(oldfile.c_str(), newfile.c_str()) == 0;
}
