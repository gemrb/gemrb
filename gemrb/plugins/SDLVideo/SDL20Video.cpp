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

	// touch input
	ignoreNextFingerUp = 0;
	ClearFirstTouch();
	EndMultiGesture();
}

SDL20VideoDriver::~SDL20VideoDriver(void)
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

int SDL20VideoDriver::CreateDriverDisplay(const Size& s, int bpp, const char* title)
{
	screenSize = s;
	this->bpp = bpp;

	Log(MESSAGE, "SDL 2 Driver", "Creating display");
	// TODO: scale methods can be nearest or linear, and should be settable in config
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	Uint32 winFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#if TARGET_OS_IPHONE || ANDROID
	// this allows the user to flip the device upsidedown if they wish and have the game rotate.
	// it also for some unknown reason is required for retina displays
	winFlags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS;
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
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, s.w, s.h, winFlags);
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
	SDL_RenderSetLogicalSize(renderer, screenSize.w, screenSize.h);
	SDL_GetRendererOutputSize(renderer, &screenSize.w, &screenSize.h);

	return GEM_OK;
}

VideoBuffer* SDL20VideoDriver::NewVideoBuffer(const Region& r, BufferFormat fmt)
{
	Uint32 format = SDLPixelFormatFromBufferFormat(fmt);
	if (format == SDL_PIXELFORMAT_UNKNOWN)
		return NULL;
	
	SDL_Texture* tex = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, r.w, r.h);
	return new SDLTextureVideoBuffer(r.Origin(), tex, fmt, renderer);
}

void SDL20VideoDriver::SwapBuffers(VideoBuffers& buffers)
{
	VideoBuffers::iterator it;
	it = buffers.begin();
	for (; it != buffers.end(); ++it) {
		(*it)->RenderOnDisplay(renderer);
	}

	SDL_RenderPresent( renderer );
}

Sprite2D* SDL20VideoDriver::GetScreenshot( Region /*r*/ )
{
	return NULL;
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
			/*
			EvntManager->MouseUp(currentGesture.endPoint.x,
								 currentGesture.endPoint.y,
								 currentGesture.endButton, GetModState(SDL_GetModState()) );
			 */
		}
	}

	currentGesture = MultiGesture();
	currentGesture.endPoint.empty();
}

bool SDL20VideoDriver::ProcessFirstTouch( int /*mouseButton*/ )
{
	/*
	if (!(MouseFlags & MOUSE_DISABLED) && firstFingerDown.fingerId >= 0) {
		// do an actual mouse move first! this is important for things such as ground piles to work!
		// also ensure any referencing of the cursor is accurate
		MouseMovement(firstFingerDown.x, firstFingerDown.y);

		// no need to scale these coordinates. they were scaled previously for us.
		EvntManager->MouseDown( firstFingerDown.x, firstFingerDown.y,
								mouseButton, GetModState() );

		ClearFirstTouch();
		ignoreNextFingerUp--;
		return true;
	}
	 */
	return false;
}

