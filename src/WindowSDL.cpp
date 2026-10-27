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
#include "WindowSDL.h"
#include "Video.h"
#include "media.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

static const char* FRAME_FILE = "tk2000-frame.png";


/*****************************************************************************/
void CWindowSDL::setScanline(bool val) noexcept {
	mScanLines = val;
}

/*****************************************************************************/
void CWindowSDL::setFullScreen(bool val) noexcept {
	mFullScreen = val;
	SDL_SetWindowFullscreen(mWindow, mFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

/*****************************************************************************/
void CWindowSDL::setShowFrame(bool val) noexcept {
	mShowFrame = val && mFrame != nullptr;
	int w = mShowFrame ? mFrameW : VIDEOWIDTH * 2;
	int h = mShowFrame ? mFrameH : VIDEOHEIGHT * 2;
	// Render at a fixed logical resolution, SDL scales it to the window size keeping the aspect ratio
	SDL_RenderSetLogicalSize(mRenderer, w, h);
	// Smooth scaling for the screen inside the frame, sharp pixels otherwise
	SDL_SetTextureScaleMode(mTarget, mShowFrame ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
	if (!mFullScreen) {
		// Default window size: 1200 wide with frame, 3x without, limited to the usable display area
		int winW = mShowFrame ? 1200 : VIDEOWIDTH * 3;
		int winH = winW * h / w;
		SDL_Rect usable;
		if (SDL_GetDisplayUsableBounds(SDL_GetWindowDisplayIndex(mWindow), &usable) == 0 && winH > usable.h * 9 / 10) {
			winH = usable.h * 9 / 10;
			winW = winH * w / h;
		}
		SDL_SetWindowSize(mWindow, winW, winH);
	}
}

/*****************************************************************************/
void CWindowSDL::loadFrame() {
	// Look next to the executable first, then in the res directory (running from the repository root)
	std::string paths[2];
	char* base = SDL_GetBasePath();
	if (base) {
		paths[0] = std::string(base) + FRAME_FILE;
		SDL_free(base);
	}
	paths[1] = std::string("res/") + FRAME_FILE;
	for (auto& path : paths) {
		int w, h, n;
		byte* data = path.empty() ? nullptr : stbi_load(path.c_str(), &w, &h, &n, 4);
		if (!data) {
			continue;
		}
		// Glass area = bounding box of the transparent pixels
		int x0 = w, y0 = h, x1 = -1, y1 = -1;
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				if (data[(y * w + x) * 4 + 3] < 128) {
					x0 = std::min(x0, x); x1 = std::max(x1, x);
					y0 = std::min(y0, y); y1 = std::max(y1, y);
				}
			}
		}
		SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(data, w, h, 32, w * 4, SDL_PIXELFORMAT_RGBA32);
		if (surface && x1 >= 0) {
			mFrame = SDL_CreateTextureFromSurface(mRenderer, surface);
			SDL_SetTextureBlendMode(mFrame, SDL_BLENDMODE_BLEND);
			SDL_SetTextureScaleMode(mFrame, SDL_ScaleModeLinear);
			mFrameW = w;
			mFrameH = h;
			mGlassRect = { x0, y0, x1 - x0 + 1, y1 - y0 + 1 };
		}
		SDL_FreeSurface(surface);
		stbi_image_free(data);
		if (mFrame) {
			return;
		}
	}
}

/*****************************************************************************/
CWindowSDL::CWindowSDL(CVideo& video) :
	mVideo(video)
{
	mWindow = SDL_CreateWindow("TK2000 Emulator", SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, VIDEOWIDTH * 3, VIDEOHEIGHT * 3,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

	if (!mWindow) {
		std::string error{ "Window could not be created! SDL Error: " };
		error.append(SDL_GetError());
		throw std::runtime_error(error);
	}
	SDL_Surface* sIcon{};
	sIcon = SDL_LoadBMP_RW(SDL_RWFromConstMem(iconBMP, sizeof(iconBMP)), true);
	if (sIcon) {
		SDL_SetColorKey(sIcon, SDL_TRUE, SDL_MapRGB(sIcon->format, 255, 0, 255));
		SDL_SetWindowIcon(mWindow, sIcon);
		SDL_FreeSurface(sIcon);
	}

	mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!mRenderer) {
		std::string error{ "Error creating renderer! SDL Error: " };
		error.append(SDL_GetError());
		throw std::runtime_error(error);
	}
	mTarget = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
		VIDEOWIDTH * 2, VIDEOHEIGHT * 2);
	if (!mTarget) {
		std::string error{ "Error creating target texture! SDL Error: " };
		error.append(SDL_GetError());
		throw std::runtime_error(error);
	}
	mScreen = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING,
		VIDEOWIDTH, VIDEOHEIGHT * 2);
	if (!mScreen) {
		std::string error{ "Error creating screen texture! SDL Error: " };
		error.append(SDL_GetError());
		throw std::runtime_error(error);
	}
	SDL_Surface* surface = SDL_LoadBMP_RW(SDL_RWFromConstMem(cpuBMP, sizeof(cpuBMP)), true);
	SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0, 0, 0));
	mCpuIcon = SDL_CreateTextureFromSurface(mRenderer, surface);
	SDL_FreeSurface(surface);
	surface = SDL_LoadBMP_RW(SDL_RWFromConstMem(tapeBMP, sizeof(tapeBMP)), true);
	SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0, 0, 0));
	mTapeIcon = SDL_CreateTextureFromSurface(mRenderer, surface);
	SDL_FreeSurface(surface);
	// Optional frame picture; without it, only the screen is shown
	loadFrame();
	setShowFrame(true);
}

