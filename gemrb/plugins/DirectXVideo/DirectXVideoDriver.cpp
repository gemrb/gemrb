#include "../../includes/win32def.h"
#include "DirectXVideoDriver.h"
#include "../Core/Interface.h"
#include <math.h>
#include <windows.h>
#include <commctrl.h>
#include "Poly.h"
#include "dinput.h"

HINSTANCE hInst;
LPDIRECT3D9				lpD3D;
LPDIRECT3DDEVICE9		lpD3DDevice;
int						ScreenWidth, 
						ScreenHeight;

LPDIRECTINPUT8			lpDI;
LPDIRECTINPUTDEVICE8	lpDIDevice;

/* The main Win32 event handler
DJM: This is no longer static as (DX5/DIB)_CreateWindow needs it
*/
LONG CALLBACK WinMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_KEYDOWN: {
			/* Ignore windows keyboard messages */;
		}
		return(0);

		/* Don't allow screen savers or monitor power downs.
		   This is because they quietly clear DirectX surfaces.
		   It would be better to allow the application to
		   decide whether or not to blow these off, but the
		   semantics of SDL_PrivateSysWMEvent() don't allow
		   the application that choice.
		 */
		case WM_SYSCOMMAND: {
			if ((wParam&0xFFF0)==SC_SCREENSAVE || 
			    (wParam&0xFFF0)==SC_MONITORPOWER)
				return(0);
		}
	}
	return(DefWindowProc(hwnd, msg, wParam, lParam));
}

DirectXVideoDriver::DirectXVideoDriver(void)
{
	sceneBegin = false;
}

DirectXVideoDriver::~DirectXVideoDriver(void)
{
	if( lpD3DDevice )
		lpD3DDevice->Release();
	if( lpD3D )
		lpD3D->Release();
	if( lpDIDevice )
		lpDIDevice->Release();
	if( lpDI )
		lpDI->Release();
}

int DirectXVideoDriver::Init(void)
{
	WNDCLASS classe;
#ifdef WM_MOUSELEAVE
	HMODULE handle;
#endif

	winClassName = (char*)malloc(6);
	strcpy(winClassName, "GemRB");

	/* Register the application class */
	classe.hCursor		 = NULL;
	classe.hIcon		 = (HICON)LoadImage(hInst, winClassName, IMAGE_ICON,
	                                    0, 0, LR_DEFAULTCOLOR);
	classe.lpszMenuName	 = NULL;
	classe.lpszClassName = winClassName;
	classe.hbrBackground = NULL;
	classe.hInstance	 = hInst;
	classe.style		 = CS_BYTEALIGNCLIENT;
	classe.lpfnWndProc	 = WinMessage;
	classe.cbWndExtra	 = 0;
	classe.cbClsExtra	 = 0;
	if ( ! RegisterClass(&classe) ) {
		printf("Couldn't register application class");
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

	hWnd = CreateWindow(winClassName, winClassName,
		(WS_OVERLAPPED),//|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX),
		0, 0, 0, 0, NULL, NULL, hInst, NULL);
	if ( hWnd == NULL ) {
		printf("Couldn't create window");
		return GEM_ERROR;
	}
	ShowWindow(hWnd, SW_HIDE);

	RECT bounds;

	bounds.top    = 0;
	bounds.bottom = core->Height;
	bounds.left   = 0;
	bounds.right  = core->Width;
	AdjustWindowRectEx(&bounds, GetWindowLong(hWnd, GWL_STYLE), FALSE, 0);

	MoveWindow(hWnd, 0, 0, bounds.right-bounds.left-1, bounds.bottom-bounds.top-1, true);

	lpD3D = Direct3DCreate9( D3D_SDK_VERSION );

	D3DDISPLAYMODE d3ddm;
	lpD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );

	D3DPRESENT_PARAMETERS d3dpp;
	memset(&d3dpp, 0, sizeof(d3dpp));
	d3dpp.Windowed = true;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dpp.BackBufferCount = 1;
	d3dpp.BackBufferFormat = d3ddm.Format;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	lpD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &lpD3DDevice);

	lpD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	lpD3DDevice->SetRenderState( D3DRS_LIGHTING, false );
	lpD3DDevice->SetRenderState( D3DRS_ZENABLE, true );

	lpD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, true );
	lpD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	lpD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

	lpD3DDevice->SetFVF( D3DFVF_CUSTOMVERTEX );

	//Direct3D Initialized. Now let's initialize DirectInput

	HRESULT ddrval = DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&lpDI, NULL);
	if(ddrval != DI_OK)
		return GEM_ERROR;

	ddrval = lpDI->CreateDevice(GUID_SysKeyboard, &lpDIDevice, NULL);
	if(ddrval != DI_OK)
		return GEM_ERROR;

	ddrval = lpDIDevice->SetDataFormat(&c_dfDIKeyboard);
	if(ddrval != DI_OK)
		return GEM_ERROR;

	ddrval = lpDIDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if(ddrval != DI_OK)
		return GEM_ERROR;

	lpDIDevice->Acquire();

	return GEM_OK;
}

int DirectXVideoDriver::CreateDisplay(int width, int height, int bpp, bool fullscreen)
{
	HWND hDesktop = GetDesktopWindow();
	RECT rect;
	GetWindowRect(hDesktop, &rect);
	
	RECT bounds;

	bounds.top    = 0;
	bounds.bottom = core->Height;
	bounds.left   = 0;
	bounds.right  = core->Width;
	AdjustWindowRectEx(&bounds, GetWindowLong(hWnd, GWL_STYLE), FALSE, 0);

	MoveWindow(hWnd, (rect.right/2)-((bounds.right-bounds.left)/2), (rect.bottom/2)-((bounds.bottom-bounds.top)/2), bounds.right-bounds.left-1, bounds.bottom-bounds.top-1, true);

	ShowWindow(hWnd, SW_SHOW);

	ScreenWidth = width;
	ScreenHeight = height;
	Viewport.x = Viewport.y = 0;
	Viewport.w = width;
	Viewport.h = height;

	lpD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 0.0f, 0L);

	return GEM_OK;
}

