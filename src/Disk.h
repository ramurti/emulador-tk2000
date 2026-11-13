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

#include "Device.h"
#include "Bus.h"
#include "Rom.h"

// The TK2000 disk interface (a cartridge, a copy of the Apple Disk II controller): one 5 1/4"
// drive, the controller's registers at $C090-$C09F and its boot ROM at $C100-$C1FF (which hides
// the TK2000's ROM and RAM there while the cartridge is plugged: here, while a disk is inserted).
// Disk images: .dsk/.do (DOS 3.3 sector order), .po (ProDOS order), .nib (raw nibbles). The
// tracks are kept as nibbles (6-and-2, 16 sectors); written tracks are decoded back into the
// image file when the motor stops, the head leaves the track or the disk is ejected.
class CDisk final : public CDevice {
public:
	CDisk(CBus& bus, CRom& rom);
	~CDisk();
	// CDevice
	byte read(const word addr, const uint64_t cycles) override;
	void write(const word addr, const byte data, const uint64_t cycles) override;
	void update(const uint64_t cycles) override {}
	void reset() override;
	// Native
	bool insert(const char* fileName);
	void eject();
	bool isInserted() const noexcept { return !mTracks.empty(); }
	bool isMotorOn() const noexcept { return mMotorOn; }
private:
	enum class EOrder { DOS, PRODOS, NIB };
	CRom& mRom;
	std::string mFileName;
	EOrder mOrder{ EOrder::DOS };
	bool mWriteProtected{ false };
	std::vector<byte> mImage;						// The image file's bytes
	std::vector<std::vector<byte>> mTracks;			// The 35 tracks as nibbles
	std::vector<bool> mDirty;						// Tracks written since the last save
	int mHalfTrack{ 0 };
	byte mPhases{ 0 };								// Stepper phases on (bits 0-3)
	bool mMotorOn{ false };
	uint64_t mSpinUntil{ 0 };						// Motor off: the drive still on until then
	bool mQ6{ false };								// Q6 on: load / sense write protect
	bool mQ7{ false };								// Q7 on: write mode
	byte mLatch{ 0 };
	byte mLastRead{ 0 };							// The last value read from the data latch
	size_t mPos{ 0 };								// Position on the track (nibbles)
private:
	byte access(const word addr, const bool isWrite, const byte data, const uint64_t cycles);
	void nibblize(int track);
	bool denibblize(int track);
	void flush();
	int imageOffset(int track, int sector) const;
};
