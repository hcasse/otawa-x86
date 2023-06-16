/*
 *	Decoder class interface
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
#ifndef OTAWA_PROG_DECODER_H
#define OTAWA_PROG_DECODER_H

#include <elm/types.h>
#include <elm/sys/Plugin.h>
#include <otawa/base.h>
#include <gel++.h>

namespace otawa {

using namespace elm;
class Inst;
namespace hard { class Platform; }
	
class Decoder {
public:
	Decoder(gel::Image *image);
	virtual ~Decoder();
	inline gel::Image *image() const { return _image; }
	virtual Inst *decode(gel::address_t a) = 0;
	virtual t::size instSize() const = 0;
	virtual hard::Platform *platform() const = 0;
private:
	gel::Image *_image;
};

class DecoderPlugin: public sys::Plugin {
public:	
	DecoderPlugin(string name, const Version& plugger_version, CString hook);
	virtual Decoder *decode(gel::Image *image) = 0;
};

#define OTAWA_DECODER_VERSION	"1.0.0"
#define OTAWA_DECODER_HOOK		decoder_plugin
#define OTAWA_DECODER_NAME		"decoder_plugin"

}	// otawa

#endif	// OTAWA_PROG_DECODER_H
