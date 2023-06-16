/*
 *	otawa-x86 -- OTAWA loader for X86 instruction set
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
#ifndef OTAWA_X86_H
#define OTAWA_X86_H

#include <otawa/hard/Register.h>

namespace otawa { namespace x86 {

using namespace otawa;
using namespace otawa::hard;

extern Register AL, AH, BL, BH, CL, CH, DL, DH;
extern Register AX, BX, CX, DX;
extern Register EAX, EBX, ECX, EDX;
extern Register CS, DS, SS, ES, FS, GS;
extern Register SP, BP, ESP, EBP, SI, DI, ESI, EDI;
extern Register EFLAGS, IP, EIP;

class Platform: public hard::Platform {
public:
	Platform();
};

otawa::Decoder *makeDecoder(gel::Image *i);

}}	// otawa::x86

#endif	// OTAWA_X86_H
