/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2012 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "SDL20Video.h"

#include "Interface.h"

#include "GUI/Button.h"
#include "GUI/Console.h"
#include "GUI/GameControl.h" // for TargetMode (contextual information for touch inputs)

using namespace GemRB;

//touch gestures
#define MIN_GESTURE_DELTA_PIXELS 5
#define TOUCH_RC_NUM_TICKS 500

SDL20VideoDriver::SDL20VideoDriver(void)
{
	assert( core->NumFingScroll > 1 && core->NumFingKboard > 1 && core->NumFingInfo > 1);
	assert( core->NumFingScroll < 5 && core->NumFingKboard < 5 && core->NumFingInfo < 5);
	assert( core->NumFingScroll != core->NumFingKboard );

	renderer = NULL;
	window = NULL;
	screenTexture = NULL;

	// touch input
	ignoreNextFingerUp = 0;
	ClearFirstTouch();
	EndMultiGesture();
}

SDL20VideoDriver::~SDL20VideoDriver(void)
{
	// no need to call DestroyMovieScreen()
	SDL_DestroyTexture(screenTexture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

int SDL20VideoDriver::CreateDisplay(int w, int h, int bpp, bool fs, const char* title)
{
	fullscreen=fs;
	width = w, height = h;

	Log(MESSAGE, "SDL 2 Driver", "Creating display");
	// TODO: scale methods can be nearest or linear, and should be settable in config
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	Uint32 winFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#if TARGET_OS_IPHONE || ANDROID
	// this allows the user to flip the device upsidedown if they wish and have the game rotate.
	// it also for some unknown reason is required for retina displays
	winFlags |= SDL_WINDOW_RESIZABLE;
	// this hint is set in the wrapper for iPad at a higher priority. set it here for iPhone
	// don't know if Android makes use of this.
	SDL_SetHintWithPriority(SDL_HINT_ORIENTATIONS, "LandscapeRight LandscapeLeft", SDL_HINT_DEFAULT);
#endif
	if (fullscreen) {
		winFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		//This is needed to remove the status bar on Android/iOS.
		//since we are in fullscreen this has no effect outside Android/iOS
		winFlags |= SDL_WINDOW_BORDERLESS;
	}
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, winFlags);
	if (window == NULL) {
		Log(ERROR, "SDL 2 Driver", "couldnt create window:%s", SDL_GetError());
		return GEM_ERROR;
	}

	renderer = SDL_CreateRenderer(window, -1, 0);

	if (renderer == NULL) {
		Log(ERROR, "SDL 2 Driver", "couldnt create renderer:%s", SDL_GetError());
		return GEM_ERROR;
	}

	// we set logical size so that platforms where the window can be a diffrent size then requested
	// function properly. eg iPhone and Android the requested size may be 640x480,
	// but the window will always be the size of the screen
	SDL_RenderSetLogicalSize(renderer, width, height);

	Viewport.w = width;
	Viewport.h = height;

	SDL_RendererInfo info;
	SDL_GetRendererInfo(renderer, &info);

	Uint32 format = SDL_PIXELFORMAT_ABGR8888;
	// TODO: SDL forces SDL_PIXELFORMAT_ABGR8888 on OpenGLES2
	// if we want to use this driver with other renderers we need to do some
	// selection such as the commented out code below
/*
	for (Uint32 i=0; i<info.num_texture_formats; i++) {
		// TODO: probably could be more educated about selecting the best format.
		switch (bpp) {
			case 16:
				if (SDL_PIXELTYPE(info.texture_formats[i]) == SDL_PIXELTYPE_PACKED16) {
					format = info.texture_formats[i];
					goto doneFormat;
				}
				continue;
			case 32:
			default:
				if (SDL_PIXELTYPE(info.texture_formats[i]) == SDL_PIXELTYPE_PACKED32) {
					format = info.texture_formats[i];
					goto doneFormat;
				}
				continue;
		}
	}

doneFormat:
	if (format == SDL_PIXELFORMAT_UNKNOWN) {
		format = SDL_GetWindowPixelFormat(window);
		// bpp will be set by SDL_PixelFormatEnumToMasks
	}
*/
	screenTexture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, width, height);

	int access;
	SDL_QueryTexture(screenTexture,
                     &format,
                     &access,
                     &width,
                     &height);

	Uint32 r, g, b, a;
	SDL_PixelFormatEnumToMasks(format, &bpp, &r, &g, &b, &a);
	a = 0; //force a to 0 or screenshots will be all black!

	Log(MESSAGE, "SDL 2 Driver", "Creating Main Surface: w=%d h=%d fmt=%s",
		width, height, SDL_GetPixelFormatName(format));
	backBuf = SDL_CreateRGBSurface( 0, width, height,
									bpp, r, g, b, a );
	// tmpBuf is here as a truly ugly hack, so we can copy backBuf to tmpBuf before blitting cursors, and then back again after the screen is presented.
	tmpBuf = SDL_CreateRGBSurface( 0, width, height, bpp, r, g, b, a );
	this->bpp = bpp;

	if (!backBuf) {
		Log(ERROR, "SDL 2 Video", "Unable to create backbuffer of %s format: %s",
			SDL_GetPixelFormatName(format), SDL_GetError());
		return GEM_ERROR;
	}
	disp = backBuf;

	return GEM_OK;
}

