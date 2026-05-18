/**
 * @file lldxhardware.cpp
 * @brief LLDXHardware implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; version 2.1 of the License only.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA"
 * $/LicenseInfo$
 */

#ifdef LL_WINDOWS

// Culled from some Microsoft sample code

#include "linden_common.h"

#include <combaseapi.h>				// For IID_PPV_ARGS()
#define INITGUID
#include <dxdiag.h>
#undef INITGUID
#include <dxgi.h>
#include <stdlib.h>					// For getenv()
#include <wbemidl.h>
#include <d3d11.h>

#include "lldxhardware.h"

void (*gWriteDebug)(const char* msg) = NULL;
LLDXHardware gDXHardware;

#define SAFE_RELEASE(p)	{ if (p) { (p)->Release(); (p)=NULL; } }

static void get_wstring(IDxDiagContainer* containerp, const WCHAR* prop_name,
						WCHAR* prop_value, int output_size)
{
	VARIANT var;
	VariantInit(&var);
	HRESULT hr = containerp->GetProp(prop_name, &var);
	if (SUCCEEDED(hr))
	{
		// Switch off the type.  There's 4 different types:
		switch (var.vt)
		{
			case VT_UI4:
				swprintf(prop_value, L"%d", var.ulVal);
				break;
			case VT_I4:
				swprintf(prop_value, L"%d", var.lVal);
				break;
			case VT_BOOL:
				wcscpy(prop_value, var.boolVal ? L"true" : L"false");
				break;
			case VT_BSTR:
				wcsncpy(prop_value, var.bstrVal, output_size - 1);
				prop_value[output_size - 1] = 0;
				break;
		}
	}
	// Clear the variant (this is needed to free BSTR memory)
	VariantClear(&var);
}

static std::string get_string(IDxDiagContainer* containerp,
							  const WCHAR* prop_name)
{
	WCHAR prop_value[256];
	get_wstring(containerp, prop_name, prop_value, 256);
	return ll_convert_wide_to_string(prop_value);
}

//static
S32 LLDXHardware::getMBVideoMemoryViaDXGI()
{
	// Let the user override the detection in case it fails on their system.
	// They can specify the amount of VRAM in megabytes, via the LL_VRAM_MB
	// environment variable. HB
	char* vram_override = getenv("LL_VRAM_MB");
	if (vram_override)
	{
		S32 vram = atoi(vram_override);
		if (vram > 0)
		{
			llinfos << "Amount of VRAM overridden via the LL_VRAM_MB environment variable; detection step skipped. VRAM amount: "
					<< vram << "MB" << llendl;
			return vram;
		}
	}

	SIZE_T vram = 0;
	if (SUCCEEDED(CoInitialize(0)))
	{
		IDXGIFactory1* factoryp = NULL;
		HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factoryp));
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter1* adapterp = NULL;
			IDXGIAdapter1* tmp_adapterp = NULL;
			DXGI_ADAPTER_DESC1 desc;
			UINT idx = 0;
			while (factoryp->EnumAdapters1(idx++, &tmp_adapterp) !=
					DXGI_ERROR_NOT_FOUND)
			{
				if (!tmp_adapterp)	// Should not happen.
				{
					break;
				}
				hr = tmp_adapterp->GetDesc1(&desc);
				if (SUCCEEDED(hr) && desc.Flags == 0)
				{
					tmp_adapterp->QueryInterface(IID_PPV_ARGS(&adapterp));
					if (adapterp)
					{
						adapterp->GetDesc1(&desc);
						if (desc.DedicatedVideoMemory > vram)
						{
							vram = desc.DedicatedVideoMemory;
						}
						SAFE_RELEASE(adapterp);
					}
				}
				SAFE_RELEASE(tmp_adapterp);
			}
			SAFE_RELEASE(factoryp);
		}
		CoUninitialize();
	}
	return vram / (1024 * 1024);
}

