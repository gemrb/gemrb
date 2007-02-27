/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "DirectXVideoDriver.h"
#include "../Core/Interface.h"
#include <math.h>
#include <windows.h>
#include <commctrl.h>
#include "Poly.h"

HINSTANCE hInst;
LPDIRECT3D9 lpD3D;
LPDIRECT3DDEVICE9 lpD3DDevice;
int ScreenWidth, ScreenHeight;

HWND hWnd;

/* The main Win32 event handler
DJM: This is no longer static as (DX5/DIB)_CreateWindow needs it
*/
LONG CALLBACK WinMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
			 {
			}
			break;

		case WM_KEYUP:
			 {
			}
			break;

		case WM_KEYDOWN:
			 {
				if (( lParam & ( 1 << 32 ) ) == 1)
					break;
				int key = -1;
				switch (( int ) wParam) {
					case VK_ESCAPE:
						core->PopupConsole();
						break;

					case VK_BACKSPACE:
						key = GEM_BACKSP;
						break;

					case VK_RETURN:
						key = GEM_RETURN;
						break;

					case VK_DELETE:
						key = GEM_DELETE;
						break;

					case VK_LEFT:
						key = GEM_LEFT;
						break;

					case VK_RIGHT:
						key = GEM_RIGHT;
						break;
				}
				break;
				if (key != -1) {
					if (!core->ConsolePopped)
						core->GetEventMgr()->OnSpecialKeyPress( key );
					else
						core->console->OnSpecialKeyPress( key );
				}
			}
			break;

		case WM_ACTIVATEAPP:
			 {
				bool fActive = ( BOOL ) wParam;   	 // activation flag 
				if (fActive)
					SetCapture( hWnd );
				else
					ReleaseCapture();
			}
			break;

		case WM_CHAR:
			 {
				unsigned char key = ( unsigned char ) wParam;
				if (( key == 0 ) || ( key == 13 )) {
					switch (key) {
						case VK_DELETE:
							key = GEM_DELETE;
							break;

						case VK_RETURN:
							key = GEM_RETURN;
							break;
					}
					if (!core->ConsolePopped)
						core->GetEventMgr()->OnSpecialKeyPress( key );
					else
						core->console->OnSpecialKeyPress( key );
					break;
				}
				if (!core->ConsolePopped)
					core->GetEventMgr()->KeyPress( key, 0 );
				else
					core->console->OnKeyPress( key, 0 );
			}
			return 0;

		case WM_MOUSEMOVE:
			 {
				unsigned short xPos = LOWORD( lParam );  // horizontal position of cursor 
				unsigned short yPos = HIWORD( lParam );  // vertical position of cursor
				if (!core->ConsolePopped)
					core->GetEventMgr()->MouseMove( xPos, yPos );
			}
			return 0;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
			 {
				unsigned long  fwKeys = wParam; 	   // key flags 
				unsigned short xPos = LOWORD( lParam );  // horizontal position of cursor 
				unsigned short yPos = HIWORD( lParam );  // vertical position of cursor
				unsigned char button = 0;
				unsigned short Mod = 0;
				if (fwKeys & MK_LBUTTON)
					button |= 0x01;
				if (fwKeys & MK_MBUTTON)
					button |= 0x02;
				if (fwKeys & MK_RBUTTON)
					button |= 0x04;
				if (fwKeys & MK_SHIFT)
					Mod |= 0x01;
				if (fwKeys & MK_CONTROL)
					Mod |= 0x02;
				if (!core->ConsolePopped)
					core->GetEventMgr()->MouseDown( xPos, yPos, button, Mod );
			}
			return 0;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
			 {
				unsigned long  fwKeys = wParam; 	   // key flags 
				unsigned short xPos = LOWORD( lParam );  // horizontal position of cursor 
				unsigned short yPos = HIWORD( lParam );  // vertical position of cursor
				unsigned char button = 0;
				unsigned short Mod = 0;
				if (fwKeys & MK_LBUTTON)
					button |= 0x01;
				if (fwKeys & MK_MBUTTON)
					button |= 0x02;
				if (fwKeys & MK_RBUTTON)
					button |= 0x04;
				if (fwKeys & MK_SHIFT)
					Mod |= 0x01;
				if (fwKeys & MK_CONTROL)
					Mod |= 0x02;
				if (!core->ConsolePopped)
					core->GetEventMgr()->MouseUp( xPos, yPos, button, Mod );
			}
			return 0;

			/* Don't allow screen savers or monitor power downs.
								   This is because they quietly clear DirectX surfaces.
								   It would be better to allow the application to
								   decide whether or not to blow these off, but the
								   semantics of SDL_PrivateSysWMEvent() don't allow
								   the application that choice.
								 */
		case WM_SYSCOMMAND:
			 {
				if (( wParam & 0xFFF0 ) == SC_SCREENSAVE ||
					( wParam & 0xFFF0 ) == SC_MONITORPOWER)
					return( 0 );
			}
	}
	return( DefWindowProc( hwnd, msg, wParam, lParam ) );
}

