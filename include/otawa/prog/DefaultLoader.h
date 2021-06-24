/*
 *	DefaultLoader class interface
 *
 *	This file is part of x86 plug-in for OTAWA.
 *	Copyright (c) 2021, Hugues Cassé <hug.casse@gmail.com>.
 *
 *	OTAWA is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	OTAWA is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with OTAWA; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef OTAWA_PROG_DEFAULT_LOADER_H
#define OTAWA_PROG_DEFAULT_LOADER_H

#include <otawa/prog/Loader.h>

namespace otawa {

using namespace elm;

class DefaultLoader: public Loader {
public:
	DefaultLoader(
		CString name,
		Version version,
		Version plugger_version,
		const elm::sys::Plugin::aliases_t & aliases = elm::sys::Plugin::aliases_t::null);
	DefaultLoader(make& maker);	

	CString getName() const override;
	Process *load(Manager *man, CString path, const PropList& props) override;
	Process *create(Manager *man, const PropList& props) override;
private:
	cstring _name;
};
	
} // otawa

#endif // OTAWA_PROG_DEFAULT_LOADER_H