VideoModes DirectXVideoDriver::GetVideoModes(bool fullscreen)
{
	VideoModes vm;
	return vm;
}

bool DirectXVideoDriver::TestVideoMode(VideoMode & vm)
{
	return false;
}

int DirectXVideoDriver::SwapBuffers(void)
{
	if(!sceneBegin) {
		//lpD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L);
		lpD3DDevice->BeginScene();
		sceneBegin = true;
	}

	lpD3DDevice->EndScene();
	sceneBegin = false;

	lpD3DDevice->Present(NULL, NULL, NULL, NULL);

	MSG msg; 
	while( PeekMessage( &msg, hWnd, 0, 0, PM_REMOVE ) ) {
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	return 0;
}

Sprite2D *DirectXVideoDriver::CreateSprite(int w, int h, int bpp, DWORD rMask, DWORD gMask, DWORD bMask, DWORD aMask, void* pixels, bool cK, int index)
{
	Sprite2D * spr = new Sprite2D();

	Poly * p = new Poly();
	Texture * t = new Texture(pixels, w, h, 32, NULL, cK, index);
	p->m_Texture = t;
	spr->vptr = p;
	spr->pixels = pixels;
	spr->Width = w;
	spr->Height = h;

	return spr;
}

Sprite2D *DirectXVideoDriver::CreateSprite8(int w, int h, int bpp, void* pixels, void* palette, bool cK, int index)
{
 	Sprite2D * spr = new Sprite2D();

	Poly * p = new Poly();
	Texture * t = new Texture(pixels, w, h, 8, palette, cK, index);
	p->m_Texture = t;
	spr->vptr = p;
	spr->pixels = pixels;
	spr->Width = w;
	spr->Height = h;

	return spr;
}

void DirectXVideoDriver::FreeSprite(Sprite2D * spr)
{
	if(spr->vptr)
		delete(spr->vptr);
	if(spr->pixels)
		delete(spr->pixels);
}

void DirectXVideoDriver::BlitSprite(Sprite2D * spr, int x, int y, bool anchor, Region * clip)
{
	
	if(!sceneBegin) {
		//lpD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L);
		lpD3DDevice->BeginScene();
		sceneBegin = true;
	}
	Poly * p = (Poly*)spr->vptr;
	
	if(anchor)
		p->SetVertRect(x-spr->XPos,y-spr->YPos,spr->Width, spr->Height, 0.0f);
	else
		p->SetVertRect(x-spr->XPos-Viewport.x, y-spr->YPos-Viewport.y, spr->Width, spr->Height, 0.0f);

	lpD3DDevice->SetTexture( 0, p->m_Texture->pTexture );
	lpD3DDevice->SetStreamSource( 0, p->m_VertexBuffer, 0, sizeof(CUSTOMVERTEX) );
	lpD3DDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );
	p->m_VertexBuffer->Unlock();
}

void DirectXVideoDriver::SetCursor(Sprite2D * spr, int x, int y)
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
void DirectXVideoDriver::SetPalette(Sprite2D * spr, Color * pal)
{
	Poly * p = (Poly*)spr->vptr;
	p->m_Texture->Init(spr->pixels, spr->Width, spr->Height, 8, (void*)pal, p->m_Texture->hasCK, p->m_Texture->colorKey);
}

void DirectXVideoDriver::ConvertToVideoFormat(Sprite2D * sprite)
{
	return;
}

#define MINCOL 2
#define MUL    2

void DirectXVideoDriver::CalculateAlpha(Sprite2D * sprite)
{
	
}

/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
void DirectXVideoDriver::DrawRect(Region &rgn, Color &color)
{
	D3DRECT rect[1];
	rect[0].x1 = rgn.x;
	rect[0].y1 = rgn.y;
	rect[0].x2 = rgn.w;
	rect[0].y2 = rgn.h;
	lpD3DDevice->Clear( 1, rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(color.r, color.b, color.g, color.a), 0.0f, 0L);
}
/** Creates a Palette from Color */
Color * DirectXVideoDriver::CreatePalette(Color color, Color back)
{
	Color * pal = (Color*)malloc(256*sizeof(Color));
	pal[0].r = 0;
	pal[0].g = 0xff;
	pal[0].b = 0;
	pal[0].a = 0;
	for(int i = 1; i < 256; i++) {
		pal[i].r = back.r+(unsigned char)(((color.r-back.r)*(i))/255.0);
		pal[i].g = back.g+(unsigned char)(((color.g-back.g)*(i))/255.0);
		pal[i].b = back.b+(unsigned char)(((color.b-back.b)*(i))/255.0);
		pal[i].a = 0;
	}
	return pal;
}
/** Blits a Sprite filling the Region */
void DirectXVideoDriver::BlitTiled(Region rgn, Sprite2D * img, bool anchor)
{
	
}
/** Send a Quit Signal to the Event Queue */
bool DirectXVideoDriver::Quit()
{
	return true;
}
/** Get the Palette of a Sprite */
Color * DirectXVideoDriver::GetPalette(Sprite2D * spr)
{
	return (Color*)(((Poly*)spr->vptr)->m_Texture->palette);
}