DirectXVideoDriver::DirectXVideoDriver(void)
{
	sceneBegin = false;
	quit = 0;
}

DirectXVideoDriver::~DirectXVideoDriver(void)
{
	if (lpD3DDevice) {
		lpD3DDevice->Release();
	}
	if (lpD3D) {
		lpD3D->Release();
	}
}

int DirectXVideoDriver::Init(void)
{
	WNDCLASS classe;
#ifdef WM_MOUSELEAVE
	HMODULE handle;
#endif

	winClassName = ( char * ) malloc( 6 );
	strcpy( winClassName, "GemRB" );

	/* Register the application class */
	classe.hCursor = NULL;
	classe.hIcon = ( HICON ) LoadImage( hInst, winClassName, IMAGE_ICON, 0, 0,
								LR_DEFAULTCOLOR );
	classe.lpszMenuName = NULL;
	classe.lpszClassName = winClassName;
	classe.hbrBackground = NULL;
	classe.hInstance = hInst;
	classe.style = CS_BYTEALIGNCLIENT;
	classe.lpfnWndProc = WinMessage;
	classe.cbWndExtra = 0;
	classe.cbClsExtra = 0;
	if (!RegisterClass( &classe )) {
		printf( "Couldn't register application class" );
		return GEM_ERROR;
	}

#ifdef WM_MOUSELEAVE
	/* Get the version of TrackMouseEvent() we use */
	/*_TrackMouseEvent = NULL;
	handle = GetModuleHandle("USER32.DLL");
	if ( handle ) {
		_TrackMouseEvent = (BOOL (WINAPI *)(TRACKMOUSEEVENT *))GetProcAddress(handle, "TrackMouseEvent");
	}
	if ( _TrackMouseEvent == NULL ) {
		_TrackMouseEvent = NULL;//WIN_TrackMouseEvent;
	}*/
#endif /* WM_MOUSELEAVE */

	hWnd = CreateWindow( winClassName, winClassName, ( WS_OVERLAPPED ),//|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX),
			0, 0, 0, 0, NULL, NULL, hInst, NULL );
	if (hWnd == NULL) {
		printf( "Couldn't create window" );
		return GEM_ERROR;
	}
	ShowWindow( hWnd, SW_HIDE );

	RECT bounds;

	bounds.top = 0;
	bounds.bottom = core->Height;
	bounds.left = 0;
	bounds.right = core->Width;
	AdjustWindowRectEx( &bounds, GetWindowLong( hWnd, GWL_STYLE ), FALSE, 0 );

	MoveWindow( hWnd, 0, 0, bounds.right - bounds.left - 1,
		bounds.bottom - bounds.top - 1, true );

	lpD3D = Direct3DCreate9( D3D_SDK_VERSION );

	D3DDISPLAYMODE d3ddm;
	lpD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );

	D3DPRESENT_PARAMETERS d3dpp;
	memset( &d3dpp, 0, sizeof( d3dpp ) );
	d3dpp.Windowed = true;
	d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dpp.BackBufferCount = 1;
	d3dpp.BackBufferFormat = d3ddm.Format;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	lpD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &lpD3DDevice );

	lpD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	lpD3DDevice->SetRenderState( D3DRS_LIGHTING, false );
	lpD3DDevice->SetRenderState( D3DRS_ZENABLE, true );

	lpD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, true );
	lpD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	lpD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

	lpD3DDevice->SetFVF( D3DFVF_CUSTOMVERTEX );

	return GEM_OK;
}

int DirectXVideoDriver::CreateDisplay(int width, int height, int bpp,
	bool fullscreen)
{
	HWND hDesktop = GetDesktopWindow();
	RECT rect;
	GetWindowRect( hDesktop, &rect );

	RECT bounds;

	bounds.top = 0;
	bounds.bottom = core->Height;
	bounds.left = 0;
	bounds.right = core->Width;
	AdjustWindowRectEx( &bounds, GetWindowLong( hWnd, GWL_STYLE ), FALSE, 0 );

	MoveWindow( hWnd,
		( rect.right / 2 ) - ( ( bounds.right - bounds.left ) / 2 ),
		( rect.bottom / 2 ) - ( ( bounds.bottom - bounds.top ) / 2 ),
		bounds.right - bounds.left - 1, bounds.bottom - bounds.top - 1, true );

	ShowWindow( hWnd, SW_SHOW );

	ScreenWidth = width;
	ScreenHeight = height;
	Viewport.x = Viewport.y = 0;
	Viewport.w = width;
	Viewport.h = height;

	lpD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
					D3DCOLOR_XRGB( 0, 0, 0 ), 0.0f, 0L );

	return GEM_OK;
}

