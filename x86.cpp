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

#include <otawa/prog/DefaultLoader.h>
#include <otawa/hard/Platform.h>

#include <otawa/prog/Decoder.h>

namespace otawa { namespace x86 {

using namespace elm;
using namespace otawa;
using hard::RegBank;
using hard::Register;

// 8-bit
Register AL(Register::Make("AL").kind(Register::INT).size(8));
Register AH(Register::Make("AH").kind(Register::INT).size(8));
Register BL(Register::Make("BL").kind(Register::INT).size(8));
Register BH(Register::Make("BH").kind(Register::INT).size(8));
Register CL(Register::Make("CL").kind(Register::INT).size(8));
Register CH(Register::Make("CH").kind(Register::INT).size(8));
Register DL(Register::Make("DL").kind(Register::INT).size(8));
Register DH(Register::Make("DH").kind(Register::INT).size(8));

// 16-bit
Register AX(Register::Make("AX").kind(Register::INT).size(16));
Register BX(Register::Make("BX").kind(Register::INT).size(16));
Register CX(Register::Make("CX").kind(Register::INT).size(16));
Register DX(Register::Make("DX").kind(Register::INT).size(16));

// 32-bit
Register EAX(Register::Make("EAX").kind(Register::INT).size(32));
Register EBX(Register::Make("EBX").kind(Register::INT).size(32));
Register ECX(Register::Make("ECX").kind(Register::INT).size(32));
Register EDX(Register::Make("EDX").kind(Register::INT).size(32));

// address registers
Register CS(Register::Make("CS").kind(Register::ADDR).size(32));
Register DS(Register::Make("DS").kind(Register::ADDR).size(32));
Register SS(Register::Make("SS").kind(Register::ADDR).size(32));
Register ES(Register::Make("DS").kind(Register::ADDR).size(32));

Register SP(Register::Make("SP").kind(Register::ADDR).size(16));
Register BP(Register::Make("BS").kind(Register::ADDR).size(16));
Register ESP(Register::Make("ESP").kind(Register::ADDR).size(32));
Register EBP(Register::Make("EBS").kind(Register::ADDR).size(32));
Register SI(Register::Make("SI").kind(Register::ADDR).size(16));
Register DI(Register::Make("DI").kind(Register::ADDR).size(16));
Register ESI(Register::Make("ESI").kind(Register::ADDR).size(32));
Register EDI(Register::Make("EDI").kind(Register::ADDR).size(32));

// status registers
Register SR(Register::Make("EDI").kind(Register::BITS).size(16));
// 11 10 09 08 07 06 05 04 03 02 01 00
// OF DF IF TF SF ZF    AF    PF    CF

// platform definition
RegBank DATA(RegBank::Make("DATA")
	.add(EAX).add(EBX).add(ECX).add(EDX)		.add(AX).add(BX).add(CX).add(DX)
	.add(AL).add(AH).add(BL).add(BH).add(CL).add(CH).add(DL).add(DH)
);

RegBank ADDRESS(RegBank::Make("ADDRESS")
	.add(ESP).add(EBP).add(ESI).add(EDI)
	.add(SP).add(BP).add(SI).add(DI)
	.add(CS).add(DS).add(SS).add(ES)
);

RegBank STATUS(RegBank::Make("STATUS").add(SR));


// platform definition
const RegBank *banks[] = { &DATA, &ADDRESS, &STATUS };
class Platform: public hard::Platform {
public:
	Platform(): hard::Platform(hard::Platform::Identification("x86")) {
		setBanks(banks_t(3, otawa::x86::banks));
	}
};


// Decoder class
class Decoder: public otawa::Decoder {
public:
	
	class BaseInst: public Inst {
	public:
		BaseInst(Address addr, t::size size, kind_t kind)
			: a(addr), s(size), k(kind) { }
		Address address() const override { return a; }
		t::uint32 size() const override { return s; }
		kind_t kind() override { return k; }
	private:
		Address a;
		t::uint32 s;
		kind_t k;
	};
	
	Decoder(gel::Image *i): otawa::Decoder(i) {	}
	
	Inst *decode(gel::address_t a) {
		return new BaseInst(a, 1, Inst::IS_RETURN | Inst::IS_CONTROL);
	}
	
	t::size instSize() const { return 1; }
	hard::Platform *platform() const { return new Platform(); }
};


// Plug-ins defintions
class Loader: public otawa::DefaultLoader {
public:
	Loader(void): otawa::DefaultLoader(make("x86", OTAWA_LOADER_VERSION).version(1.0).alias("elf_23")) {
	}
	CString getName(void) const { return "x86"; }
};

class DecoderPlugin: public otawa::DecoderPlugin {
public:
	DecoderPlugin(): otawa::DecoderPlugin("x86", OTAWA_DECODER_NAME, OTAWA_DECODER_NAME) {}
	
	Decoder *decode(gel::Image *image) override {
		return new Decoder(image);
	}
};

} }	// otawa::x86

otawa::x86::Loader otawa_x86_loader;
ELM_PLUGIN(otawa_x86_loader, OTAWA_LOADER_HOOK);
otawa::x86::DecoderPlugin otawa_x86_decoder;
ELM_PLUGIN(otawa_x86_decoder, OTAWA_DECODER_HOOK);