int SDL20VideoDriver::ProcessEvent(const SDL_Event & event)
{
	Control* focusCtrl = NULL; //used for contextual touch events.
	static int numFingers = 0;
	// static bool continuingGesture = false; // resets when numFingers changes

	// beware that this may be removed before all events it created are processed!
	SDL_Finger* finger0 = SDL_GetTouchFinger(event.tfinger.touchId, 0);

	if (finger0) {
		numFingers = SDL_GetNumTouchFingers(event.tfinger.touchId);
	}
	// need 2 separate tests.
	// sometimes finger0 will become null while we are still processig its touches
	if (numFingers) {
		focusCtrl = NULL;//EvntManager->GetFocusedControl();
	}

	// TODO: we need a method to process gestures when numFingers changes
	// some gestures would want to continue while some would want to end/abort
	// currently finger up clears the gesture and finger down does not
	// this is due to GESTURE_FORMATION_ROTATION being the only gesture we have at this time
	switch (event.type) {
		// For swipes only. gestures requireing pinch or rotate need to use SDL_MULTIGESTURE or SDL_DOLLARGESTURE
		case SDL_FINGERMOTION:
			if (currentGesture.type) {
				// continuingGesture = false;
				break; // finger motion has no further power over multigestures
			}
			if (numFingers > 1) {
				ignoreNextFingerUp = numFingers;
			}
			break;
		case SDL_FINGERDOWN:
			if (!finger0) numFingers++;
			// continuingGesture = false;

			if (numFingers == 1
				// this test is for when multiple fingers begin the first touch
				// commented out because we dont care right now, but if we need it i want it documented
				//|| (numFingers > 1 && firstFingerDown.fingerId < 0)
				) {
				/*lastMouseDownTime = EvntManager->GetRKDelay();
				if (ignoreNextFingerUp <= 0 && lastMouseDownTime != (unsigned long) ~0) {
					lastMouseDownTime += lastMouseDownTime + lastTime;
				}*/
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
					//EvntManager->OnSpecialKeyPress( GEM_TAB );
					//EvntManager->OnSpecialKeyPress( GEM_ALT );
				}
				if ((numFingers == core->NumFingScroll || numFingers == core->NumFingKboard)
					/*&& focusCtrl && focusCtrl->ControlType == IE_GUI_GAMECONTROL*/) {
					// scrolling cancels previous action
				}
			}
			break;
		case SDL_FINGERUP:
			{
				if (numFingers) numFingers--;

				// we need to get mouseButton before calling ProcessFirstTouch
				// int mouseButton = (firstFingerDown.fingerId >= 0 || continuingGesture == true) ? GEM_MB_ACTION : GEM_MB_MENU;
				// continuingGesture = false;
				EndMultiGesture(true);
				/*
				if (numFingers == 0) { // this event was the last finger that was in contact
					ProcessFirstTouch(mouseButton);
					if (ignoreNextFingerUp <= 0) {
						ignoreNextFingerUp = 1; // set to one because we decrement unconditionally later

						// move cursor to ensure any referencing of the cursor is accurate
						MouseMovement(ScaleCoordinateHorizontal(event.tfinger.x),
									  ScaleCoordinateVertical(event.tfinger.y));

						EvntManager->MouseUp(ScaleCoordinateHorizontal(event.tfinger.x),
											 ScaleCoordinateVertical(event.tfinger.y),
											 mouseButton, GetModState());
					} else {
						focusCtrl = EvntManager->GetFocusedControl();
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
				*/
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
			}
			break;
		case SDL_MOUSEWHEEL:
			/*
			 TODO: need a preference for inverting these
			 sdl 2.0.4 autodetects (SDL_MOUSEWHEEL_FLIPPED in SDL_MouseWheelEvent)
			 */
			/*short scrollX;
			scrollX= event.wheel.x * -1;
			short scrollY;
			scrollY= event.wheel.y * -1;*/
			//EvntManager->MouseWheelScroll( scrollX, scrollY );
			break;
		/* not user input events */
		case SDL_TEXTINPUT:
			for (size_t i=0; i < strlen(event.text.text); i++) {
				//EvntManager->KeyPress( event.text.text[i], GetModState(event.key.keysym.mod));
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
	/*
	if(core->UseSoftKeyboard){
		SDL_StopTextInput();
		if(core->ConsolePopped) core->PopupConsole();
	}
	*/
}

/*
 This method is intended for devices with no physical keyboard or with an optional soft keyboard (iOS/Android)
 */
void SDL20VideoDriver::ShowSoftKeyboard()
{
	/*
	if(core->UseSoftKeyboard){
		SDL_StartTextInput();
	}
	*/
}

bool SDL20VideoDriver::TouchInputEnabled()
{
	// note from upstream: on some platforms a device may become seen only after use
	return SDL_GetNumTouchDevices() > 0;
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

float SDL20VideoDriver::ScaleCoordinateHorizontal(float /*x*/)
{
	float scaleX;
	SDL_RenderGetScale(renderer, &scaleX, NULL);

	int winW, winH;
	// float xoffset = 0.0;
	SDL_GetWindowSize(window, &winW, &winH);
	// float winWf = winW, winHf = winH;
	// only need to scale if they do not have the same ratio
	/*if ((winWf / winHf) != ((float)width / height)) {
		xoffset = ((winWf - (width * scaleX)) / 2) / winWf;
		return ((x - xoffset) * (winWf / scaleX));
	}*/
	return winW; //x * width;
}

float SDL20VideoDriver::ScaleCoordinateVertical(float /*y*/)
{
	float scaleY;
	SDL_RenderGetScale(renderer, NULL, &scaleY);

	int winW, winH;
	// float yoffset = 0.0;
	SDL_GetWindowSize(window, &winW, &winH);
	// float winWf = winW, winHf = winH;
	// only need to scale if they do not have the same ratio
	/* if ((winWf / winHf) != ((float)width / height)) {
		yoffset = ((winHf - (height * scaleY)) / 2) / winHf;
		return ((y - yoffset) * (winHf / scaleY));
	}*/
	return winH; //y * height;
}

#ifndef USE_OPENGL
#include "plugindef.h"


GEMRB_PLUGIN(0xDBAAB51, "SDL2 Video Driver")
PLUGIN_DRIVER(SDL20VideoDriver, "sdl")
END_PLUGIN()

#endif
