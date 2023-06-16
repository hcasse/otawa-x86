/*
 *	DefaultLoader class implementation
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

#include <elm/avl/Map.h>
#include <elm/sys/Plugger.h>

#include <gel++.h>
#include <gel++/Image.h>

#include <otawa/prog/Decoder.h>
#include <otawa/prog/DefaultLoader.h>
#include <otawa/prog/Process.h>
#include <otawa/otawa.h>

namespace otawa {

class DecoderPlugger: public sys::Plugger {
public:
	DecoderPlugger(): sys::Plugger(OTAWA_DECODER_NAME, OTAWA_DECODER_VERSION) {
		addPath(MANAGER.prefixPath() / "lib/otawa/decode");
	}
} decoder_plugger;

class DefaultProcess;

class DefaultSegment: public Segment {
public:
	DefaultSegment(
		Decoder& d,
		cstring name,
		address_t address,
		ot::size size,
		flags_t flags
	): Segment(name, address, size, flags), decoder(d) {
	}
protected:
	Inst *decode(address_t address) override {
		return decoder.decode(address.offset());
	}
private:
	Decoder& decoder;
};

class DefaultProcess: public Process {
	friend class DefaultLoader;
public:
	
	DefaultProcess(
		Manager *manager,
		const PropList& props = EMPTY,
		File *program = nullptr
	):
		Process(manager, props, program),
		image(nullptr),
		pf(nullptr),
		decoder(nullptr),
		start_inst(nullptr)
	{ }
	
	~DefaultProcess() {
		if(image != nullptr)
			delete image;
		if(pf != nullptr)
			delete pf;
	}

	hard::Platform *platform() override { return pf; }
	
	Inst *start() override {
		if(start_inst == nullptr)
			start_inst = findInstAt(start_addr);
		return start_inst;
	}
	
	int instSize() const override {
		return decoder->instSize();
	}

	void get(Address at, Address &val) override {
		auto s = image->at(at.offset());
		ASSERT(s != nullptr);
		auto b = s->buffer();
		t::uint32 a;
		b.get(at.offset() - s->baseAddress(), a);
		val = a;
	}
	
	void get(Address at, char *buf, int size) override {
		auto s = image->at(at.offset());
		ASSERT(s != nullptr);
		auto b = s->buffer();
		cstring cs;
		b.get(at.offset() - s->baseAddress(), cs);
		ASSERT(cs.length() >= size);
		for(const char *p = cs.chars(); *p != '\0'; p++)
			*buf++ = *p++;
		*buf++ = '\0';
	}
	
	void get(Address at, string &str) override {
		auto s = image->at(at.offset());
		ASSERT(s != nullptr);
		auto b = s->buffer();
		string cs;
		b.get(at.offset() - s->baseAddress(), cs);		
	}

	void get(Address at, t::int8 &val) override {
		auto s = image->at(at.offset());
		ASSERT(s != nullptr);
		auto b = s->buffer();
		b.get(at.offset() - s->baseAddress(), val);
	}

	void get(Address at, t::uint8 &val) override {
		auto s = image->at(at.offset());
		ASSERT(s != nullptr);
		auto b = s->buffer();
		b.get(at.offset() - s->baseAddress(), val);
	}

	void get(Address at, t::int16 &val) override {
		auto s = image->at(at.offset());
		ASSERT(s != nullptr);
		auto b = s->buffer();
		b.get(at.offset() - s->baseAddress(), val);
	}

	void get(Address at, t::uint16 &val) override {
		auto s = image->at(at.offset());
		ASSERT(s != nullptr);
		auto b = s->buffer();
		b.get(at.offset() - s->baseAddress(), val);
	}

	void get(Address at, t::int32 &val) override {
		auto s = image->at(at.offset());
		ASSERT(s != nullptr);
		auto b = s->buffer();
		b.get(at.offset() - s->baseAddress(), val);
	}

	void get(Address at, t::uint32 &val) override {
		auto s = image->at(at.offset());
		ASSERT(s != nullptr);
		auto b = s->buffer();
		b.get(at.offset() - s->baseAddress(), val);
	}

	void get(Address at, t::int64 &val) override {
		auto s = image->at(at.offset());
		ASSERT(s != nullptr);
		auto b = s->buffer();
		b.get(at.offset() - s->baseAddress(), val);
	}

	void get(Address at, t::uint64 &val) override {
		auto s = image->at(at.offset());
		ASSERT(s != nullptr);
		auto b = s->buffer();
		b.get(at.offset() - s->baseAddress(), val);
	}

	File *loadFile(elm::CString path) override {
		if(image != nullptr)
			throw otawa::Exception("cannot load additional executable file!");
		try {
			
			// build the image
			auto f = gel::Manager::open(path);
			image = f->make();
			auto of = new File(path);
			addFile(of);
			start_addr = f->entry();

			// create the decoder
			string mach = _ << "elf_" << f->elfMachine();
			auto plugin = static_cast<DecoderPlugin *>(decoder_plugger.plug(mach));
			if(plugin == nullptr)
				throw otawa::Exception(_ << "cannot open " << path << ": no decoder for " << mach);
			decoder = plugin->decode(image);
			pf = decoder->platform();
			
			// parse all segments
			avl::Map<gel::File *, File *> map;
			map.put(f, of);
			for(auto s: image->segments()) {
				if(s->file() == nullptr) {
					if(s->isStack())
						stack_top = s->base() + s->size();
					continue;
				}
				auto cf = map.get(s->file(), nullptr);
				if(cf == nullptr) {
					cerr << "DEBUG: added file " << s->file()->path() << io::endl;
					cf = new File(s->file()->path());
					addFile(cf);
					map.put(s->file(), cf);
				}
				Segment::flags_t flags = 0;
				if(s->isExecutable())
					flags |= Segment::EXECUTABLE;
				if(s->isWritable())
					flags |= Segment::WRITABLE;
				if(s->hasContent())
					flags |= Segment::INITIALIZED;
				auto os = new DefaultSegment(*decoder, s->name(), s->base(), s->size(), flags);
				cf->addSegment(os);
			}
			
			// load the symbols
			for(auto p: map.pairs()) {
				auto& st = p.fst->symbols();
				for(auto s: st) {
					auto k = Symbol::NONE;
					switch(s->type()) {
					case gel::Symbol::FUNC:			k = Symbol::FUNCTION; break;
					case gel::Symbol::DATA:			k = Symbol::DATA; break;
					case gel::Symbol::OTHER_TYPE:	k = Symbol::LABEL; break;
					default:						break;
					}
					p.snd->addSymbol(new Symbol(*p.snd, s->name(), k, s->value(), s->size()));
				}
			}
			
			return of;
		}
		catch(gel::Exception& e) {
			throw otawa::Exception(_ << "cannot open " << path << ": " << e.message());
		}
	}

private:
	gel::Image *image;
	hard::Platform *pf;
	Address stack_top, start_addr;
	Decoder *decoder;
	Inst *start_inst;
};

	
/**
 * @class DefaultLoader
 * Default loader implementation using a @ref Decoder to decode instructions.
 * @ingroup prog
 */

///
DefaultLoader::DefaultLoader(
	CString name,
	Version version,
	Version plugger_version,
	const elm::sys::Plugin::aliases_t & aliases)
	: Loader(name, version, plugger_version, aliases), _name(name) { }

///
DefaultLoader::DefaultLoader(make& maker): Loader(maker), _name("") {
}

///
CString DefaultLoader::getName() const {
	return _name;
}

///
Process *DefaultLoader::load(Manager *man, CString path, const PropList& props) {
	auto p = create(man, props);
	p->loadProgram(path);
	return p;
}

///
Process *DefaultLoader::create(Manager *man, const PropList& props) {
	return new DefaultProcess(man, props);
}
	
} // otawa

