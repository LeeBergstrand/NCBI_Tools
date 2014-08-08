/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998-1999  Brian Bruns
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _tds_configs_h_
#define _tds_configs_h_

/* $Id: tds_configs.h 103491 2007-05-04 17:18:18Z kazimird $ */

#include "freetds_sysconfdir.h"

#ifndef _tds_h_
#error tds.h must be included before tds_configs.h
#endif

#ifdef __cplusplus
extern "C"
{
#if 0
}
#endif
#endif

#ifndef _WIN32
#    define FREETDS_SYSCONFFILE     "/etc/freetds.conf"
#    define FREETDS_POOLCONFFILE    "/etc/pool.conf"
#    define FREETDS_LOCALECONFFILE  "/etc/locales.conf"
#else
#    define FREETDS_SYSCONFFILE     "c:/freetds.conf"
#    define FREETDS_POOLCONFFILE    "c:/pool.conf"
#    define FREETDS_LOCALECONFFILE  "c:/locales.conf"
#endif

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* _tds_configs_h_ */
