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

#include "pch.h"
#include "Tape.h"

#define CT2_MAGIC "CTK2"
#define CT2_CAB_A "CA"
#define CT2_CAB_B "CB"
#define CT2_DATA  "DA"

#pragma pack(push, 1)
struct SCh {
	byte ID[2];
	word size;
};
#pragma pack(pop)

// A tape loader polls the cassette input in a tight loop; other programs (e.g. a game reading
// the keyboard, which shares the address) read it sparsely. Full speed is kept only while loading.
constexpr uint64_t FAST_READ_CYCLES = 200;
constexpr unsigned FAST_READS_LOADING = 64;
constexpr uint64_t LOAD_IDLE_CYCLES = CPU_CLOCK / 2;		// 0.5 s

/*************************************************************************************************/
CTape::CTape(CBus& bus, CCpu6502& cpu) :
	mCpu(cpu),
	mCyclesNeeded(0)
{
	bus.addDevice(EDevices::TAPE, this);
	bus.registerAddr(EDevices::TAPE, 0xC010, 0xC02F);
	bus.registerAddr(EDevices::TAPE, 0xC052, 0xC053);
	bus.registerAddr(EDevices::TAPE, 0xC056, 0xC057);
	mQueueIt = mQueueCycles.end();
}

/*************************************************************************************************/
byte CTape::read(const word addr, const uint64_t cycles) {
	if ((addr & 0xF0) == 0x10) {
		if (!mPlay) {
			return 0x00;
		}
		const unsigned long long actualCycle = mCpu.getCumulativeCycles();
		if (actualCycle - mLastReadCycle < FAST_READ_CYCLES) {
			if (++mFastReads >= FAST_READS_LOADING) {
				mLastLoadCycle = actualCycle;
				mCpu.setFullSpeed(true);
			}
		} else {
			mFastReads = 0;
		}
		mLastReadCycle = actualCycle;
		// Sparse reads (e.g. the keyboard, at the same address) while no loader is active don't move the
		// tape, as if the recorder had been stopped: programs loaded in parts ask to stop and restart it
		if (actualCycle - mLastLoadCycle > LOAD_IDLE_CYCLES) {
			return mCasOut;
		}
		if (mStartCycle == 0 && mQueueIt != mQueueCycles.end()) {
			mStartCycle = actualCycle;
			mCyclesNeeded = (*mQueueIt);
		}
		if (actualCycle - mStartCycle > mCyclesNeeded) {
			mStartCycle = actualCycle;
			mCasOut ^= 0x80;
			if (mQueueIt != mQueueCycles.end()) {
				mCyclesNeeded = *(mQueueIt++);
			} else {
				mStartCycle = 0;
				mPlay = false;
				mCpu.setFullSpeed(false);
			}
		}
	}

	return mCasOut;
}

/*************************************************************************************************/
void CTape::update(const uint64_t cycles) {
	// Back to normal speed when the program stops loading (e.g. a game started before the tape end)
	if (mPlay && mCpu.getFullSpeed() && cycles - mLastLoadCycle > LOAD_IDLE_CYCLES) {
		mCpu.setFullSpeed(false);
	}
}

/*************************************************************************************************/
void CTape::reset() {
	mPlay = false;
	mStartCycle = 0;
}

/*************************************************************************************************/
void CTape::play() noexcept {
	if (mQueueIt != mQueueCycles.end()) {
		mPlay = true;
		mLastLoadCycle = mCpu.getCumulativeCycles();
		mCpu.setFullSpeed(true);
	}
}

/*************************************************************************************************/
void CTape::stop() noexcept {
	if (mPlay) {
		mPlay = false;
		mQueueIt = mQueueCycles.end();
	}
}

/*************************************************************************************************/
void CTape::rewind() noexcept {
	mQueueIt = mQueueCycles.begin();
}

/*************************************************************************************************/
bool CTape::getPlayState() const noexcept {
	return mPlay;
}

