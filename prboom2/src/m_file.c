/* Emacs style mode select   -*- C -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *  External simple file handling.
 *
 *-----------------------------------------------------------------------------*/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <direct.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "lprintf.h"
#include "z_zone.h"

#include "m_file.h"

#ifdef _MSC_VER
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define F_OK 0
#define W_OK 2
#define R_OK 4
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define MKDIR_NO_ERROR 0

#ifdef _WIN32
static wchar_t *ConvertMultiByteToWide(const char *str, UINT code_page) {
  wchar_t *wstr = NULL;
  int wlen = 0;

  wlen = MultiByteToWideChar(code_page, 0, str, -1, NULL, 0);

  if (!wlen) {
    errno = EINVAL;
    lprintf(LO_INFO, "Warning: Failed to convert path to wide encoding\n");
    return NULL;
  }

  wstr = Z_Malloc(sizeof(wchar_t) * wlen);

  if (!wstr) {
    lprintf(LO_INFO, "ConvertMultiByteToWide: Failed to allocate new string\n");
    return NULL;
  }

  if (MultiByteToWideChar(code_page, 0, str, -1, wstr, wlen) == 0) {
    errno = EINVAL;
    lprintf(LO_INFO, "Warning: Failed to convert path to wide encoding\n");
    Z_Free(wstr);
    return NULL;
  }

  return wstr;
}

static char *ConvertWideToMultiByte(const wchar_t *wstr, UINT code_page) {
  char *str = NULL;
  int len = 0;

  len = WideCharToMultiByte(code_page, 0, wstr, -1, NULL, 0, NULL, NULL);

  if (!len) {
    errno = EINVAL;
    lprintf(LO_INFO, "Warning: Failed to convert path to multi byte encoding\n");
    return NULL;
  }

  str = Z_Malloc(sizeof(char) * len);

  if (!str) {
    lprintf(LO_INFO, "ConvertWideToMultiByte: Failed to allocate new string\n");
    return NULL;
  }

  if (WideCharToMultiByte(code_page, 0, wstr, -1, str, len, NULL, NULL) == 0) {
    errno = EINVAL;
    lprintf(LO_INFO, "Warning: Failed to convert path to multi byte encoding\n");
    Z_Free(str);
    return NULL;
  }

  return str;
}

wchar_t *ConvertUtf8ToWide(const char *str) {
  return ConvertMultiByteToWide(str, CP_UTF8);
}

static char *ConvertWideToUtf8(const wchar_t *wstr) {
  return ConvertWideToMultiByte(wstr, CP_UTF8);
}

static wchar_t *ConvertSysNativeMBToWide(const char *str) {
  return ConvertMultiByteToWide(str, CP_ACP);
}

static char *ConvertWideToSysNativeMB(const wchar_t *wstr) {
  return ConvertWideToMultiByte(wstr, CP_ACP);
}
#endif

char *ConvertSysNativeMBToUtf8(const char *str) {
#ifdef _WIN32
  char *ret = NULL;
  wchar_t *wstr = NULL;

  wstr = ConvertSysNativeMBToWide(str);

  if (!wstr)
    return NULL;

  ret = ConvertWideToUtf8(wstr);

  Z_Free(wstr);

  return ret;
#else
  return Z_Strdup(str);
#endif
}

char *ConvertUtf8ToSysNativeMB(const char *str) {
#ifdef _WIN32
  char *ret = NULL;
  wchar_t *wstr = NULL;

  wstr = ConvertUtf8ToWide(str);

  if (!wstr)
    return NULL;

  ret = ConvertWideToSysNativeMB(wstr);

  Z_Free(wstr);

  return ret;
#else
  return Z_Strdup(str);
#endif
}

static int M_open(const char *filename, int oflag) {
#ifdef _WIN32
  wchar_t *wname;
  int ret;

  wname = ConvertUtf8ToWide(filename);

  if (!wname)
    return 0;

  ret = _wopen(wname, oflag);

  Z_Free(wname);

  return ret;
#else
  return open(filename, oflag);
#endif
}

static int M_access(const char *path, int mode) {
#ifdef _WIN32
  wchar_t *wpath;
  int ret;

  wpath = ConvertUtf8ToWide(path);

  if (!wpath)
    return 0;

  ret = _waccess(wpath, mode);

  Z_Free(wpath);

  return ret;
#else
  return access(path, mode);
#endif
}

static int M_mkdir(const char *path) {
#ifdef _WIN32
  wchar_t *wdir = NULL;
  int ret;

  wdir = ConvertUtf8ToWide(path);

  if (!wdir)
    return -1;

  ret = _wmkdir(wdir);

  Z_Free(wdir);

  return ret;
#else
  return mkdir(path, 0755);
#endif
}

static int M_stat(const char *path, struct stat *buf)
{
#ifdef _WIN32
  wchar_t *wpath;
  struct _stat wbuf;
  int ret;

  wpath = ConvertUtf8ToWide(path);

  if (!wpath)
    return -1;

  ret = _wstat(wpath, &wbuf);

  // The _wstat() function expects a struct _stat* parameter that is
  // incompatible with struct stat*. We copy only the required compatible
  // field.
  buf->st_mode = wbuf.st_mode;
  buf->st_mtime = wbuf.st_mtime;

  Z_Free(wpath);

  return ret;
#else
  return stat(path, buf);
#endif
}

