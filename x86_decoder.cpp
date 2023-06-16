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

#include "x86.h"

namespace otawa { namespace x86 {

/**
 * SUMMARY OF ENCODING
 *
 * http://ref.x86asm.net/coder32.html
 * https://pnx.tf/files/x86_opcode_structure_and_instruction_overview.pdf
 *
 * FORMAT
 * 	* legacy prefixes
 * 	* opcode with prefixes
 * 	* ModR/M
 *  * SIB
 *  * Displacement
 *  * Immediate
 *
 * REGISTER ENCODING
 * 			8b		16b		32b		64b
 * 0.000	AL		AX		EAX		RAX
 *	0.001	CL		CX		ECX		RCX
 *	0.010	DL		DX		EDX		RDX
 * 	0.011	BL		BX		EBX		RBX
 * 	0.100	AH		SP		ESP		RSP
 * 	0.101	CH		BP		EBP		RBP
 * 	0.110	DH		SI		ESI		RSI
 * 	0.111	BH		DI		EDI		RDI
 * 	1.000	R8L	R8W	R8D	R8
 * 	...
 * 	1.111	R15L	R15W	R15D	R15
 *
 * LEGACY PREFIXES
 * 	Group1 : 0xF0 (LOCK), 0xF2 (REPNE/REPNZ),
 * 		0xF3 (REP or REPE/REPZ).
 * 	Group 2 : 0x2E (CS override), 0x36 (SS override),
 * 		0x3E (DS override), 0x26 (ES override),
 * 		0x64 (FS override), 0x65 (GS override),
 * 		0x2E (branch not taken), 0x3E (branch taken)
 *	Group 3 : 0x66 (operand-size override prefix)
 *	Group 4 : 0x67 (address-size override prefix)
 *
 * OPCODE
 * 	* <op>
 * 	* 0x0F <op>
 * 	* 0x0F 0x38 <op>
 *  * 0w0F 0x3A <op>
 *
 * MODR/M
 * 	MODRM.mod (7..6) = 0b11 register direct, else register-indirect
 * 	MODRM.reg (5..3) instruction dependent/register num
 * 	MODRM.rm (2..0) direct/indirect register
 */
inline t::uint8 modrm_mod(t::uint8 b) { return b >> 6; }
inline t::uint8 modrm_reg(t::uint8 b) { return (b >> 3) & 0b111; }
inline t::uint8 modrm_rm(t::uint8 b) { return b & 0b111; }


// r8, r/m8: AL, CL, DL, BL, AH, CH, DHn DHn BH, BPL, SPL, DIL, SIL
// r16, r/m16: AX, CX, DX, BX, SP, BP, SI, DI
// r32, r/m32: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
// r64, r/m64: RAX, RBX, RCX, RDX, RDI, RSI, RBP, RSP

static Register *reg32[] = {
	&EAX,
	&ECX,
	&EDX,
	&EBX,
	&ESP,
	&EBP,
	&ESI,
	&EDI
};

typedef enum {
	R32_R,	// 32-bit register (read)
	R32_W,	// 32-bit register (written)
	SIMM,	// signed immediate
	UIMM,	// unisgned immediate
	IPREL	// offset on the PC
} arg_type_t;


typedef struct inst_t {
	cstring format;
	Inst::kind_t kind;
	t::uint32 argc;
	arg_type_t args[4];
} inst_t;


// Instruction definitions
static const inst_t

	// JMP
	JMP = { "jmp %p", Inst::IS_CONTROL, 1, { IPREL} },

	// mov
	MOV32 = { "mov %1, %0", Inst::IS_ALU, 2, { R32_W, R32_R }},
	MOVI_DIS8 = { "movl %0, %1(%2)", Inst::IS_MEM|Inst::IS_STORE, 3, { UIMM, SIMM, R32_R } },

	// push/pop
	PUSH = { "push %0", Inst::IS_MEM|Inst::IS_STORE, 1, { R32_R } },
	POP = { "pop %0", Inst::IS_MEM|Inst::IS_LOAD, 1, { R32_W } },

	// sub
	SUB32I = { "sub %0, %1", Inst::IS_ALU, 2, { SIMM, R32_W }  },