void SDL20VideoDriver::InitMovieScreen(int &w, int &h, bool yuv)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	if (screenTexture) SDL_DestroyTexture(screenTexture);
	if (yuv) {
		screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, w, h);
	} else {
		screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	}
	if (!screenTexture) {
		Log(ERROR, "SDL 2 Driver", "Unable to create texture for video playback: %s", SDL_GetError());
	}
	w = width;
	h = height;
	//setting the subtitle region to the bottom 1/4th of the screen
	subtitleregion.w = w;
	subtitleregion.h = h/4;
	subtitleregion.x = 0;
	subtitleregion.y = h-h/4;
}

void SDL20VideoDriver::DestroyMovieScreen()
{
	if (screenTexture) SDL_DestroyTexture(screenTexture);
	// recreate the texture for gameplay
	// temporarily hardcoding format: see 91becce77374e96da38eb0d9a45f119a74b07cd4
	Uint32 format = SDL_PIXELFORMAT_ABGR8888;
	//SDL_GetWindowPixelFormat(window);
	screenTexture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, width, height);
	// destroy any events that took place during the movies
	SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
	SDL_RenderClear(renderer); // I guess the videos can potentially be a larger size then the game.
}

void SDL20VideoDriver::showFrame(unsigned char* buf, unsigned int bufw,
							   unsigned int bufh, unsigned int sx, unsigned int sy, unsigned int w,
							   unsigned int h, unsigned int dstx, unsigned int dsty,
							   int g_truecolor, unsigned char *pal, ieDword titleref)
{
	assert( bufw == w && bufh == h );

	SDL_Rect srcRect = {(int)sx, (int)sy, (int)w, (int)h};
	SDL_Rect destRect = {(int)dstx, (int)dsty, (int)w, (int)h};

	Uint32 *dst;
	unsigned int row, col;
	void *pixels;
	int pitch;
	SDL_Color color = {0, 0, 0, 0};

	if(SDL_LockTexture(screenTexture, NULL, &pixels, &pitch) != GEM_OK) {
		Log(ERROR, "SDL 2 driver", "Unable to lock video player: %s", SDL_GetError());
		return;
	}
	if (g_truecolor) {
		Uint16 *src = (Uint16*)buf;
		for (row = 0; row < bufh; ++row) {
			dst = (Uint32*)((Uint8*)pixels + row * pitch);
			for (col = 0; col < bufw; ++col) {
				color.r = ((*src & 0x7C00) >> 7) | ((*src & 0x7C00) >> 12);
				color.g = ((*src & 0x03E0) >> 2) | ((*src & 0x03E0) >> 8);
				color.b = ((*src & 0x001F) << 3) | ((*src & 0x001F) >> 2);
				// video player texture is of ARGB format. buf is RGB555
				*dst++ = (0xFF000000|(color.r << 16)|(color.g << 8)|(color.b));
				src++;
			}
		}
	} else {
		Uint8 *src = buf;
		SDL_Palette* palette;
		palette = SDL_AllocPalette(256);
		for (int i = 0; i < 256; i++) {
			palette->colors[i].r = ( *pal++ ) << 2;
			palette->colors[i].g = ( *pal++ ) << 2;
			palette->colors[i].b = ( *pal++ ) << 2;
		}
		for (row = 0; row < bufh; ++row) {
			dst = (Uint32*)((Uint8*)pixels + row * pitch);
			for (col = 0; col < bufw; ++col) {
				color = palette->colors[*src++];
				// video player texture is of ARGB format
				*dst++ = (0xFF000000|(color.r << 16)|(color.g << 8)|(color.b));
			}
		}
		SDL_FreePalette(palette);
	}
	SDL_UnlockTexture(screenTexture);

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, screenTexture, &srcRect, &destRect);

	if (titleref>0) {
		SDL_Rect rect = RectFromRegion(subtitleregion);
		SDL_RenderFillRect(renderer, &rect);
		DrawMovieSubtitle( titleref );
	}

	SDL_RenderPresent(renderer);
}