/*************************************************************************************************/
bool CTape::insertCt2(const char *fileName) {
	FILE *fileTape = fopen(fileName, "rb");
	if (!fileTape) {
		return false;
	}
	dword magic;
	size_t rlen = fread(&magic, 1, sizeof(dword), fileTape);
	if (memcmp(&magic, CT2_MAGIC, sizeof(dword)) != 0) {
		return false;
	}
	mQueueCycles.clear();
	mFiles.clear();
	mFileName = std::filesystem::absolute(fileName).string();
	SCh ch;
	while (true) {
		rlen = fread(&ch, 1, sizeof(SCh), fileTape);
		if (feof(fileTape)) {
			break;
		}
		if (memcmp(ch.ID, CT2_CAB_A, sizeof(word)) == 0) {
			mFiles.emplace_back();
			for (int i = 0; i < 500; i++) {
				mQueueCycles.emplace_back(500);
				mQueueCycles.emplace_back(500);
			}
		} else if (memcmp(ch.ID, CT2_CAB_B, sizeof(word)) == 0) {
			mQueueCycles.emplace_back(464);
			mQueueCycles.emplace_back(679);
			for (int i = 0; i < 32; i++) {
				mQueueCycles.emplace_back(679);
				mQueueCycles.emplace_back(679);
			}
			mQueueCycles.emplace_back(199);
			mQueueCycles.emplace_back(250);
		} else if (memcmp(ch.ID, CT2_DATA, sizeof(word)) == 0) {
			if (mFiles.empty()) {
				mFiles.emplace_back();
			}
			mFiles.back().emplace_back();
			byte b;
			for (int i = 0; i < ch.size; i++) {
				rlen = fread(&b, 1, 1, fileTape);
				mFiles.back().back().push_back(b);
				for (int j = 0; j < 8; j++) {
					int mask = 1 << (7 - j);
					if ((mask & b) == mask) {
						// 1
						mQueueCycles.emplace_back(500);
						mQueueCycles.emplace_back(500);
					} else {
						// 0
						mQueueCycles.emplace_back(250);
						mQueueCycles.emplace_back(250);
					}
				}
			}
		}
	}
	mQueueIt = mQueueCycles.begin();	// Update iterator
	return true;
}

/*************************************************************************************************/
// The inserted tape as a WAV for a real TK2000 (LOADT), next to the .ct2 (never over an existing
// file): the timings measured on a real recording, as "Py converte/ct2wav.py" (tested on a real
// TK2000). Per file: pilot, then per block a lead-in, the sync and the bytes (MSB first, with
// the checksum); 8-bit mono at 44100 Hz.
bool CTape::saveWav(std::string& wavName) const {
	constexpr int SAMPLE_RATE = 44100;
	constexpr int AMPLITUDE = 110;
	constexpr double PILOT_US = 493, LEAD_US = 666, SYNC0_US = 204, SYNC1_US = 238;
	constexpr double BIT0_US = 246, BIT1_US = 493;
	constexpr int PILOT_N = 9468, LEAD_N = 63;
	if (mFiles.empty()) {
		return false;
	}
	std::vector<byte> samples;
	double t = 0;		// time in samples, accumulated (no rounding drift)
	int level = 0;
	auto half = [&](double us) {
		t += us * SAMPLE_RATE / 1e6;
		const size_t end = (size_t)std::nearbyint(t);
		samples.resize(end, (byte)(level ? 128 + AMPLITUDE : 128 - AMPLITUDE));
		level ^= 1;
	};
	auto silence = [&](double sec) {
		samples.resize(samples.size() + (size_t)(sec * SAMPLE_RATE), 128);
		t = (double)samples.size();
	};
	silence(1.0);
	for (size_t f = 0; f < mFiles.size(); f++) {
		if (f) {
			silence(2.0);
		}
		for (int i = 0; i < PILOT_N; i++) {
			half(PILOT_US);
		}
		for (auto& block : mFiles[f]) {
			for (int i = 0; i < LEAD_N; i++) {
				half(LEAD_US);
			}
			half(SYNC0_US);
			half(SYNC1_US);
			for (byte b : block) {
				for (int k = 7; k >= 0; k--) {
					const double us = (b >> k) & 1 ? BIT1_US : BIT0_US;
					half(us);
					half(us);
				}
			}
		}
		half(LEAD_US);		// the edge that closes the last checksum bit
	}
	silence(1.0);

	std::filesystem::path base(mFileName);
	base.replace_extension();
	std::filesystem::path out = base.string() + ".wav";
	for (int n = 2; std::filesystem::exists(out); n++) {
		out = base.string() + "-" + std::to_string(n) + ".wav";
	}
	FILE* f = fopen(out.string().c_str(), "wb");
	if (!f) {
		return false;
	}
	auto put32 = [&](uint32_t v) { fwrite(&v, 4, 1, f); };
	auto put16 = [&](uint16_t v) { fwrite(&v, 2, 1, f); };
	const uint32_t size = (uint32_t)samples.size();
	fwrite("RIFF", 1, 4, f); put32(36 + size); fwrite("WAVEfmt ", 1, 8, f);
	put32(16); put16(1); put16(1); put32(SAMPLE_RATE); put32(SAMPLE_RATE); put16(1); put16(8);
	fwrite("data", 1, 4, f); put32(size);
	fwrite(samples.data(), 1, size, f);
	const bool ok = ferror(f) == 0;
	fclose(f);
	wavName = out.filename().string();
	return ok;
}