	// special instruction
	ENDBR32 = { "endbr32", Inst::IS_INTERN, 0 },

	UNKNOWN = {"unknown", 0, 0 };


// Decoder class
class Decoder: public otawa::Decoder {
public:

	typedef enum {
		MODE_NONE = 0,
		MODE_REAL = 1,
		MODE_PROTECT = 2,
		MODE_LONG = 3
	} mode_t;

	t::int32
		PREF_LOCK		= 0x0001,
		PREF_REPNEZ		= 0x0002,
		PREF_REPEZZ		= 0x0004,
		PREF_NOT_TAKEN	= 0x0008,
		PREF_TAKEN		= 0x0010,
		PREF_OPER_OVER	= 0x0020,
		PREF_ADDR_OVER	= 0x0040;

	Decoder(gel::Image *image): otawa::Decoder(image) {}

	otawa::Inst * decode(gel::address_t a) override {

		// look for the segment
		if(curs.size() == 0 || base > a || a >= base + curs.size()) {
			bool found = false;
			for(auto s: image()->segments())
				if(s->range().contains(a)) {
					if(!s->isExecutable())
						return nullptr;
					curs = s->buffer();
					base = s->baseAddress();
					found = true;
					break;
			}
			if(!found)
				return nullptr;
		}
		curs.move(a - base);

		// scan prefixes
		hard::Register *seg = nullptr;
		t::uint8 byte;
		t::uint32 prefs = 0;
		bool done = false;
		while(!done) {
			if(!curs.read(byte))
				return unknown(a, size(a));
			curs.read(byte);
			switch(byte) {
			case 0xF0: prefs |= PREF_LOCK; break;
			case 0xF2: prefs |= PREF_REPNEZ; break;
			case 0xF3: prefs |= PREF_REPEZZ; break;
			case 0x2E: seg = &CS; prefs |= PREF_NOT_TAKEN; break;
			case 0x36: seg = &SS; break;
			case 0x3E: seg = &DS; prefs |= PREF_TAKEN; break;
			case 0x26: seg = &ES; break;
			case 0x64: seg = &FS; break;
			case 0x65: seg = &GS; break;
			case 0x66: prefs |= PREF_OPER_OVER; break;
			case 0x67: prefs |= PREF_ADDR_OVER; break;
			default: done = true; break;
			}
		}

		// scan opcode
		t::uint8 opcode;
		curs.read(opcode);
		switch(opcode) {

		// one-byte instructions
		case 0X50: case 0x51: case 0x52: case 0x53:
		case 0X54: case 0x55: case 0x56: case 0x57:
				return make(a, PUSH, opcode & 0x7);

		case 0X58: case 0x59: case 0x5A: case 0x5B:
		case 0X5C: case 0x5D: case 0x5E: case 0x5F:
				return make(a, POP, opcode & 0x7);

		case 0xEB: {
				t::int8 dis;
				if(!curs.read(dis))
					return unknown(a, size(a));
				return make(a, JMP, t::int32(dis));
			}
			break;

		// 2-bytes or more
		case 0x0F: {
				if(!curs.read(opcode))
					return unknown(a, size(a));
				switch(opcode) {

				case 0x1e: {
						if(!curs.avail(1))
							return unknown(a, size(a));
						curs.read(opcode);
						switch(opcode) {
						case 0xfb:	return make(a, ENDBR32);
						default:	return unknown(a, size(a));
						}
					}

				case 0x38:	// 3-bytes opcode 1
				case 0x3a:	// 3-bytes opcode 2
					return unknown(a, size(a));
				}
			}
			break;

		// instruction with ModR/M
		default: {
				t::uint8 modrm;
				if(!curs.read(modrm))
					return unknown(a, size(a));
				auto mod = modrm_mod(modrm), reg = modrm_reg(modrm), rm = modrm_rm(modrm);
				switch(opcode) {
				case 0x89:
					switch(mod) {
					case 0b11:
						return make(a, MOV32, rm, reg);
					}
					break;

				case 0x83: {
						switch(mod) {
						case 0b11: {
								t::int8 imm;
								if(!curs.read(imm))
									return unknown(a, size(a));
								return make(a, alu_imm(reg), t::int32(imm), rm);
							}
							break;
						}
					}
					break;

				case 0xc7: {
						t::int8 dis;
						if(!curs.read(dis))
							return unknown(a, size(a));
						t::uint32 imm;
						if(!curs.read(imm))
							return unknown(a, size(a));
						return make(a, MOVI_DIS8, imm, dis, rm);
					}
					break;
				}
			}
		}
		return unknown(a, size(a));
	}

