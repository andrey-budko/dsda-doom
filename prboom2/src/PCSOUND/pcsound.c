// Emacs style mode select   -*- C -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// DESCRIPTION:
//    PC speaker interface.
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "pcsound.h"

//e6y
#include "doomtype.h"
#include "lprintf.h"
#include "m_file.h"

#ifdef USE_WIN32_PCSOUND_DRIVER
extern pcsound_driver_t pcsound_win32_driver;
#endif

#ifdef HAVE_LINUX_KD_H
extern pcsound_driver_t pcsound_linux_driver;
#endif

extern pcsound_driver_t pcsound_sdl_driver;

static pcsound_driver_t *drivers[] =
{
#ifdef HAVE_LINUX_KD_H
    &pcsound_linux_driver,
#endif
#ifdef USE_WIN32_PCSOUND_DRIVER
    &pcsound_win32_driver,
#endif
    &pcsound_sdl_driver,
    NULL,
};

static pcsound_driver_t *pcsound_driver = NULL;

int PCSound_Init(pcsound_callback_func callback_func)
{
    char *driver_name;
    int i;

    if (pcsound_driver != NULL)
    {
        return 1;
    }

    // Check if the environment variable is set

    driver_name = M_getenv("PCSOUND_DRIVER");

    if (driver_name != NULL)
    {
        for (i=0; drivers[i] != NULL; ++i)
        {
            if (!strcasecmp(drivers[i]->name, driver_name))
            {
                // Found the driver!

                if (drivers[i]->init_func(callback_func))
                {
                    pcsound_driver = drivers[i];
                }
                else
                {
                    lprintf(LO_WARN, "Failed to initialise PC sound driver: %s\n",
                           drivers[i]->name);
                    break;
                }
            }
        }
    }
    else
    {
        // Try all drivers until we find a working one

        for (i=0; drivers[i] != NULL; ++i)
        {
            if (drivers[i]->init_func(callback_func))
            {
                pcsound_driver = drivers[i];
                break;
            }
        }
    }

    if (pcsound_driver != NULL)
    {
        lprintf(LO_INFO, "Using PC sound driver: %s\n", pcsound_driver->name);
        return 1;
    }
    else
    {
        lprintf(LO_WARN, "Failed to find a working PC sound driver.\n");
        return 0;
    }
}

void PCSound_Shutdown(void)
{
    pcsound_driver->shutdown_func();
    pcsound_driver = NULL;
}