LLSD LLDXHardware::getDisplayInfo()
{
	if (mInfo.size())
	{
		return mInfo;
	}

	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		llwarns << "COM library initialization failed !" << llendl;
		gWriteDebug("COM library initialization failed !\n");
		return mInfo;
	}

	IDxDiagProvider* dx_diag_providerp = NULL;
	IDxDiagContainer* dx_diag_rootp = NULL;
	IDxDiagContainer* devices_containerp = NULL;
	IDxDiagContainer* device_containerp = NULL;
	IDxDiagContainer* file_containerp = NULL;
	IDxDiagContainer* driver_containerp = NULL;

	// CoCreate a IDxDiagProvider*
	llinfos << "CoCreateInstance IID_IDxDiagProvider" << llendl;
	hr = CoCreateInstance(CLSID_DxDiagProvider, NULL, CLSCTX_INPROC_SERVER,
						  IID_IDxDiagProvider, (LPVOID*)&dx_diag_providerp);
	if (FAILED(hr))
	{
		llwarns << "No DXDiag provider found !  DirectX not installed !"
				<< llendl;
		gWriteDebug("No DXDiag provider found !  DirectX not installed !\n");
		goto exit_cleanup;
	}
	if (SUCCEEDED(hr)) // if FAILED(hr) then dx9 is not installed
	{
		// Fill out a DXDIAG_INIT_PARAMS struct and pass it to
		// IDxDiagContainer::Initialize(). Passing in TRUE for bAllowWHQLChecks
		// allows dxdiag to check if drivers are digital signed as logo'd by
		// WHQL which may connect via internet to update WHQL certificates.
		DXDIAG_INIT_PARAMS dx_diag_init_params;
		ZeroMemory(&dx_diag_init_params, sizeof(DXDIAG_INIT_PARAMS));

		dx_diag_init_params.dwSize = sizeof(DXDIAG_INIT_PARAMS);
		dx_diag_init_params.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
		dx_diag_init_params.bAllowWHQLChecks = TRUE;
		dx_diag_init_params.pReserved = NULL;

		LL_DEBUGS("AppInit") << "dx_diag_providerp->Initialize" << LL_ENDL;
		hr = dx_diag_providerp->Initialize(&dx_diag_init_params);
		if (FAILED(hr))
		{
			goto exit_cleanup;
		}

		LL_DEBUGS("AppInit") << "dx_diag_providerp->GetRootContainer"
							 << LL_ENDL;
		hr = dx_diag_providerp->GetRootContainer(&dx_diag_rootp);
		if (FAILED(hr) || !dx_diag_rootp)
		{
			goto exit_cleanup;
		}

		HRESULT hr;

		// Get display driver information
		LL_DEBUGS("AppInit") << "dx_diag_rootp->GetChildContainer" << LL_ENDL;
		hr = dx_diag_rootp->GetChildContainer(L"DxDiag_DisplayDevices",
												&devices_containerp);
		if (FAILED(hr) || !devices_containerp)
		{
			// Do not release 'dirty' devices_containerp at this stage, only
			// dx_diag_rootp
			devices_containerp = NULL;
			goto exit_cleanup;
		}

		DWORD dw_device_count;
		// Make sure there is something inside
		hr = devices_containerp->GetNumberOfChildContainers(&dw_device_count);
		if (FAILED(hr) || dw_device_count == 0)
		{
			goto exit_cleanup;
		}

		// Get device 0
		LL_DEBUGS("AppInit") << "devices_containerp->GetChildContainer"
							 << LL_ENDL;
		hr = devices_containerp->GetChildContainer(L"0", &device_containerp);
		if (FAILED(hr) || !device_containerp)
		{
			goto exit_cleanup;
		}

		// Get the English VRAM string
		std::string ram_str = get_string(device_containerp,
										 L"szDisplayMemoryEnglish");

		// Dump the string as an int into the structure
		char* stopstring;
		mInfo["VRAM"] = S32(strtol(ram_str.c_str(),
								   &stopstring, 10) / (1024 * 1024));
		std::string device_name = get_string(device_containerp,
											 L"szDescription");
		mInfo["DeviceName"] = device_name;
		std::string device_driver=  get_string(device_containerp,
											   L"szDriverVersion");
		mInfo["DriverVersion"] = device_driver;

		// ATI has a slightly different version string
		if (device_name.length() >= 4 && device_name.substr(0, 4) == "ATI ")
		{
			// Get the key
			HKEY hKey;
			const DWORD RV_SIZE = 100;
			WCHAR release_version[RV_SIZE];

			// Hard coded registry entry. Using this since it is simpler for
			// now. And using EnumDisplayDevices to get a registry key also
			// requires a hard coded Query value.
			if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,
											TEXT("SOFTWARE\\ATI Technologies\\CBT"),
											&hKey))
			{
				// Get the value
				DWORD dwType = REG_SZ;
				DWORD dwSize = sizeof(WCHAR) * RV_SIZE;
				if (ERROR_SUCCESS == RegQueryValueEx(hKey,
													 TEXT("ReleaseVersion"),
				 									 NULL, &dwType,
													 (LPBYTE)release_version,
													 &dwSize))
				{
					// Print the value; Windows does not guarantee to be nul
					// terminated
					release_version[RV_SIZE - 1] = 0;
					mInfo["DriverVersion"] =
						ll_convert_wide_to_string(release_version);

				}
				RegCloseKey(hKey);
			}
		}
	}