void SDL20VideoDriver::showYUVFrame(unsigned char** buf, unsigned int *strides,
				  unsigned int /*bufw*/, unsigned int /*bufh*/,
				  unsigned int w, unsigned int h,
				  unsigned int dstx, unsigned int dsty,
				  ieDword /*titleref*/)
{
	SDL_Rect destRect;
	destRect.x = dstx;
	destRect.y = dsty;
	destRect.w = w;
	destRect.h = h;
/*
 // This is superseded by SDL_UpdateYUVTexture, but I havent throughly tested it.
 // Not that I expect it to perform worse than the manual code :)
	Uint8 *pixels;
	int pitch;
	if(SDL_LockTexture(screenTexture, NULL, (void**)&pixels, &pitch) != GEM_OK) {
		Log(ERROR, "SDL 2 driver", "Unable to lock video player: %s", SDL_GetError());
		return;
	}
	pitch = w;
	if((unsigned int)pitch == strides[0]) {
		int size = pitch * bufh;
		memcpy(pixels, buf[0], size);
		memcpy(pixels + size, buf[2], size / 4);
		memcpy(pixels + size * 5 / 4, buf[1], size / 4);
	} else {
		unsigned char *Y,*U,*V,*iY,*iU,*iV;
		unsigned int i;
		Y = pixels;
		V = pixels + pitch * h;
		U = pixels + pitch * h * 5 / 4;

		iY = buf[0];
		iU = buf[1];
		iV = buf[2];

		for (i = 0; i < (h/2); i++) {
			memcpy(Y,iY,pitch);
			iY += strides[0];
			Y += pitch;

			memcpy(Y,iY,pitch);
			memcpy(U,iU,pitch / 2);
			memcpy(V,iV,pitch / 2);

			Y  += pitch;
			U  += pitch / 2;
			V  += pitch / 2;
			iY += strides[0];
			iU += strides[1];
			iV += strides[2];
		}
	}
	SDL_UnlockTexture(screenTexture);
 */
	SDL_RenderClear(renderer);
	// FIXME: why do we have to invert the last 2 channels? I dont think the data is in the format we claim...
	SDL_UpdateYUVTexture(screenTexture, NULL, buf[0], strides[0], buf[2], strides[2], buf[1], strides[1]);
	SDL_RenderCopy(renderer, screenTexture, NULL, &destRect);
	SDL_RenderPresent(renderer);
}

int SDL20VideoDriver::SwapBuffers(void)
{
	//this is not pretty. We make a complete copy of the backBuf into tmpBuf, then copy it back after SDLVideoDriver::SwapBuffers has blitted cursors and tooltips, and SDL_UpdateTexture has copied it to the screentexture.
	if (Cursor[CursorIndex] && !(MouseFlags & (MOUSE_DISABLED | MOUSE_HIDDEN))) {
		SDL_BlitSurface(backBuf, NULL, tmpBuf, NULL);
	}
	int ret = SDLVideoDriver::SwapBuffers();

	SDL_UpdateTexture(screenTexture, NULL, backBuf->pixels, backBuf->pitch);
	if (Cursor[CursorIndex] && !(MouseFlags & (MOUSE_DISABLED | MOUSE_HIDDEN))) {
		SDL_BlitSurface(tmpBuf, NULL, backBuf, NULL);
	}
	/*
	 Commenting this out because I get better performance (on iOS) with SDL_UpdateTexture
	 Don't know how universal it is yet so leaving this in commented out just in case

	 void *pixels;
	 int pitch;
	 if(SDL_LockTexture(screenTexture, NULL, &pixels, &pitch) != GEM_OK) {
	 Log(ERROR, "SDL 2 driver", "Unable to lock screen texture: %s", SDL_GetError());
	 return GEM_ERROR;
	 }

	 ieByte* src = (ieByte*)backBuf->pixels;
	 ieByte* dest = (ieByte*)pixels;
	 for( int row = 0; row < height; row++ ) {
	 memcpy(dest, src, width * backBuf->format->BytesPerPixel);
	 dest += pitch;
	 src += backBuf->pitch;
	 }
	 SDL_UnlockTexture(screenTexture);
	 */

	/*
	 if (fadeColor.a) {
	 SDL_Rect dst = {
	 xCorr, yCorr, Viewport.w, Viewport.h
	 };
	 SDL_SetRenderDrawColor(renderer, fadeColor.r, fadeColor.g, fadeColor.b, fadeColor.a);
	 SDL_RenderFillRect(renderer, &dst);
	 }
	 */
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
	SDL_RenderPresent( renderer );
	return ret;
}

