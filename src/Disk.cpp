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

#include "pch.h"
#include "Disk.h"

/* Constants */

constexpr int TRACKS = 35;
constexpr int SECTORS = 16;
constexpr int SECTOR_SIZE = 256;
constexpr size_t SECTORS_IMAGE_SIZE = TRACKS * SECTORS * SECTOR_SIZE;	// .dsk, .do, .po
constexpr size_t NIB_TRACK_SIZE = 6656;
constexpr size_t NIB_IMAGE_SIZE = TRACKS * NIB_TRACK_SIZE;				// .nib
constexpr byte VOLUME = 254;

// The interface's boot ROM ($C100-$C1FF): the Apple Disk II boot ROM for the TK2000 (from the
// TK2000 emulator by Fabio Belavenuto, Outros/Firmware_Disco.rom). It finds its slot ($C1: I/O
// at $C090), reads track 0 sector 0 into $0800 and jumps to $0801.
static const byte bootRom[256] = {
	0xA2, 0x20, 0xA0, 0x00, 0xA2, 0x03, 0x86, 0x3C, 0x8A, 0x0A, 0x24, 0x3C, 0xF0, 0x10, 0x05, 0x3C,
	0x49, 0xFF, 0x29, 0x7E, 0xB0, 0x08, 0x4A, 0xD0, 0xFB, 0x98, 0x9D, 0x56, 0x03, 0xC8, 0xE8, 0x10,
	0xE5, 0x20, 0x58, 0xFF, 0xBA, 0xBD, 0x00, 0x01, 0x0A, 0x0A, 0x0A, 0x0A, 0x85, 0x2B, 0xAA, 0xBD,
	0x8E, 0xC0, 0xBD, 0x8C, 0xC0, 0xBD, 0x8A, 0xC0, 0xBD, 0x89, 0xC0, 0xA0, 0x50, 0xBD, 0x80, 0xC0,
	0x98, 0x29, 0x03, 0x0A, 0x05, 0x2B, 0xAA, 0xBD, 0x81, 0xC0, 0xA9, 0x56, 0x20, 0xA8, 0xFC, 0x88,
	0x10, 0xEB, 0x85, 0x26, 0x85, 0x3D, 0x85, 0x41, 0xA9, 0x08, 0x85, 0x27, 0x18, 0x08, 0xBD, 0x8C,
	0xC0, 0x10, 0xFB, 0x49, 0xD5, 0xD0, 0xF7, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0xC9, 0xAA, 0xD0, 0xF3,
	0xEA, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0xC9, 0x96, 0xF0, 0x09, 0x28, 0x90, 0xDF, 0x49, 0xAD, 0xF0,
	0x25, 0xD0, 0xD9, 0xA0, 0x03, 0x85, 0x40, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0x2A, 0x85, 0x3C, 0xBD,
	0x8C, 0xC0, 0x10, 0xFB, 0x25, 0x3C, 0x88, 0xD0, 0xEC, 0x28, 0xC5, 0x3D, 0xD0, 0xBE, 0xA5, 0x40,
	0xC5, 0x41, 0xD0, 0xB8, 0xB0, 0xB7, 0xA0, 0x56, 0x84, 0x3C, 0xBC, 0x8C, 0xC0, 0x10, 0xFB, 0x59,
	0xD6, 0x02, 0xA4, 0x3C, 0x88, 0x99, 0x00, 0x03, 0xD0, 0xEE, 0x84, 0x3C, 0xBC, 0x8C, 0xC0, 0x10,
	0xFB, 0x59, 0xD6, 0x02, 0xA4, 0x3C, 0x91, 0x26, 0xC8, 0xD0, 0xEF, 0xBC, 0x8C, 0xC0, 0x10, 0xFB,
	0x59, 0xD6, 0x02, 0xD0, 0x87, 0xA0, 0x00, 0xA2, 0x56, 0xCA, 0x30, 0xFB, 0xB1, 0x26, 0x5E, 0x00,
	0x03, 0x2A, 0x5E, 0x00, 0x03, 0x2A, 0x91, 0x26, 0xC8, 0xD0, 0xEE, 0xE6, 0x27, 0xE6, 0x3D, 0xA5,
	0x3D, 0xCD, 0x00, 0x08, 0xA6, 0x2B, 0x90, 0xDB, 0x4C, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Physical sector -> sector in the image file
static const int dosOrder[SECTORS] = { 0, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 15 };
static const int prodosOrder[SECTORS] = { 0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15 };

// 6-and-2 encoding: 6-bit values -> disk nibbles
static const byte writeTable[64] = {
	0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6, 0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3,
	0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xCB, 0xCD, 0xCE, 0xCF, 0xD3,
	0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC,
	0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/*************************************************************************************************/
CDisk::CDisk(CBus& bus, CRom& rom) :
	mRom(rom)
{
	bus.addDevice(EDevices::DISK, this);
	bus.registerAddr(EDevices::DISK, 0xC090, 0xC09F);
}

/*************************************************************************************************/
CDisk::~CDisk() {
	flush();
}

/*************************************************************************************************/
byte CDisk::read(const word addr, const uint64_t cycles) {
	return access(addr, false, 0);
}

/*************************************************************************************************/
void CDisk::write(const word addr, const byte data, const uint64_t cycles) {
	access(addr, true, data);
}

/*************************************************************************************************/
void CDisk::reset() {
	mMotorOn = false;
	mQ6 = false;
	mQ7 = false;
	mPhases = 0;
}

/*************************************************************************************************/
// The controller's switches ($C090 + n): 0-7 the stepper phases off/on, 8/9 the motor off/on,
// A/B the drive, C/D Q6 off/on, E/F Q7 off/on. Reading C (Q6 off, Q7 off) gives the next
// nibble under the head; with Q7 on (write mode) an access to C writes the latch (loaded by
// writing D or F) on the track; reading E with Q6 on senses the write protection (bit 7).
// The disk turns as fast as the program reads it: a nibble per access, with the motor on.
byte CDisk::access(const word addr, const bool isWrite, const byte data) {
	const int sw = addr & 0x0F;
	if (sw < 8) {
		const int phase = sw >> 1;
		if (sw & 1) {
			mPhases |= 1 << phase;
			const int oldTrack = mHalfTrack / 2;
			const int d = (phase - (mHalfTrack & 3)) & 3;
			if (d == 1 && mHalfTrack < TRACKS * 2 - 2) {
				mHalfTrack++;
			} else if (d == 3 && mHalfTrack > 0) {
				mHalfTrack--;
			}
			if (mHalfTrack / 2 != oldTrack) {
				flush();
			}
		} else {
			mPhases &= ~(1 << phase);
		}
		return 0;
	}
	switch (sw) {
	case 0x8:
		mMotorOn = false;
		flush();
		break;
	case 0x9:
		mMotorOn = true;
		break;
	case 0xC:
		mQ6 = false;
		break;
	case 0xD:
		mQ6 = true;
		break;
	case 0xE:
		mQ7 = false;
		break;
	case 0xF:
		mQ7 = true;
		break;
	}
	if (isWrite && mQ7 && (sw == 0xD || sw == 0xF)) {
		mLatch = data;
	}
	if (mTracks.empty() || !mMotorOn) {			// the disk only turns with the motor on
		return 0;
	}
	auto& track = mTracks[mHalfTrack / 2];
	if (sw == 0xC) {
		mPos %= track.size();
		if (mQ7) {
			if (!mWriteProtected) {
				track[mPos] = mLatch;
				mDirty[mHalfTrack / 2] = true;
			}
			mPos++;
			return 0;
		}
		return track[mPos++];
	}
	if (sw == 0xE && !isWrite && mQ6) {
		return mWriteProtected ? 0x80 : 0x00;
	}
	return 0;
}

/*************************************************************************************************/
int CDisk::imageOffset(int track, int sector) const {
	const int s = mOrder == EOrder::PRODOS ? prodosOrder[sector] : dosOrder[sector];
	return (track * SECTORS + s) * SECTOR_SIZE;
}

/*************************************************************************************************/
// A track of the image as nibbles: per sector the address field (volume, track, sector,
// checksum in 4-and-4), the data field (6-and-2) and self-sync gaps
void CDisk::nibblize(int track) {
	auto& nibs = mTracks[track];
	nibs.clear();
	if (mOrder == EOrder::NIB) {
		auto start = mImage.begin() + track * NIB_TRACK_SIZE;
		nibs.assign(start, start + NIB_TRACK_SIZE);
		return;
	}
	auto four4 = [&](byte v) {
		nibs.push_back((v >> 1) | 0xAA);
		nibs.push_back(v | 0xAA);
	};
	nibs.insert(nibs.end(), 48, 0xFF);
	for (int p = 0; p < SECTORS; p++) {
		nibs.insert(nibs.end(), { 0xD5, 0xAA, 0x96 });
		four4(VOLUME);
		four4((byte)track);
		four4((byte)p);
		four4(VOLUME ^ track ^ p);
		nibs.insert(nibs.end(), { 0xDE, 0xAA, 0xEB });
		nibs.insert(nibs.end(), 6, 0xFF);
		const byte* data = &mImage[imageOffset(track, p)];
		byte buf[342] = {};
		for (int i = 0; i < SECTOR_SIZE; i++) {		// the low 2 bits (swapped), then the high 6
			const byte bits = ((data[i] & 1) << 1) | ((data[i] >> 1) & 1);
			buf[i % 86] |= bits << (2 * (i / 86));
			buf[86 + i] = data[i] >> 2;
		}
		nibs.insert(nibs.end(), { 0xD5, 0xAA, 0xAD });
		byte prev = 0;
		for (byte v : buf) {
			nibs.push_back(writeTable[v ^ prev]);
			prev = v;
		}
		nibs.push_back(writeTable[prev]);
		nibs.insert(nibs.end(), { 0xDE, 0xAA, 0xEB });
		nibs.insert(nibs.end(), 27, 0xFF);
	}
}

/*************************************************************************************************/
// A written track back into the image: the sectors found (address field, then its data field
// with a good checksum)
bool CDisk::denibblize(int track) {
	const auto& nibs = mTracks[track];
	if (mOrder == EOrder::NIB) {
		std::copy(nibs.begin(), nibs.begin() + std::min(nibs.size(), NIB_TRACK_SIZE),
			mImage.begin() + track * NIB_TRACK_SIZE);
		return true;
	}
	byte readTable[256];
	std::fill(std::begin(readTable), std::end(readTable), 0xFF);
	for (int i = 0; i < 64; i++) {
		readTable[writeTable[i]] = (byte)i;
	}
	const size_t n = nibs.size();
	auto at = [&](size_t i) { return nibs[i % n]; };
	auto four4 = [&](size_t i) { return (byte)(((at(i) << 1) | 1) & at(i + 1)); };
	bool found = false;
	for (size_t i = 0; i < n; i++) {
		if (at(i) != 0xD5 || at(i + 1) != 0xAA || at(i + 2) != 0x96) {
			continue;
		}
		const byte t = four4(i + 5), s = four4(i + 7);
		if (t != track || s >= SECTORS) {
			continue;
		}
		size_t j = i + 11;								// the data field, after the gap
		const size_t end = j + 64;
		while (j < end && !(at(j) == 0xD5 && at(j + 1) == 0xAA && at(j + 2) == 0xAD)) {
			j++;
		}
		if (j == end) {
			continue;
		}
		j += 3;
		byte buf[342];
		byte prev = 0;
		bool ok = true;
		for (int k = 0; k < 342 && ok; k++) {
			const byte v = readTable[at(j + k)];
			ok = v != 0xFF;
			prev ^= v;
			buf[k] = prev;
		}
		if (!ok || readTable[at(j + 342)] != prev) {
			continue;
		}
		byte* data = &mImage[imageOffset(track, s)];
		for (int k = 0; k < SECTOR_SIZE; k++) {
			const byte bits = (buf[k % 86] >> (2 * (k / 86))) & 3;
			data[k] = (byte)((buf[86 + k] << 2) | ((bits & 1) << 1) | (bits >> 1));
		}
		found = true;
	}
	return found;
}

/*************************************************************************************************/
// The written tracks into the image file
void CDisk::flush() {
	bool changed = false;
	for (int t = 0; t < (int)mDirty.size(); t++) {
		if (mDirty[t]) {
			changed |= denibblize(t);
			mDirty[t] = false;
		}
	}
	if (!changed) {
		return;
	}
	FILE* f = fopen(mFileName.c_str(), "wb");
	if (f) {
		fwrite(mImage.data(), 1, mImage.size(), f);
		fclose(f);
	}
}

/*************************************************************************************************/
bool CDisk::insert(const char* fileName) {
	eject();
	std::filesystem::path path = std::filesystem::absolute(fileName);
	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	FILE* f = fopen(path.string().c_str(), "rb");
	if (!f) {
		return false;
	}
	std::vector<byte> image(NIB_IMAGE_SIZE + 1);
	const size_t size = fread(image.data(), 1, image.size(), f);
	fclose(f);
	image.resize(size);
	const EOrder order = ext == ".nib" ? EOrder::NIB : ext == ".po" ? EOrder::PRODOS : EOrder::DOS;
	if (size != (order == EOrder::NIB ? NIB_IMAGE_SIZE : SECTORS_IMAGE_SIZE)) {
		return false;
	}
	f = fopen(path.string().c_str(), "r+b");			// a read-only file: a protected disk
	mWriteProtected = f == nullptr;
	if (f) {
		fclose(f);
	}
	mFileName = path.string();
	mOrder = order;
	mImage = std::move(image);
	mTracks.assign(TRACKS, {});
	mDirty.assign(TRACKS, false);
	for (int t = 0; t < TRACKS; t++) {
		nibblize(t);
	}
	mPos = 0;
	mRom.setSlotRom(bootRom);
	return true;
}

/*************************************************************************************************/
void CDisk::eject() {
	flush();
	mTracks.clear();
	mDirty.clear();
	mImage.clear();
	mRom.setSlotRom(nullptr);
}
