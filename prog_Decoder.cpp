/*
 *	Decoder class implementation
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

#include <otawa/prog/Decoder.h>

namespace otawa {

/**
 * @class Decoder
 * A decoder is able to decode instruction from a program image.
 * Decoders are provided by DecoderPlugin.
 * @ingroup prog
 */

/**
 * Build a decoder on the given image.
 * @param image		Image used by the decoder.
 */
Decoder::Decoder(gel::Image *image): _image(image) {
}

///
Decoder::~Decoder() { }


/**
 * @fn Inst *Decoder::decode(gel::address_t a);
 * Decode the instruction at address a.
 * @param a		Address of decoded instruction.
 * @return		Decode instruction (ownership passed to caller).
 */

/**
 * @fn t::size Decoder::instSize() const;
 * Get the minimim size of an instruction.
 * @return	Minimum size of an instruction (in bytes).
 */

/**
 * @fn hard::Platform *Decoder::platform() const;
 * Get the platform used by the decoder.
 * @return	Decoder platform.
 */


/**
 * @class DecoderPlugin
 * Plug-in for Decoder classes. Provides a function to build a decoder.
 * @ingroup prog
 */

///
DecoderPlugin::DecoderPlugin(string name, const Version& plugger_version, CString hook)
	: sys::Plugin(name, plugger_version, hook) { }
	
/**
 * @fn Decoder *DecoderPlugin::decode(gel::Image *image);
 * Build a decoder working on the given image.
 * @param image		Image to decode in.
 * @return			Decoder for the image.
 */
	
}	// otawa