int SDL20VideoDriver::PollEvents()
{
	if (ignoreNextFingerUp <= 0
		&& firstFingerDownTime
		&& GetTickCount() - firstFingerDownTime >= TOUCH_RC_NUM_TICKS) {
		// enough time has passed to transform firstTouch into a right click event

		// store the finger coordinates before calling ProcessFirstTouch (they get cleared)
		int x = firstFingerDown.x;
		int y = firstFingerDown.y;
		ProcessFirstTouch(GEM_MB_MENU);

		if (currentGesture.type == GESTURE_NONE) {
			Control* focusCtrl = EvntManager->GetMouseFocusedControl();
			if (focusCtrl && focusCtrl->ControlType == IE_GUI_GAMECONTROL
				&& ((GameControl*)focusCtrl)->GetTargetMode() == TARGET_MODE_NONE) {

				BeginMultiGesture(GESTURE_FORMATION_ROTATION);
			} else {
				EvntManager->MouseUp(x, y, GEM_MB_MENU, GetModState());
				ignoreNextFingerUp = 1;
			}
		}
	}

	return SDLVideoDriver::PollEvents();
}

void SDL20VideoDriver::ClearFirstTouch()
{
	firstFingerDown = SDL_TouchFingerEvent();
	firstFingerDown.fingerId = -1;
	firstFingerDownTime = 0;
}

void SDL20VideoDriver::BeginMultiGesture(MultiGestureType type)
{
	assert(type != GESTURE_NONE);
	assert(currentGesture.type == GESTURE_NONE);
	// warning: we are assuming this is a "virgin" gesture initialized by EndGesture
	currentGesture.type = type;
	switch (type) {
		case GESTURE_FORMATION_ROTATION:
			currentGesture.endButton = GEM_MB_MENU;
			break;
		default:
			currentGesture.endButton = GEM_MB_ACTION;
			break;
	}
}

void SDL20VideoDriver::EndMultiGesture(bool success)
{
	if (success && currentGesture.type) {
		if (!currentGesture.endPoint.isempty()) {
			// dont send events for invalid coordinates
			// we assume this means the gesture doesnt want an up event
			EvntManager->MouseUp(currentGesture.endPoint.x,
								 currentGesture.endPoint.y,
								 currentGesture.endButton, GetModState());
		}
	}
	if (currentGesture.type) {
		GameControl* gc = core->GetGameControl();
		if (gc) {
			// FIXME: should this always happen?
			gc->ClearMouseState();
		}
	}

	currentGesture = MultiGesture();
	currentGesture.endPoint.empty();
}

bool SDL20VideoDriver::ProcessFirstTouch( int mouseButton )
{
	if (!(MouseFlags & MOUSE_DISABLED) && firstFingerDown.fingerId >= 0) {
		// do an actual mouse move first! this is important for things such as ground piles to work!
		// also ensure any referencing of the cursor is accurate
		MouseMovement(firstFingerDown.x, firstFingerDown.y);

		if (CursorIndex != VID_CUR_DRAG)
			CursorIndex = VID_CUR_DOWN;

		// no need to scale these coordinates. they were scaled previously for us.
		EvntManager->MouseDown( firstFingerDown.x, firstFingerDown.y,
								mouseButton, GetModState() );

		ClearFirstTouch();
		ignoreNextFingerUp--;
		return true;
	}
	return false;
}