int M_remove(const char *path)
{
#ifdef _WIN32
  wchar_t *wpath;
  int ret;

  wpath = ConvertUtf8ToWide(path);

  if (!wpath)
    return 0;

  ret = _wremove(wpath);

  Z_Free(wpath);

  return ret;
#else
  return remove(path);
#endif
}

int M_MakeDir(const char *path, int require) {
  int error;

  if (M_IsDir(path))
    return MKDIR_NO_ERROR;

  error = M_mkdir(path);

  if (require && error)
    I_Error("Unable to create directory %s (%d)", path, errno);

  return error;
}

dboolean M_IsDir(const char *name)
{
  struct stat sbuf;

  return !M_stat(name, &sbuf) && S_ISDIR(sbuf.st_mode);
}

dboolean M_ReadWriteAccess(const char *name)
{
  return !M_access(name, R_OK | W_OK);
}

dboolean M_ReadAccess(const char *name)
{
  return !M_access(name, R_OK);
}

dboolean M_WriteAccess(const char *name)
{
  return !M_access(name, W_OK);
}

FILE* M_OpenFile(const char *name, const char *mode)
{
  #ifdef _WIN32
  FILE *file;
  wchar_t *wname, *wmode;

  wname = ConvertUtf8ToWide(name);

  if (!wname)
    return NULL;

  wmode = ConvertUtf8ToWide(mode);

  if (!wmode)
  {
    Z_Free(wname);
    return NULL;
  }

  file = _wfopen(wname, wmode);

  Z_Free(wname);
  Z_Free(wmode);

  return file;
#else
  return fopen(name, mode);
#endif
}

int M_OpenRB(const char *name)
{
  return M_open(name, O_RDONLY | O_BINARY);
}

dboolean M_FileExists(const char *name)
{
  FILE* fp;

  fp = M_OpenFile(name, "rb");

  if (fp)
  {
    fclose(fp);

    return true;
  }

  return false;
}

/*
 * M_WriteFile
 *
 * killough 9/98: rewritten to use stdio and to flash disk icon
 */

dboolean M_WriteFile(char const *name, const void *source, size_t length)
{
  FILE *fp;

  errno = 0;

  if (!(fp = M_OpenFile(name, "wb")))  // Try opening file
    return 0;                          // Could not open file for writing

  length = fwrite(source, 1, length, fp) == (size_t)length;   // Write data
  fclose(fp);

  if (!length)                         // Remove partially written file
    M_remove(name);

  return length;
}

/*
 * M_ReadFile
 *
 * killough 9/98: rewritten to use stdio and to flash disk icon
 */

int M_ReadFile(char const *name, byte **buffer)
{
  FILE *fp;

  if ((fp = M_OpenFile(name, "rb")))
    {
      size_t length;

      fseek(fp, 0, SEEK_END);
      length = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      *buffer = Z_Malloc(length);
      if (fread(*buffer, 1, length, fp) == length)
        {
          fclose(fp);
          return length;
        }
      fclose(fp);
    }

  /* cph 2002/08/10 - this used to return 0 on error, but that's ambiguous,
   * because we could have a legit 0-length file. So make it -1. */
  return -1;
}

// Same as above, but add null terminator
int M_ReadFileToString(char const *name, char **buffer) {
  FILE *fp;

  if ((fp = M_OpenFile(name, "rb")))
  {
    size_t length;

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    *buffer = Z_Malloc(length + 1);
    if (fread(*buffer, 1, length, fp) == length)
    {
      fclose(fp);
      (*buffer)[length] = '\0';
      return length;
    }
    Z_Free(*buffer);
    *buffer = NULL;
    fclose(fp);
  }

  /* cph 2002/08/10 - this used to return 0 on error, but that's ambiguous,
   * because we could have a legit 0-length file. So make it -1. */
  return -1;
}

char *M_getcwd(char *buffer, int len)
{
#ifdef _WIN32
  wchar_t *wret;
  char *ret;

  wret = _wgetcwd(NULL, 0);

  if (!wret)
    return NULL;

  ret = ConvertWideToUtf8(wret);

  free(wret);

  if (!ret)
    return NULL;

  if (!buffer)
    return ret;

  if (strlen(ret) >= len) {
    Z_Free(ret);
    return NULL;
  }

  strcpy(buffer, ret);
  Z_Free(ret);

  return buffer;
#else
  return getcwd(buffer, len);
#endif
}

#ifdef _WIN32
typedef struct {
  char *var;
  const char *name;
} env_var_t;

static env_var_t *env_vars;
static int num_vars;
#endif

char *M_getenv(const char *name) {
#ifdef _WIN32
  int i;
  wchar_t *wenv, *wname;
  char *env;

  for (i = 0; i < num_vars; ++i) {
    if (!strcasecmp(name, env_vars[i].name))
      return env_vars[i].var;
  }

  wname = ConvertUtf8ToWide(name);

  if (!wname)
    return NULL;

  wenv = _wgetenv(wname);

  Z_Free(wname);

  if (wenv)
    env = ConvertWideToUtf8(wenv);
  else
    env = NULL;

  env_vars = Z_Realloc(env_vars, (num_vars + 1) * sizeof(*env_vars));
  env_vars[num_vars].var = env;
  env_vars[num_vars].name = Z_Strdup(name);
  num_vars++;

  return env;
#else
  return getenv(name);
#endif
}