/*****************************************************************************/
CWindowSDL::~CWindowSDL() {
	SDL_DestroyTexture(mCpuIcon);
	SDL_DestroyTexture(mTapeIcon);
	SDL_DestroyTexture(mFrame);
	SDL_DestroyTexture(mTarget);
	SDL_DestroyTexture(mScreen);
	SDL_DestroyRenderer(mRenderer);		//Free resources and close SDL
	SDL_DestroyWindow(mWindow);			//Destroy window
}

/*****************************************************************************/
void CWindowSDL::render(bool cpuState, bool tapeState) {
	// Compose the emulated screen, icons and menu at native size
	SDL_SetRenderTarget(mRenderer, mTarget);
	SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(mRenderer);
	char* fb = (char*)mVideo.getFrameBuffer();
	char* ptr;
	int pitch;
	SDL_LockTexture(mScreen, nullptr, (void **)&ptr, &pitch);
	memset(ptr, 0, VIDEOHEIGHT * 2 * pitch);
	for (int i = 0; i < VIDEOHEIGHT; i++) {
		memcpy(ptr, fb, pitch);
		ptr += pitch;
		if (!mScanLines) {
			memcpy(ptr, fb, pitch);
		}
		ptr += pitch;
		fb += pitch;
	}
	SDL_UnlockTexture(mScreen);
	if (mMenu) {
		SDL_SetTextureBlendMode(mScreen, SDL_BLENDMODE_BLEND);
		SDL_SetTextureColorMod(mScreen, 100, 100, 100);
	} else {
		SDL_SetTextureBlendMode(mScreen, SDL_BLENDMODE_NONE);
		SDL_SetTextureColorMod(mScreen, 255, 255, 255);
	}
	SDL_RenderCopy(mRenderer, mScreen, NULL, NULL);

	// Render states
	SDL_SetTextureBlendMode(mCpuIcon, SDL_BLENDMODE_BLEND);
	if (cpuState) {
		SDL_SetTextureColorMod(mCpuIcon, 0, 160, 0);
	} else {
		SDL_SetTextureColorMod(mCpuIcon, 160, 0, 0);
	}
	SDL_SetTextureAlphaMod(mCpuIcon, 128);

	SDL_SetTextureBlendMode(mTapeIcon, SDL_BLENDMODE_BLEND);
	if (tapeState) {
		SDL_SetTextureColorMod(mTapeIcon, 0, 160, 0);
	} else {
		SDL_SetTextureColorMod(mTapeIcon, 160, 0, 0);
	}
	SDL_SetTextureAlphaMod(mTapeIcon, 128);

	SDL_Rect r{VIDEOWIDTH * 2 - 32, VIDEOHEIGHT * 2 - 16, 16, 16};
	SDL_RenderCopy(mRenderer, mCpuIcon, nullptr, &r);
	r.x += 16;
	SDL_RenderCopy(mRenderer, mTapeIcon, nullptr, &r);

	// Render menu if active
	if (mMenu) {
		mMenu->render();
	}

	// Scale the composed screen to the window, inside the monitor glass when the frame is shown
	SDL_SetRenderTarget(mRenderer, nullptr);
	SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(mRenderer);			// Clear letterbox borders
	if (mShowFrame) {
		// Dark CRT glass, picture with some overscan margin, frame on top hides the glass corners.
		// The picture is added to the glass color, so its black is exactly the glass color.
		SDL_SetRenderDrawColor(mRenderer, 16, 20, 18, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(mRenderer, &mGlassRect);
		SDL_Rect pic{ mGlassRect.x + mGlassRect.w * 6 / 100, mGlassRect.y + mGlassRect.h * 7 / 100,
			mGlassRect.w * 88 / 100, mGlassRect.h * 86 / 100 };
		SDL_SetTextureBlendMode(mTarget, SDL_BLENDMODE_ADD);
		SDL_RenderCopy(mRenderer, mTarget, nullptr, &pic);
		SDL_RenderCopy(mRenderer, mFrame, nullptr, nullptr);
	} else {
		SDL_SetTextureBlendMode(mTarget, SDL_BLENDMODE_NONE);
		SDL_RenderCopy(mRenderer, mTarget, nullptr, nullptr);
	}
	// Present the screen
	SDL_RenderPresent(mRenderer);
}

/*****************************************************************************/
bool CWindowSDL::loop() {
	//Event handler
	SDL_Event e;
	bool processed = false;

	//Handle events on queue
	while (SDL_PollEvent(&e) != 0) {
		switch (e.type) {
		case SDL_QUIT:
			return true;

		case SDL_KEYDOWN:
			switch (e.key.keysym.sym) {
			case SDLK_UP:
				if (mMenu) {
					processed = true;
					mMenu->up();
				}
				break;

			case SDLK_DOWN:
				if (mMenu) {
					processed = true;
					mMenu->down();
				}
				break;

			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				if (mMenu) {
					processed = true;
					mMenuSel = mMenu->getSelected();
				}
				break;

			case SDLK_F7:
				setScanline(!mScanLines);
				processed = true;
				break;

			case SDLK_F8:
				setFullScreen(!mFullScreen);
				processed = true;
				break;

			case SDLK_F9:
				setShowFrame(!mShowFrame);
				processed = true;
				break;

			}
			if (!processed) {
				// Notify all observers
				notify(e.key);
			}
			break;

		case SDL_KEYUP:
			// Notify all observers
			notify(e.key);
			break;

		}
	}
	return false;
}

/*****************************************************************************/
SDL_Window* CWindowSDL::getWindow() const {
	return mWindow;
}

/*****************************************************************************/
SDL_Renderer* CWindowSDL::getRenderer() const {
	return mRenderer;
}

/*****************************************************************************/
CMenu* CWindowSDL::getMenu() const {
	return mMenu;
}

/*****************************************************************************/
void CWindowSDL::setMenu(CMenu* menu) {
	mMenu = menu;
	mMenuSel = -1;
}

/*****************************************************************************/
int CWindowSDL::getMenuSel() const {
	return mMenuSel;
}