exit_cleanup:
	if (!mInfo.size())
	{
		llinfos << "Failed to get data, cleaning up..." << llendl;
	}
	SAFE_RELEASE(file_containerp);
	SAFE_RELEASE(driver_containerp);
	SAFE_RELEASE(device_containerp);
	SAFE_RELEASE(devices_containerp);
	SAFE_RELEASE(dx_diag_rootp);
	SAFE_RELEASE(dx_diag_providerp);

	CoUninitialize();
	return mInfo;
}

// Methods to request and hold a high-performance GPU on Windows 10+. Laptops
// can dynamically switch between integrated and discrete GPUs. The viewer has
// GPU-specific optimizations, and this switching can cause problems and
// crashes. The login screen requires low performance, which can lead to the OS
// deciding to switch to the integrated GPU; to avoid this, we request and hold
// a high-performance GPU using A D3D11 context until login. For diagnostics,
// we also log GPU changes.

ID3D11Device* gD3D11Devicep = NULL;
ID3D11DeviceContext* gD3D11Contextp = NULL;
LUID gExpectedAdapterLUID;
HMODULE gD3D11Libraryp;

//static
void LLDXHardware::requestHighPerformanceGPU()
{
	// Try to load d3d11.dll and request high performance adapter
	gD3D11Libraryp = LoadLibraryA("d3d11.dll");
	if (!gD3D11Libraryp)
	{
		llinfos << "D3D11 is not available on this machine, cannot request dGPU."
				<< llendl;
		return;
	}

	typedef HRESULT(WINAPI* PFN_D3D11_CREATE_DEVICE)(IDXGIAdapter*,
													 D3D_DRIVER_TYPE,
													 HMODULE, UINT,
													 const D3D_FEATURE_LEVEL*,
													 UINT, UINT,
													 ID3D11Device**,
													 D3D_FEATURE_LEVEL*,
													 ID3D11DeviceContext**);

	PFN_D3D11_CREATE_DEVICE D3D11CreateDevicep =
		(PFN_D3D11_CREATE_DEVICE)GetProcAddress(gD3D11Libraryp,
												"D3D11CreateDevice");
	if (!D3D11CreateDevicep)
	{
		llwarns << "Failed to get D3D11CreateDevice() function from d3d11.dll. dGPU request failed."
				<< llendl;
		FreeLibrary(gD3D11Libraryp);
		gD3D11Libraryp = NULL;
		return;
	}

	// Try to enumerate adapters and select the best one
	IDXGIFactory1* factoryp = NULL;
	IDXGIAdapter1* sel_adapterp = NULL;
	std::string selected_descr;
	HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1),
									(void**)&factoryp);
	if (SUCCEEDED(hr) && factoryp)
	{
		IDXGIAdapter1* adapterp = NULL;
		SIZE_T max_vram = 0;
		UINT adapter_idx = 0;
		S32 adapter_count = 0;
		std::string description;
		// Enumerate all adapters and find the one with the most dedicated
		// video memory
		while (factoryp->EnumAdapters1(adapter_idx,
									   &adapterp) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapterp->GetDesc1(&desc);
			description = ll_convert_wide_to_string(desc.Description);

			// Skip software adapters
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				LL_DEBUGS("AppInit") << "Adapter " << adapter_idx << ": "
									 << description << ", Dedicated VRAM: "
									 << (desc.DedicatedVideoMemory >> 20)
									 << "MB, Vendor: 0x" << std::hex
									 << desc.VendorId << std::dec
									 << ", Flags: " << desc.Flags << LL_ENDL;
			}
			else
			{
				llinfos << "Adapter " << adapter_idx << ": " << description
						<< ", Dedicated VRAM: "
						<< (desc.DedicatedVideoMemory >> 20)
						<< "MB, Vendor: 0x" << std::hex << desc.VendorId
						<< std::dec << ", Flags: " << desc.Flags << llendl;

				++adapter_count;
				// Select adapter with most dedicated video memory (typically
				// the discrete GPU)
				if (desc.DedicatedVideoMemory > max_vram)
				{
					if (sel_adapterp)
					{
						sel_adapterp->Release();
					}
					sel_adapterp = adapterp;
					sel_adapterp->AddRef();
					max_vram = desc.DedicatedVideoMemory;
					gExpectedAdapterLUID = desc.AdapterLuid;
					selected_descr = description;
				}
			}

			adapterp->Release();
			++adapter_idx;
		}
		factoryp->Release();

		if (adapter_count < 2)
		{
			// Only one adapter, no need to request high-performance GPU
			if (sel_adapterp)
			{
				sel_adapterp->Release();
			}
			gExpectedAdapterLUID = { 0, 0 };
			FreeLibrary(gD3D11Libraryp);
			gD3D11Libraryp = NULL;
			return;
		}

		llinfos << "Selected as preferred adapter (highest VRAM): "
				<< selected_descr << llendl;
	}

	// Create a temporary device to ensure high-performance GPU is selected
	// This initialization can help "wake up" the discrete GPU
	D3D_FEATURE_LEVEL feature_lvl;
	D3D_FEATURE_LEVEL req_levels[] = { D3D_FEATURE_LEVEL_11_0,
									   D3D_FEATURE_LEVEL_11_1 };

	bool has_selection = sel_adapterp != NULL;
	if (has_selection)
	{
		hr = D3D11CreateDevicep(sel_adapterp, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
								req_levels, _countof(req_levels),
								D3D11_SDK_VERSION, &gD3D11Devicep,
								&feature_lvl, &gD3D11Contextp);
		sel_adapterp->Release();

		if (!SUCCEEDED(hr))
		{
			llwarns << "D3D11 failed to use preffered adapter "
					<< selected_descr << llendl;
			gExpectedAdapterLUID = { 0, 0 };
			has_selection = false;
		}
	}

	if (!has_selection)
	{
		// Either failed to select or did not find an adapter.
		hr = D3D11CreateDevicep(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
								req_levels, _countof(req_levels),
								D3D11_SDK_VERSION, &gD3D11Devicep,
								&feature_lvl, &gD3D11Contextp);
		if (!SUCCEEDED(hr))
		{
			llwarns << "D3D11 failed to use hardware adapter" << llendl;
			FreeLibrary(gD3D11Libraryp);
			gD3D11Libraryp = NULL;
			// These should not be set, but make sure they are null.
			gD3D11Devicep = NULL;
			gD3D11Contextp = NULL;
		}
	}
}

