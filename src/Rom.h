// Copyright (c) 2024 FBLabs
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#pragma once

#include "Common.h"
#include "Device.h"
#include "Bus.h"

// ROM at $C100-$FFFF. On the 64K TK2000 II, RAM under the ROM is selected by $C05B and the
// ROM again by $C05A; writes to $C100-$FFFF always go to the RAM.
class CRom final : public CDevice {
public:
	CRom(CBus& bus, byte* ramPtr);
	byte read(const word addr, const uint64_t cycles) override;
	void write(const word addr, const byte data, const uint64_t cycles) override;
	void update(const uint64_t cycles) override {}
	void reset() override;
private:
	byte* mRamPtr = nullptr;
	bool mRamSelected{ false };
};
