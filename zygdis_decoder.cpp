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

#include <Zydis/Zydis.h>
#include "x86.h"

namespace otawa { namespace x86 {

// Decoder class
class Decoder: public otawa::Decoder {
public:

	class BaseInst: public Inst {
	public:
		BaseInst(Decoder& decoder, Address addr, t::size size, kind_t kind)
			: dec(decoder), a(addr.offset()), s(size), k(kind) { }
		Address address() const override { return a; }
		t::uint32 size() const override { return s; }
		kind_t kind() override { return k; }

		void dump(io::Output &out) override {
			//cerr << io::hex(a) << ':' << s << io::endl;
			auto r = dec.decodeRaw(a);
			ASSERT(r);
			char buffer[256];
			ZydisFormatterFormatInstruction(&dec.zform, &dec.zi, buffer, sizeof(buffer), a);
			out << buffer;
		}

	protected:
		Decoder &dec;
		t::uint32 a;
		t::uint32 s;
		kind_t k;
	};

	class Branch: public BaseInst {
	public:
		Branch(Decoder& decoder, Address addr, t::size size, kind_t kind, t::int32 off): BaseInst(decoder, addr, size, kind), o(off), t(nullptr) { }
		Inst *target() override {
			if(getKind().isIndirect())
				return nullptr;
			else if(t == nullptr)
				t = dec.decode(a + s + o);
			return t;
		}
	private:
		t::int32 o;
		Inst *t;
	};

	Decoder(gel::Image *i): otawa::Decoder(i) {
		ZydisDecoderInit(&zdec, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_ADDRESS_WIDTH_32);
		ZydisFormatterInit(&zform, ZYDIS_FORMATTER_STYLE_INTEL);
	}

	Inst *decode(gel::address_t a) override {
		if(!decodeRaw(a))
			return nullptr;

		// compute kind
		Inst *inst = nullptr;
		Inst::kind_t k = 0;
		switch(zi.mnemonic) {

		// relative branchers
		// target = topAddress() + operands[0].imm
		case ZYDIS_MNEMONIC_JMP:
			k = Inst::IS_CONTROL;
			goto branch;
		case ZYDIS_MNEMONIC_JB:		// ULT
		case ZYDIS_MNEMONIC_JBE:	// ULE
		case ZYDIS_MNEMONIC_JCXZ:	// CX = 0
		case ZYDIS_MNEMONIC_JECXZ:	// ???
		case ZYDIS_MNEMONIC_JKNZD:	// ???
		case ZYDIS_MNEMONIC_JKZD:	// ???
		case ZYDIS_MNEMONIC_JL:		// LT
		case ZYDIS_MNEMONIC_JLE:	// LE
		case ZYDIS_MNEMONIC_JNB:	// UGE
		case ZYDIS_MNEMONIC_JNBE:	// UGT
		case ZYDIS_MNEMONIC_JNL:	// GE
		case ZYDIS_MNEMONIC_JNLE:	// GT
		case ZYDIS_MNEMONIC_JNO:	// not overflow
		case ZYDIS_MNEMONIC_JNP:	// parity odd
		case ZYDIS_MNEMONIC_JNS:	// positive
		case ZYDIS_MNEMONIC_JNZ:	// NE
		case ZYDIS_MNEMONIC_JO:		// overflow
		case ZYDIS_MNEMONIC_JP:		// parity
		case ZYDIS_MNEMONIC_JRCXZ:	// ???
		case ZYDIS_MNEMONIC_JS:		// negative
		case ZYDIS_MNEMONIC_JZ:		// EQ
			k = Inst::IS_CONTROL | Inst::IS_COND;
			goto branch;

		case ZYDIS_MNEMONIC_RET:
			k = Inst::IS_RETURN | Inst::IS_CONTROL;
			goto normal;

		branch:
			if(zi.operands[0].type != ZYDIS_OPERAND_TYPE_IMMEDIATE)
				inst = new Branch(*this, a, zi.length, k | Inst::IS_INDIRECT, 0);
			else
				inst = new Branch(*this, a, zi.length, k, zi.operands[0].imm.value.s);
			break;

		default:
		normal:
			inst = new BaseInst(*this, a, zi.length, k);
			break;
		}

		// dump operands (for debugging)
#		if 0
		cerr << "DEBUG: instr @" << io::hex(a) << ": " << static_cast<otawa::Inst *>(inst) << io::endl;
		for(int i = 0; i < zi.operand_count; i++) {
			cerr << "DEBUG: operand " << i << " (" << int(zi.operands[i].type) << "): ";
			switch(zi.operands[i].type) {
			case ZYDIS_OPERAND_TYPE_REGISTER: {
					auto r = decodeReg(zi.operands[i].reg.value);
					if(r != nullptr)
						cerr << r->name();
					cerr << " (" << int(zi.operands[i].reg.value) << ")";
				}
				break;
			case ZYDIS_OPERAND_TYPE_MEMORY: {
					cerr << "[";
					auto r = decodeReg(zi.operands[i].mem.segment);
					if(r != nullptr)
						cerr << r->name() << ":";
					else
						cerr << int(zi.operands[i].mem.segment) << ":";
					r = decodeReg(zi.operands[i].mem.base);
					if(r != nullptr)
						cerr << r->name();
					else
						cerr << int(zi.operands[i].mem.base);
					if(zi.operands[i].mem.index != ZYDIS_REGISTER_NONE) {
						r = decodeReg(zi.operands[i].mem.index);
						if(r != nullptr)
							cerr << " + " << r->name();
						else
							cerr << " + " << int(zi.operands[i].mem.index);
						if(zi.operands[i].mem.scale != 0)
							cerr << "*" << zi.operands[i].mem.scale;
					}
					if(zi.operands[i].mem.disp.value != 0)
						cerr << " + " << zi.operands[i].mem.disp.value;
					cerr << "]";
				}
				break;
			case ZYDIS_OPERAND_TYPE_POINTER:
				break;
			case ZYDIS_OPERAND_TYPE_IMMEDIATE:
				if(zi.operands[i].imm.is_signed)
					cerr << io::hex(zi.operands[i].imm.value.s);
				else
					cerr << io::hex(zi.operands[i].imm.value.u);
				break;
			default:
				break;
			}
			switch(zi.operands[i].visibility) {
			case ZYDIS_OPERAND_VISIBILITY_EXPLICIT: cerr << " +"; break;
			case ZYDIS_OPERAND_VISIBILITY_IMPLICIT: cerr << " ~"; break;
			case ZYDIS_OPERAND_VISIBILITY_HIDDEN:   cerr << " -"; break;
			default:								cerr << " ?"; break;
			}
			cerr << io::endl;
		}
		cerr << io::endl;
#		endif

		return inst;
	}