//static
void LLDXHardware::detectGPUChange()
{
	if (!gD3D11Devicep)
	{
		// Cannot detect without D3D11 device
		return;
	}

	if (gExpectedAdapterLUID.LowPart == 0 &&
		gExpectedAdapterLUID.HighPart == 0)
	{
		// No specific adapter was selected, can't detect changes.
		return;
	}

	IDXGIDevice* dxgi_devp = NULL;
	HRESULT hr = gD3D11Devicep->QueryInterface(__uuidof(IDXGIDevice),
											   (void**)&dxgi_devp);
	if (SUCCEEDED(hr) && dxgi_devp)
	{
		IDXGIAdapter* adapterp = NULL;
		hr = dxgi_devp->GetAdapter(&adapterp);
		if (SUCCEEDED(hr) && adapterp)
		{
			DXGI_ADAPTER_DESC desc;
			adapterp->GetDesc(&desc);

			bool changed = false;
			// Check if LUID has changed
			if (desc.AdapterLuid.LowPart != gExpectedAdapterLUID.LowPart ||
				desc.AdapterLuid.HighPart != gExpectedAdapterLUID.HighPart)
			{
				changed = true;
				llwarns << "GPU change detected !  Current adapter: "
						<< ll_convert_wide_to_string(desc.Description) << llendl;
			}

			adapterp->Release();
			dxgi_devp->Release();
			return;
		}

		if (dxgi_devp)
		{
			dxgi_devp->Release();
		}
	}
}

//static
void LLDXHardware::clearHighPerformanceGPURequest()
{
	gExpectedAdapterLUID = { 0, 0 };
	if (gD3D11Contextp)
	{
		gD3D11Contextp->Release();
		gD3D11Contextp = NULL;
	}
	if (gD3D11Devicep)
	{
		gD3D11Devicep->Release();
		gD3D11Devicep = NULL;
	}
	if (gD3D11Libraryp)
	{
		FreeLibrary(gD3D11Libraryp);
		gD3D11Libraryp = NULL;
	}
}

#endif
