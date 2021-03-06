/**
 *
 * GSettings libcompizconfig backend
 *
 * ccs_gsettings_wrapper_factory.h
 *
 * Copyright (c) 2012 Canonical Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authored By:
 *	Sam Spilsbury <sam.spilsbury@canonical.com>
 *
 **/
#ifndef _CCS_GSETTINGS_WRAPPER_FACTORY_H
#define _CCS_GSETTINGS_WRAPPER_FACTORY_H

#include <ccs-defs.h>

COMPIZCONFIG_BEGIN_DECLS

#include <ccs-fwd.h>
#include <ccs_gsettings_backend_fwd.h>

CCSGSettingsWrapperFactory *
ccsGSettingsWrapperFactoryDefaultImplNew (CCSObjectAllocationInterface *ai);

COMPIZCONFIG_END_DECLS

#endif
