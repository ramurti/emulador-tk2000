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

#include "Subject.h"
#include "Menu.h"

/* Prototypes */

class CVideo;

/*************************************************************************************************/
// Picture around the screen: the TK2000 with its monitor, the monitor and the TK2000 label only, none
enum class EFrameMode { Full, Monitor, Off };

/*************************************************************************************************/
class CWindowSDL final : public CSubject<SDL_KeyboardEvent> {
private:
	SDL_Window* mWindow{};
	SDL_Renderer* mRenderer{};
	SDL_Texture* mScreen{};
	int mScanLines{ 0 };				// scan lines: how much darker (%) the second row of each line is (0: off)
	CVideo& mVideo;
	bool mFullScreen{ false };
	CMenu* mMenu = nullptr;
	int mMenuSel = -1;
	SDL_Texture* mCpuIcon{};
	SDL_Texture* mTapeIcon{};
	SDL_Texture* mTarget{};				// Emulated screen + icons + menu, composed before scaling
	SDL_Texture* mFrame{};				// Computer/monitor picture with a transparent glass area
	int mFrameW{ 0 };
	int mFrameH{ 0 };
	SDL_Rect mGlassRect{};				// Bounding box of the transparent glass area in mFrame
	bool mShowFrame{ false };
	EFrameMode mFrameMode{ EFrameMode::Off };
	SDL_Rect mFrameView{};				// Part of mFrame shown: all of it, or the monitor and the TK2000 label
private:
	void setFullScreen(bool val) noexcept;
	void loadFrame();
public:
	CWindowSDL(CVideo& video);
	~CWindowSDL();
	// Native
	void render(bool cpuState, bool tapeState);
	void nextScanline() noexcept;		// off, 25%, 35%, off again
	int getScanline() const noexcept;
	void setFrameMode(EFrameMode mode) noexcept;
	EFrameMode getFrameMode() const noexcept;
	void nextFrameMode() noexcept;
	bool getFullScreen() const noexcept;
	void toggleFullScreen() noexcept;	// borderless full screen over the whole display, or back to the window
	bool loop();
	SDL_Window* getWindow() const;
	SDL_Renderer* getRenderer() const;
	CMenu* getMenu() const;
	void setMenu(CMenu* menu);
	int getMenuSel() const;
};