	t::size instSize() const override { return 1; }
	hard::Platform *platform() const override { return new Platform(); }

private:

	bool decodeRaw(t::uint32 a) {
		auto seg = image()->at(a);
		if(seg == nullptr)
			return false;
		auto buf = seg->buffer();
		auto r = ZydisDecoderDecodeBuffer(
			&zdec,
			buf.at(a - seg->baseAddress()),
			seg->baseAddress() + seg->size() - a,
			&zi);
		return ZYAN_SUCCESS(r);
	}

	hard::Register *decodeReg(ZydisRegister r) {
		switch(r) {
		case ZYDIS_REGISTER_AL: 	return &AL;
		case ZYDIS_REGISTER_CL:		return &CL;
		case ZYDIS_REGISTER_DL:		return &DL;
		case ZYDIS_REGISTER_BL:		return &BL;
		case ZYDIS_REGISTER_AH:		return &AH;
		case ZYDIS_REGISTER_CH:		return &CH;
		case ZYDIS_REGISTER_DH:		return &DH;
		case ZYDIS_REGISTER_BH:		return &BH;
		//case ZYDIS_REGISTER_SPL:	return &SP;
		//case ZYDIS_REGISTER_BPL:	return &BP;
		case ZYDIS_REGISTER_AX:		return &AX;
		case ZYDIS_REGISTER_CX:		return &CX;
		case ZYDIS_REGISTER_DX:		return &DX;
		case ZYDIS_REGISTER_BX:		return &BX;
		case ZYDIS_REGISTER_SP:		return &SP;
		case ZYDIS_REGISTER_BP:		return &BP;
		case ZYDIS_REGISTER_SI:		return &SI;
		case ZYDIS_REGISTER_DI:		return &DI;
		case ZYDIS_REGISTER_EAX:	return &EAX;
		case ZYDIS_REGISTER_ECX:	return &ECX;
		case ZYDIS_REGISTER_EDX:	return &EDX;
		case ZYDIS_REGISTER_EBX:	return &EBX;
		case ZYDIS_REGISTER_ESP:	return &ESP;
		case ZYDIS_REGISTER_EBP:	return &EBP;
		case ZYDIS_REGISTER_ESI:	return &ESI;
		case ZYDIS_REGISTER_EDI:	return &EDI;
		case ZYDIS_REGISTER_EFLAGS:	return &EFLAGS;
		case ZYDIS_REGISTER_IP:		return &IP;
		case ZYDIS_REGISTER_EIP:	return &EIP;
		default:
			return nullptr;
		}
	}

	ZydisDecoder zdec;
	ZydisFormatter zform;
	ZydisDecodedInstruction zi;
};

otawa::Decoder *makeDecoder(gel::Image *i) {
	return new Decoder(i);
}

}}	// otawa::x86

