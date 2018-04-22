/**
* Copyright (C) 2018 Elisha Riedlinger
*
* This software is  provided 'as-is', without any express  or implied  warranty. In no event will the
* authors be held liable for any damages arising from the use of this software.
* Permission  is granted  to anyone  to use  this software  for  any  purpose,  including  commercial
* applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*   1. The origin of this software must not be misrepresented; you must not claim that you  wrote the
*      original  software. If you use this  software  in a product, an  acknowledgment in the product
*      documentation would be appreciated but is not required.
*   2. Altered source versions must  be plainly  marked as such, and  must not be  misrepresented  as
*      being the original software.
*   3. This notice may not be removed or altered from any source distribution.
*/

#include "ddraw.h"

HRESULT m_IDirect3DTextureX::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	return ProxyQueryInterface(ProxyInterface, riid, ppvObj, IID_IDirect3DTexture, this);
}

ULONG m_IDirect3DTextureX::AddRef()
{
	return ProxyInterface->AddRef();
}

ULONG m_IDirect3DTextureX::Release()
{
	ULONG x = ProxyInterface->Release();

	if (x == 0)
	{
		WrapperInterface->DeleteMe();
	}

	return x;
}

HRESULT m_IDirect3DTextureX::Initialize(LPDIRECT3DDEVICE a, LPDIRECTDRAWSURFACE b)
{
	if (ProxyDirectXVersion != 1)
	{
		Logging::Log() << __FUNCTION__ << " Not Implimented";
		return E_NOTIMPL;
	}

	if (a)
	{
		a = static_cast<m_IDirect3DDevice *>(a)->GetProxyInterface();
	}
	if (b)
	{
		b = static_cast<m_IDirectDrawSurface *>(b)->GetProxyInterface();
	}

	return ((IDirect3DTexture*)ProxyInterface)->Initialize(a, b);
}

HRESULT m_IDirect3DTextureX::GetHandle(LPDIRECT3DDEVICE2 a, LPD3DTEXTUREHANDLE b)
{
	if (a)
	{
		a = static_cast<m_IDirect3DDevice2 *>(a)->GetProxyInterface();
	}

	return ProxyInterface->GetHandle(a, b);
}

HRESULT m_IDirect3DTextureX::PaletteChanged(DWORD a, DWORD b)
{
	return ProxyInterface->PaletteChanged(a, b);
}

HRESULT m_IDirect3DTextureX::Load(LPDIRECT3DTEXTURE2 a)
{
	if (a)
	{
		a = static_cast<m_IDirect3DTexture2 *>(a)->GetProxyInterface();
	}

	return ProxyInterface->Load(a);
}

HRESULT m_IDirect3DTextureX::Unload()
{
	if (ProxyDirectXVersion != 1)
	{
		Logging::Log() << __FUNCTION__ << " Not Implimented";
		return E_NOTIMPL;
	}

	return ((IDirect3DTexture*)ProxyInterface)->Unload();
}