int SDL20VideoDriver::ProcessEvent(const SDL_Event & event)
{
	Control* focusCtrl = NULL; //used for contextual touch events.
	static int numFingers = 0;
	static bool continuingGesture = false; // resets when numFingers changes

	// beware that this may be removed before all events it created are processed!
	SDL_Finger* finger0 = SDL_GetTouchFinger(event.tfinger.touchId, 0);

	if (finger0) {
		numFingers = SDL_GetNumTouchFingers(event.tfinger.touchId);
	}
	// need 2 separate tests.
	// sometimes finger0 will become null while we are still processig its touches
	if (numFingers) {
		focusCtrl = EvntManager->GetMouseFocusedControl();
	}

	bool ConsolePopped = core->ConsolePopped;

	// TODO: we need a method to process gestures when numFingers changes
	// some gestures would want to continue while some would want to end/abort
	// currently finger up clears the gesture and finger down does not
	// this is due to GESTURE_FORMATION_ROTATION being the only gesture we have at this time
	switch (event.type) {
		// For swipes only. gestures requireing pinch or rotate need to use SDL_MULTIGESTURE or SDL_DOLLARGESTURE
		case SDL_FINGERMOTION:
			if (currentGesture.type) {
				continuingGesture = false;
				break; // finger motion has no further power over multigestures
			}
			if (numFingers > 1) {
				ignoreNextFingerUp = numFingers;
			}

			if (numFingers == core->NumFingScroll
				|| (numFingers != core->NumFingKboard && (focusCtrl && focusCtrl->ControlType == IE_GUI_TEXTAREA))) {
				//any # of fingers != NumFingKBoard will scroll a text area
				if (focusCtrl && focusCtrl->ControlType == IE_GUI_TEXTAREA) {
					// if we are scrolling a text area we dont want the keyboard in the way
					HideSoftKeyboard();
				} else if (!focusCtrl || focusCtrl->ControlType == IE_GUI_BUTTON) {
					// ensure the control we touched becomes focused before attempting to scroll it.
					// we cannot safely call ProcessFirstTouch anymore because now we process mouse events
					// this can result in a selection box being created

					EvntManager->MouseDown(ScaleCoordinateHorizontal(event.tfinger.x),
										   ScaleCoordinateVertical(event.tfinger.y),
										   GEM_MB_ACTION, 0);
					focusCtrl = EvntManager->GetMouseFocusedControl();
					if (focusCtrl && focusCtrl->ControlType == IE_GUI_GAMECONTROL) {
						((GameControl*)focusCtrl)->ClearMouseState();
					}
				}
				// invert the coordinates such that dragging down scrolls up etc.
				int scrollX = (event.tfinger.dx * width) * -1;
				int scrollY = (event.tfinger.dy * height) * -1;

				EvntManager->MouseWheelScroll( scrollX, scrollY );
			} else if (numFingers == core->NumFingKboard && !continuingGesture) {
				int delta = (int)(event.tfinger.dy * height) * -1;
				if (delta >= MIN_GESTURE_DELTA_PIXELS){
					// if the keyboard is already up interpret this gesture as console pop
					if( SDL_IsScreenKeyboardShown(window) && !ConsolePopped ) core->PopupConsole();
					else ShowSoftKeyboard();
				} else if (delta <= -MIN_GESTURE_DELTA_PIXELS) {
					HideSoftKeyboard();
				}
			} else if (numFingers == 1) { //click and drag
				if (!continuingGesture) {
					int x = event.tfinger.dx * width;
					int y = event.tfinger.dy * height;
					if ((x >= -MIN_GESTURE_DELTA_PIXELS
						&& x <= MIN_GESTURE_DELTA_PIXELS)
						||
						(y >= -MIN_GESTURE_DELTA_PIXELS
						&& y <= MIN_GESTURE_DELTA_PIXELS)) {
						break;
					} else /*if (focusCtrl && focusCtrl->ControlType != IE_GUI_GAMECONTROL)*/ {
						//break;
					}
					ProcessFirstTouch(GEM_MB_ACTION);
				}
				CursorIndex = VID_CUR_DRAG;

				// standard mouse movement
				MouseMovement(ScaleCoordinateHorizontal(event.tfinger.x),
							  ScaleCoordinateVertical(event.tfinger.y));
			}
			// we set this on finger motion because simple up/down are not part of gestures
			continuingGesture = true;
			break;
		case SDL_FINGERDOWN:
			if (!finger0) numFingers++;
			continuingGesture = false;

			if (numFingers == 1
				// this test is for when multiple fingers begin the first touch
				// commented out because we dont care right now, but if we need it i want it documented
				//|| (numFingers > 1 && firstFingerDown.fingerId < 0)
				) {
				lastMouseDownTime = EvntManager->GetRKDelay();
				if (ignoreNextFingerUp <= 0 && lastMouseDownTime != (unsigned long) ~0) {
					lastMouseDownTime += lastMouseDownTime + lastTime;
				}
				// do not send a mouseDown event. we delay firstTouch until we know more about the context.
				firstFingerDown = event.tfinger;
				firstFingerDownTime = GetTickCount();
				// ensure we get the coords for the actual first finger
				if (finger0) {
					firstFingerDown.x = ScaleCoordinateHorizontal(finger0->x);
					firstFingerDown.y = ScaleCoordinateVertical(finger0->y);
				} else {
					// rare case where the touch has
					// been removed before processing
					firstFingerDown.x = ScaleCoordinateHorizontal(event.tfinger.x);
					firstFingerDown.y = ScaleCoordinateVertical(event.tfinger.y);
				}
			} else if (currentGesture.type == GESTURE_NONE) {
				if (EvntManager && numFingers == core->NumFingInfo) {
					EvntManager->OnSpecialKeyPress( GEM_TAB );
					EvntManager->OnSpecialKeyPress( GEM_ALT );
				}
				if ((numFingers == core->NumFingScroll || numFingers == core->NumFingKboard)
					&& focusCtrl && focusCtrl->ControlType == IE_GUI_GAMECONTROL) {
					// scrolling cancels previous action
					((GameControl*)focusCtrl)->ClearMouseState();
				}
			}
			break;
		case SDL_FINGERUP:
			{
				if (numFingers) numFingers--;

				// we need to get mouseButton before calling ProcessFirstTouch
				int mouseButton = (firstFingerDown.fingerId >= 0 || continuingGesture == true) ? GEM_MB_ACTION : GEM_MB_MENU;
				continuingGesture = false;
				EndMultiGesture(true);

				if (numFingers == 0) { // this event was the last finger that was in contact
					ProcessFirstTouch(mouseButton);
					if (ignoreNextFingerUp <= 0) {
						ignoreNextFingerUp = 1; // set to one because we decrement unconditionally later
						if (CursorIndex != VID_CUR_DRAG)
							CursorIndex = VID_CUR_UP;
						// move cursor to ensure any referencing of the cursor is accurate
						MouseMovement(ScaleCoordinateHorizontal(event.tfinger.x),
									  ScaleCoordinateVertical(event.tfinger.y));

						EvntManager->MouseUp(ScaleCoordinateHorizontal(event.tfinger.x),
											 ScaleCoordinateVertical(event.tfinger.y),
											 mouseButton, GetModState());
					} else {
						focusCtrl = EvntManager->GetMouseFocusedControl();
						if (focusCtrl && focusCtrl->ControlType == IE_GUI_BUTTON)
							// these are repeat events so the control should stay pressed
							((Button*)focusCtrl)->SetState(IE_GUI_BUTTON_UNPRESSED);
					}
				}
				if (numFingers != core->NumFingInfo) {
					// FIXME: this is "releasing" the ALT key even when it hadn't been previously "pushed"
					// this isn't causing a problem currently
					EvntManager->KeyRelease( GEM_ALT, 0 );
				}
				ignoreNextFingerUp--;
			}
			break;
		case SDL_MULTIGESTURE:// use this for pinch or rotate gestures. see also SDL_DOLLARGESTURE
			{
				/* formation rotation gesture:
				first touch with a single finger to obtain the pivot
				then touch and drag with a second finger (while maintaining contact with first)
				to move the application point
				*/
				GameControl* gc = (GameControl*)focusCtrl;
				switch (currentGesture.type) {
					case GESTURE_FORMATION_ROTATION:
					{
						SDL_Finger* secondFinger = SDL_GetTouchFinger(event.mgesture.touchId, 1);
						if (secondFinger && gc && gc->GetTargetMode() == TARGET_MODE_NONE) {
							int x = ScaleCoordinateHorizontal(secondFinger->x);// + Viewport.x;
							int y = ScaleCoordinateVertical(secondFinger->y);// + Viewport.y;
							gc->OnMouseOver(x, y);
							currentGesture.endPoint = Point(x, y);
						}
						break;
					}
					case GESTURE_NONE:
					default:
						break;
				}
			}
			break;
		case SDL_MOUSEWHEEL:
			/*
			 TODO: need a preference for inverting these
			 sdl 2.0.4 autodetects (SDL_MOUSEWHEEL_FLIPPED in SDL_MouseWheelEvent)
			 */
			short scrollX;
			scrollX= event.wheel.x * -1;
			short scrollY;
			scrollY= event.wheel.y * -1;
			EvntManager->MouseWheelScroll( scrollX, scrollY );
			break;
		/* not user input events */
		case SDL_TEXTINPUT:
			for (size_t i=0; i < strlen(event.text.text); i++) {
				if (core->ConsolePopped)
					core->console->OnKeyPress( event.text.text[i], GetModState(event.key.keysym.mod));
				else
					EvntManager->KeyPress( event.text.text[i], GetModState(event.key.keysym.mod));
			}
			break;
		/* not user input events */
		case SDL_WINDOWEVENT://SDL 1.2
			switch (event.window.event) {
				case SDL_WINDOWEVENT_MINIMIZED://SDL 1.3
					// We pause the game and audio when the window is minimized.
					// on iOS/Android this happens when leaving the application or when play is interrupted (ex phone call)
					// if win/mac/linux has a problem with this behavior we can work something out.
					core->GetAudioDrv()->Pause();//this is for ANDROID mostly
					core->SetPause(PAUSE_ON);
					break;
				case SDL_WINDOWEVENT_RESTORED: //SDL 1.3
					/*
					 reset all input variables as if no events have happened yet
					 restoring from "minimized state" should be a clean slate.
					 */
					ClearFirstTouch();
					EndMultiGesture();
					ignoreNextFingerUp = 0;
					// should we reset the lastMouseTime vars?
#if TARGET_OS_IPHONE
					// FIXME:
					// sleep for a short while to avoid some unknown Apple threading issue with OpenAL threads being suspended
					// even using Apple examples of how to properly suspend an OpenAL context and resume on iOS are falling flat
					// it could be this bug affects only the simulator.
					sleep(1);
#endif
					core->GetAudioDrv()->Resume();//this is for ANDROID mostly
					break;
					/*
				case SDL_WINDOWEVENT_RESIZED: //SDL 1.2
					// this event exists in SDL 1.2, but this handler is only getting compiled under 1.3+
					Log(WARNING, "SDL 2 Driver",  "Window resized so your window surface is now invalid.");
					break;
					 */
			}
			break;
		// conditionally handle mouse events
		// discard them if they are produced by touch events
		// do NOT discard mouse wheel events
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if (event.button.which == SDL_TOUCH_MOUSEID) {
				break;
			}
			else {
				/**
				 * As being SDL2-only, try to query the clipboard state to
				 * paste when middle clicking the mouse.
				 */
				if (
					   event.button.button == SDL_BUTTON_MIDDLE
					&& event.type == SDL_MOUSEBUTTONDOWN
					&& SDL_HasClipboardText()
				) {
					char *pasteValue = SDL_GetClipboardText();

					if (pasteValue != NULL) {
						String *pasteValueString = StringFromCString(pasteValue);
						core->RequestPasting(*pasteValueString);
						delete pasteValueString;
						SDL_free(pasteValue);
					}

					break;
				} else {
					// we do not want these events to cascade down to SDL_KEYDOWN, so we return here instead of at default .
					return SDLVideoDriver::ProcessEvent(event);
				}
			}
		case SDL_KEYDOWN:
			{
				SDL_Keycode key = SDL_GetKeyFromScancode(event.key.keysym.scancode);
				if (key == SDLK_SPACE && SDL_GetModState() & KMOD_CTRL) {
					// special treatment: console popping is the only KEYDOWN event in SDLVideoDriver::ProcessEvent that uses a standard key (and therefore will never be hit). Therefore, implement this here also.
					core->PopupConsole();
					break;
				}
				if (key == SDLK_LSHIFT || key == SDLK_RSHIFT || key == SDLK_LCTRL || key == SDLK_RCTRL) {
					// if key is literally just ctrl or shift -- skip it.
					break;
				}
				if (SDL_GetModState() & KMOD_NUM && key >= SDLK_KP_DIVIDE && key <= SDLK_KP_EQUALS && key != SDLK_KP_ENTER) {
					// ignore numpad keys (handled by SDL_TEXTINPUT) if KMOD_NUM. Never ignore numpad enter.
					break;
				}
				if (key >= 32 && key < 127) {
					// ignore keys that generate text (these are handeled by SDL_TEXTINPUT).
					break;
				}
			}
			// fall through
		case SDL_KEYUP: // we let SDL_KEYUP pass directly to SDLVideo below, since SDL_TEXTINPUT feeds input directly as if it were pressed/keydown.
		default:
			return SDLVideoDriver::ProcessEvent(event);
	}
	return GEM_OK;
}