	///
	t::size instSize() const override {
		return 1;
	}

	///
	hard::Platform * platform() const override {
		return new Platform();
	}

private:
	typedef t::uint32 arg_t;

	const inst_t alu_imm(t::uint8 code) {
		switch(code) {
		case 5: 	return SUB32I;
		default:	return UNKNOWN;
		}
	}

	class Inst: public otawa::Inst {
		friend class Decoder;
	public:

		inline Inst(Decoder& dec, gel::address_t addr, t::size size, const inst_t& inst)
			: _dec(dec), _addr(addr), _size(size), _inst(inst), _target(nullptr) { }

		otawa::Inst::kind_t kind() override { return _inst.kind; }
		Address address() const override { return _addr; }
		t::uint32 size() const override { return _size; }

		void dump(io::Output & out) override {
			for(int p = 0; p < _inst.format.length(); p++) {
				if(_inst.format[p] != '%')
					out << _inst.format[p];
				else {
					p++;
					int i = _inst.format[p] - '0';
					switch(_inst.args[i]) {
					case R32_R:
					case R32_W:
						out << reg32[args[i]]->name();
						break;
					case SIMM: {
							t::int32 x = args[i];
							if(x < 0) {
								out << '-';
								x = -x;
							}
							out << io::hex(x);
						}
						break;
					case UIMM:
						out << io::hex(t::uint32(args[i]));
						break;
					case IPREL:
						out << "0x" << (address() + t::int32(args[i]));
					}
				}
			}
		}

		void readRegSet(otawa::RegSet & set) override {
			for(unsigned int i = 0; i < _inst.argc; i++)
				switch(_inst.args[i]) {
				case R32_R:
					set.add(reg32[args[i]]->platformNumber());
					break;
				default:
					break;
				}
		}

		void writeRegSet(otawa::RegSet & set) override {
			for(unsigned int i = 0; i < _inst.argc; i++)
				switch(_inst.args[i]) {
				case R32_W:
					set.add(reg32[args[i]]->platformNumber());
					break;
				default:
					break;
				}
		}

		otawa::Inst *target() override {
			if(_target == nullptr)
				return _target;
			for(unsigned i = 0; i < _inst.argc; i++)
				if(_inst.args[i] == IPREL) {
					_target = _dec.decode(_addr + t::int32(args[i]));
					break;
				}
			return nullptr;
		}

	private:
		Decoder& _dec;
		gel::address_t _addr;
		t::size _size;
		const inst_t& _inst;
		t::uint32 args[4];
		otawa::Inst *_target;
	};

	Inst *unknown(gel::address_t addr, t::size size) {
		return new Inst(*this, addr, size, UNKNOWN);
	}

	inline t::size size(gel::address_t a) { return curs.offset() - (a - base); }


	Inst *make(gel::address_t a, const inst_t& inst) {
		return new Inst(*this, a, size(a), inst);
	}

	Inst *make(gel::address_t a, const inst_t& inst, arg_t arg1) {
		auto i = new Inst(*this, a, size(a), inst);
		i->args[0] = arg1;
		return i;
	}

	Inst *make(gel::address_t a, const inst_t& inst, arg_t arg1, arg_t arg2) {
		auto i = new Inst(*this, a, size(a), inst);
		i->args[0] = arg1;
		i->args[1] = arg2;
		return i;
	}

	Inst *make(gel::address_t a, const inst_t& inst, arg_t arg1, arg_t arg2, arg_t arg3) {
		auto i = new Inst(*this, a, size(a), inst);
		i->args[0] = arg1;
		i->args[1] = arg2;
		i->args[2] = arg3;
		return i;
	}

	gel::address_t base;
	gel::Cursor curs;
};

otawa::Decoder *makeDecoder(gel::Image *i) {
	return new Decoder(i);
}

}} //otawa::x86