VideoModes DirectXVideoDriver::GetVideoModes(bool fullscreen)
{
	VideoModes vm;
	return vm;
}

bool DirectXVideoDriver::TestVideoMode(VideoMode& vm)
{
	return false;
}

int DirectXVideoDriver::SwapBuffers(void)
{
	if (!sceneBegin) {
		lpD3DDevice->BeginScene();
		sceneBegin = true;
	}

	if (core->ConsolePopped) {
		core->DrawConsole();
	}

	lpD3DDevice->EndScene();
	sceneBegin = false;

	lpD3DDevice->Present( NULL, NULL, NULL, NULL );

	MSG msg; 
	while (PeekMessage( &msg, hWnd, 0, 0, PM_REMOVE )) {
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	return quit;
}

Sprite2D* DirectXVideoDriver::CreateSprite(int w, int h, int bpp, DWORD rMask,
	DWORD gMask, DWORD bMask, DWORD aMask, void* pixels, bool cK, int index)
{
	Sprite2D* spr = new Sprite2D();

	Poly* p = new Poly();
	Texture* t = new Texture( pixels, w, h, 32, NULL, cK, index );
	p->m_Texture = t;
	spr->vptr = p;
	spr->pixels = pixels;
	spr->Width = w;
	spr->Height = h;

	return spr;
}

Sprite2D* DirectXVideoDriver::CreateSprite8(int w, int h, int bpp,
	void* pixels, void* palette, bool cK, int index)
{
	Sprite2D* spr = new Sprite2D();

	Poly* p = new Poly();
	Texture* t = new Texture( pixels, w, h, 8, palette, cK, index );
	p->m_Texture = t;
	spr->vptr = p;
	spr->pixels = pixels;
	spr->Width = w;
	spr->Height = h;

	return spr;
}

void DirectXVideoDriver::FreeSprite(Sprite2D* spr)
{
	if (spr->vptr) {
		delete( spr->vptr );
	}
	if (spr->pixels) {
		delete( spr->pixels );
	}
}

void DirectXVideoDriver::BlitSprite(Sprite2D* spr, int x, int y, bool anchor,
	Region* clip)
{
	if (!sceneBegin) {
		lpD3DDevice->BeginScene();
		sceneBegin = true;
	}
	Poly* p = ( Poly* ) spr->vptr;

	Region rgn( x - spr->XPos, y - spr->YPos, spr->Width, spr->Height );
	if (!anchor) {
		rgn.x -= Viewport.x;
		rgn.y -= Viewport.y;
	}

	if (clip) {
		if (rgn.x >= clip->x + clip->w)
			return;
		if (rgn.y >= clip->y + clip->h)
			return;
		if (rgn.x + rgn.w <= clip->x)
			return;
		if (rgn.y + rgn.h <= clip->y)
			return;
	}

	if (anchor) {
		p->SetVertRect( x - spr->XPos, y - spr->YPos, spr->Width, spr->Height,
			0.0f );
	} else {
		p->SetVertRect( x - spr->XPos - Viewport.x,
			y - spr->YPos - Viewport.y, spr->Width, spr->Height, 0.0f );
	}

	lpD3DDevice->SetTexture( 0, p->m_Texture->pTexture );
	lpD3DDevice->SetStreamSource( 0, p->m_VertexBuffer, 0,
					sizeof( CUSTOMVERTEX ) );
	lpD3DDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );
	//p->m_VertexBuffer->Unlock();
}

void DirectXVideoDriver::SetCursor(Sprite2D* spr, int x, int y)
{
	//TODO: Implement Cursor
	return;
}

Region DirectXVideoDriver::GetViewport()
{
	return Viewport;
}

void DirectXVideoDriver::SetViewport(int x, int y)
{
	Viewport.x = x;
	Viewport.y = y;
}