/*
 This method is intended for devices with no physical keyboard or with an optional soft keyboard (iOS/Android)
 */
void SDL20VideoDriver::HideSoftKeyboard()
{
	if(core->UseSoftKeyboard){
		SDL_StopTextInput();
		if(core->ConsolePopped) core->PopupConsole();
	}
}

/*
 This method is intended for devices with no physical keyboard or with an optional soft keyboard (iOS/Android)
 */
void SDL20VideoDriver::ShowSoftKeyboard()
{
	if(core->UseSoftKeyboard){
		SDL_StartTextInput();
	}
}

/* no idea how elaborate this should be*/
void SDL20VideoDriver::MoveMouse(unsigned int x, unsigned int y)
{
	SDL_WarpMouseInWindow(window, x, y);
}

void SDL20VideoDriver::SetGamma(int brightness, int /*contrast*/)
{
	// FIXME: hardcoded hack. in in Interface our default brigtness value is 10
	// so we assume that to be "normal" (1.0) value.
	SDL_SetWindowBrightness(window, (float)brightness/10.0);
}

bool SDL20VideoDriver::SetFullscreenMode(bool set)
{
	Uint32 flags = 0;
	if (set) {
	flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	if (SDL_SetWindowFullscreen(window, flags) == GEM_OK) {
		fullscreen = set;
		return true;
	}
	return false;
}

bool SDL20VideoDriver::ToggleGrabInput()
{
	bool isGrabbed = SDL_GetWindowGrab(window);
	SDL_SetWindowGrab(window, (SDL_bool)!isGrabbed);
	return (isGrabbed != SDL_GetWindowGrab(window));
}

// Private methods

bool SDL20VideoDriver::SetSurfaceAlpha(SDL_Surface* surface, unsigned short alpha)
{
	bool ret = SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
	if (ret == GEM_OK) {
		ret = SDL_SetSurfaceAlphaMod(surface, alpha);
	}
	if (ret == GEM_OK) {
		SDL_SetSurfaceRLE(surface, SDL_TRUE);
	}
	return ret;
}

float SDL20VideoDriver::ScaleCoordinateHorizontal(float x)
{
	float scaleX;
	SDL_RenderGetScale(renderer, &scaleX, NULL);

	int winW, winH;
	float xoffset = 0.0;
	SDL_GetWindowSize(window, &winW, &winH);
	float winWf = winW, winHf = winH;
	// only need to scale if they do not have the same ratio
	if ((winWf / winHf) != ((float)width / height)) {
		xoffset = ((winWf - (width * scaleX)) / 2) / winWf;
		return ((x - xoffset) * (winWf / scaleX));
	}
	return x * width;
}

float SDL20VideoDriver::ScaleCoordinateVertical(float y)
{
	float scaleY;
	SDL_RenderGetScale(renderer, NULL, &scaleY);

	int winW, winH;
	float yoffset = 0.0;
	SDL_GetWindowSize(window, &winW, &winH);
	float winWf = winW, winHf = winH;
	// only need to scale if they do not have the same ratio
	if ((winWf / winHf) != ((float)width / height)) {
		yoffset = ((winHf - (height * scaleY)) / 2) / winHf;
		return ((y - yoffset) * (winHf / scaleY));
	}
	return y * height;
}

// note from upstream: on some platforms a device may become seen only after use
bool SDL20VideoDriver::TouchInputEnabled() const
{
	return SDL_GetNumTouchDevices() > 0;
}

#ifndef USE_OPENGL
#include "plugindef.h"

GEMRB_PLUGIN(0xDBAAB51, "SDL2 Video Driver")
PLUGIN_DRIVER(SDL20VideoDriver, "sdl")
END_PLUGIN()
#endif