void DirectXVideoDriver::MoveViewportTo(int x, int y)
{
	Viewport.x = x - Viewport.w;
	Viewport.y = y - Viewport.h;
}
/** No descriptions */
void DirectXVideoDriver::SetPalette(Sprite2D* spr, Color* pal)
{
	Poly* p = ( Poly* ) spr->vptr;
	p->m_Texture->Init( spr->pixels, spr->Width, spr->Height, 8,
					( void * ) pal, p->m_Texture->hasCK,
					p->m_Texture->colorKey );
	//lpD3DDevice->SetPaletteEntries(p->m_Texture->paletteIndex, (PALETTEENTRY*)pal);
}

void DirectXVideoDriver::ConvertToVideoFormat(Sprite2D* sprite)
{
	return;
}

#define MINCOL 2
#define MUL    2

void DirectXVideoDriver::CalculateAlpha(Sprite2D* sprite)
{
}

/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
void DirectXVideoDriver::DrawRect(Region& rgn, Color& color)
{
	CUSTOMVERTEX pVertices[4];

	float left, top, right, bottom, d = 0.0f;
	left = ( ( float ) rgn.x / ( float ) ScreenWidth ) * 2.0f - 1.0f;
	right = ( ( float ) ( rgn.x + rgn.w ) / ( float ) ScreenWidth ) * 2.0f -
		1.0f;
	bottom = ( ( float ) rgn.y / ( float ) ScreenHeight ) * 2.0f - 1.0f;
	top = ( ( float ) ( rgn.y + rgn.h ) / ( float ) ScreenHeight ) * 2.0f -
		1.0f;
	top = -top;
	bottom = -bottom;

	pVertices[0].x = left;
	pVertices[0].y = bottom;
	pVertices[0].z = d;
	pVertices[0].color = D3DCOLOR_ARGB( 255 - color.a, color.r, color.b,
							color.g );
	pVertices[0].tu = 0.0;
	pVertices[0].tv = 0.0;
	pVertices[1].x = right;
	pVertices[1].y = bottom;
	pVertices[1].z = d;
	pVertices[1].color = D3DCOLOR_ARGB( 255 - color.a, color.r, color.b,
							color.g );
	pVertices[1].tu = 1.0;
	pVertices[1].tv = 0.0;
	pVertices[2].x = left;
	pVertices[2].y = top;
	pVertices[2].z = d;
	pVertices[2].color = D3DCOLOR_ARGB( 255 - color.a, color.r, color.b,
							color.g );
	pVertices[2].tu = 0.0;
	pVertices[2].tv = 1.0;
	pVertices[3].x = right;
	pVertices[3].y = top;
	pVertices[3].z = d;
	pVertices[3].color = D3DCOLOR_ARGB( 255 - color.a, color.r, color.b,
							color.g );
	pVertices[3].tu = 1.0;
	pVertices[3].tv = 1.0;

	if (!sceneBegin) {
		lpD3DDevice->BeginScene();
		sceneBegin = true;
	}

	lpD3DDevice->SetTexture( 0, NULL );
	lpD3DDevice->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2,
					( void * ) pVertices, sizeof( CUSTOMVERTEX ) );
}

/** Frees a Palette */
void DirectXVideoDriver::FreePalette(Color *&palette)
{
	if(palette) {
		free(palette);
		palette = NULL;
	}
}
/** Creates a Palette from Color */
Color* DirectXVideoDriver::CreatePalette(Color color, Color back)
{
	Color* pal = ( Color* ) malloc( 256 * sizeof( Color ) );
	pal[0].r = 0;
	pal[0].g = 0xff;
	pal[0].b = 0;
	pal[0].a = 0;
	for (int i = 1; i < 256; i++) {
		pal[i].r = back.r +
			( unsigned char ) ( ( ( color.r - back.r ) * ( i ) ) / 255.0 );
		pal[i].g = back.g +
			( unsigned char ) ( ( ( color.g - back.g ) * ( i ) ) / 255.0 );
		pal[i].b = back.b +
			( unsigned char ) ( ( ( color.b - back.b ) * ( i ) ) / 255.0 );
		pal[i].a = 0;
	}
	return pal;
}
/** Blits a Sprite filling the Region */
void DirectXVideoDriver::BlitTiled(Region rgn, Sprite2D* img, bool anchor)
{
}
/** Send a Quit Signal to the Event Queue */
bool DirectXVideoDriver::Quit()
{
	quit = 1;
	return true;
}
/** Get the Palette of a Sprite */
Color* DirectXVideoDriver::GetPalette(Sprite2D* spr)
{
	Color* pal = ( Color* ) malloc( 256 * sizeof( Color ) );
	memcpy( pal, ( ( ( Poly * ) spr->vptr )->m_Texture->palette ),
		256 * sizeof( Color ) );
	return pal;
}

