
//================================================================================================
// Originally Based on debug_draw DX11 sample code by Guilherme R. Lampert (Public Domain)
// The framework has been extended and modified by D.Moore
//================================================================================================

//========================================================
// Globals
//========================================================
#define DEBUG_DRAW_IMPLEMENTATION
#include "Framework.h"
#include "ShaderSet.h"

#include <cstdlib>
#include <tuple>
#include <chrono>
#include <fstream>

// ========================================================
// IMGUI
// ========================================================
#include "imgui/imgui_impl_dx11.h"
// This is Imgui IO handler which is defined elsewhere.
extern IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ========================================================
// LIBRARY linking
// ========================================================

#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "d3dcompiler")

// ========================================================
// Globals
// ========================================================

Keys keys;
Mouse mouse;
Time deltaTime;
Camera camera;

//================================================================================
// Time releated functions
//================================================================================

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
static TimePoint g_startupTime = std::chrono::high_resolution_clock::now();

std::int64_t getTimeMicroseconds()
{
	const auto currentTime = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(currentTime - g_startupTime).count();
}

double getTimeSeconds()
{
	return getTimeMicroseconds() * 0.000001;
}

v2 getMousePosition()
{
	return v2(mouse.lastPosX, mouse.lastPosY);
}

//================================================================================
// Debug print functions.
//================================================================================

void errorF(const char * format, ...)
{
	va_list args;
	va_start(args, format);
	std::vfprintf(stderr, format, args);
	va_end(args);

	// Default newline and flush (like std::endl)
	std::fputc('\n', stderr);
	std::fflush(stderr);
}

void panicF(const char * format, ...)
{
	va_list args;
	char buffer[2048] = { '\0' };

	va_start(args, format);
	std::vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	MessageBoxA(nullptr, buffer, "Fatal Error", MB_OK);
	std::abort();
}

void debugF(const char * format, ...)
{
	va_list args;
	char buffer[2048] = { '\0' };

	va_start(args, format);
	std::vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	OutputDebugString(buffer);
}

// ========================================================
// Window
// ========================================================

const char WindowClassName[] = "FrameworkD3D11";

class Window
{
public:

	HINSTANCE m_hInstance = nullptr;
	HWND      m_hWnd = nullptr;

	Window(HINSTANCE hInstance, int nCmdShow, const char* pTitleString)
		: m_hInstance(hInstance)
	{
		registerClass();
		createWindow(nCmdShow, pTitleString);
	}

	virtual ~Window()
	{
		if (m_hWnd != nullptr)
		{
			DestroyWindow(m_hWnd);
		}
		if (m_hInstance != nullptr)
		{
			UnregisterClass(WindowClassName, m_hInstance);
		}
	}

	void runMessageLoop()
	{
		MSG msg = { 0 };
		bool bKeepGoing = true;
		while (bKeepGoing)
		{
			while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);			
				
				if (WM_QUIT == msg.message)
				{
					bKeepGoing = false;
				}
			}
			onRender();
		}
	}

	virtual void onRender() = 0;


	virtual void onResize() = 0;

	static int s_width;
	static int s_height;

private:

	void registerClass()
	{
		WNDCLASSEX wcex = { 0 };
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = &Window::WndProc;
		wcex.hInstance = m_hInstance;
		wcex.lpszClassName = WindowClassName;
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		RegisterClassEx(&wcex);
	}

	void createWindow(int nCmdShow, const char* pTitleString)
	{
		// Calculate adjusted window size to get desired client resolution.
		RECT rect{ 0,0,s_width,s_height };
		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

		UINT width = rect.right - rect.left;
		UINT height = rect.bottom - rect.top;

		// Create the window.
		m_hWnd = CreateWindow(WindowClassName, pTitleString, WS_OVERLAPPEDWINDOW,
			0, 0, width, height,
			nullptr, nullptr, m_hInstance, this);
		if (m_hWnd == nullptr)
		{
			panicF("Failed to create application window!");
		}

		// Register for WM_INPUT
		RAWINPUTDEVICE Rid[1];
		Rid[0].usUsagePage = 0x01;
		Rid[0].usUsage = 0x02;
		Rid[0].dwFlags = NULL;
		Rid[0].hwndTarget = NULL; // NULL == Follow keyboard focus
		if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE)
		{
			panicF("Failed to register raw input mouse device.");
		}


		ShowWindow(m_hWnd, nCmdShow);
		UpdateWindow(m_hWnd);
	}


	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		//	static Window* s_pWindow = nullptr;

			// Let imgui have a peek at the inputs first.
		ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);

		switch (message)
		{
		case WM_NCCREATE:
		{
			// Forward 'this' pointer to the windows user data.
			LPCREATESTRUCT pCreateStruct = (LPCREATESTRUCT)lParam;
			Window* pWindow = (Window*)pCreateStruct->lpCreateParams;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pWindow);
		} break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_KEYDOWN:
			if (wParam == VK_RETURN) { keys.showGrid = !keys.showGrid; }
			if (wParam == VK_SPACE) { keys.showLabels = !keys.showLabels; }
			return 0;

		case WM_INPUT:
		{
			constexpr u32 kInputBufferSize = 64;
			UINT dwSize = kInputBufferSize;
			BYTE lpb[kInputBufferSize];

			// obtain size
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

			if (dwSize <= kInputBufferSize)
			{
				// get the actual data
				GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
				RAWINPUT* raw = (RAWINPUT*)lpb;

				if (raw->header.dwType == RIM_TYPEMOUSE // is mouse data
					&& raw->data.mouse.usFlags == 0 // and is relative mouse position
					)
				{
					// These must be accumulated you may receive more than one per frame.
					mouse.deltaX += raw->data.mouse.lLastX;
					mouse.deltaY += raw->data.mouse.lLastY;
				}
			}
			break;
		}

		case WM_MOUSEMOVE:
		{
			mouse.lastPosX = LOWORD(lParam);
			mouse.lastPosY = HIWORD(lParam);
		} break;

		case WM_SIZE:
		{
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);

			if (width != s_width || height != s_height)
			{
				if (width != 0 && height != 0)
				{
					/*
					Window* pWindow = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
					if (pWindow)
						pWindow->onResize();
					*/
				}
			}
		}
		break;

		default:
			break;
		} // switch

		return DefWindowProc(hWnd, message, wParam, lParam);
	}
};

// Caching these for convenience
int Window::s_width = 1024;
int Window::s_height = 768;

// ========================================================
// RenderWindowD3D11
// ========================================================

class RenderWindowD3D11 : public Window
{
public:

	ComPtr<IDXGISwapChain>         m_pSwapChain;
	ComPtr<ID3D11Device>           m_pD3DDevice;
	ComPtr<ID3D11DeviceContext>    m_pDeviceContext;
	ComPtr<ID3D11Texture2D>		   m_pDepthStencil;
	ComPtr<ID3D11DepthStencilView> m_pDepthStencilView;
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;
	ComPtr<ID3D11DepthStencilState> m_pDepthStencilState;

	std::function<void()>          m_pRenderCallback;
	std::function<void(u32, u32)>  m_pResizeCallback;

	RenderWindowD3D11(HINSTANCE hInstance, int nCmdShow, const char* pTitleString)
		: Window(hInstance, nCmdShow, pTitleString)
	{
		initD3D();
	}

	void onRender() override
	{
		const float clearColor[] = { 0, 0, 0, 0.f };
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView.Get(), clearColor);

		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		if (m_pRenderCallback)
		{
			m_pRenderCallback();
		}	
		//m_pSwapChain->Present(1, 0); // use VSYNC
		m_pSwapChain->Present(0, 0);	// disable VSYNC
	}

	void onResize() override
	{
		HRESULT hr;

		// Obtaini the new client rectangle.
		RECT clientRect;
		GetClientRect(m_hWnd, &clientRect);
		UINT width = clientRect.right - clientRect.left;
		UINT height = clientRect.bottom - clientRect.top;

		// Pass this to the rest of the app.
		s_width = width;
		s_height = height;

		// Release all outstanding references to the swap chain's buffers.
		m_pDeviceContext->OMSetRenderTargets(0, 0, 0);
		m_pRenderTargetView.Reset();

		// Release depth buffer
		m_pDepthStencilView.Reset();
		m_pDepthStencil.Reset();


		// Preserve the existing buffer count and format.
		// Automatically choose the width and height to match the client rect for HWNDs.
		hr = m_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
		if (FAILED(hr))
		{
			panicF("Failed to ResizeBuffers.");
		}

		// Now setup all the views and bind the target.
		SetupRenderTarget(width, height);

		m_pResizeCallback(width, height);
	}

private:

	void initD3D()
	{
		UINT createDeviceFlags = 0;
		const UINT width = Window::s_width;
		const UINT height = Window::s_height;

		// If the project is in a debug build, enable debugging via SDK Layers with this flag.
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // DEBUG

		// Acceptable driver types.
		const D3D_DRIVER_TYPE driverTypes[] = {
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_WARP,
			D3D_DRIVER_TYPE_REFERENCE,
		};
		const UINT numDriverTypes = ARRAYSIZE(driverTypes);

		// This array defines the ordering of feature levels that D3D should attempt to create.
		const D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
		};
		const UINT numFeatureLevels = ARRAYSIZE(featureLevels);

		DXGI_SWAP_CHAIN_DESC sd = { 0 };
		sd.BufferCount = 2;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = m_hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		HRESULT hr;
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

		// Try to create the device and swap chain:
		for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; ++driverTypeIndex)
		{
			auto driverType = driverTypes[driverTypeIndex];
			hr = D3D11CreateDeviceAndSwapChain(nullptr, driverType, nullptr, createDeviceFlags,
				featureLevels, numFeatureLevels, D3D11_SDK_VERSION,
				&sd, m_pSwapChain.GetAddressOf(), m_pD3DDevice.GetAddressOf(),
				&featureLevel, m_pDeviceContext.GetAddressOf());

			if (hr == E_INVALIDARG)
			{
				// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
				hr = D3D11CreateDeviceAndSwapChain(nullptr, driverType, nullptr, createDeviceFlags,
					&featureLevels[1], numFeatureLevels - 1,
					D3D11_SDK_VERSION, &sd, m_pSwapChain.GetAddressOf(),
					m_pD3DDevice.GetAddressOf(), &featureLevel,
					m_pDeviceContext.GetAddressOf());
			}

			if (SUCCEEDED(hr))
			{
				break;
			}
		}

		if (FAILED(hr))
		{
			panicF("Failed to create D3D device or swap chain!");
		}

		// Now setup all the views and bind the target.
		SetupRenderTarget(width, height);

	}

	void SetupRenderTarget(const UINT width, const UINT height)
	{
		HRESULT hr;

		// Create a render target view for the framebuffer:
		ID3D11Texture2D * backBuffer = nullptr;
		hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
		if (FAILED(hr))
		{
			panicF("Failed to get framebuffer from swap chain!");
		}

		hr = m_pD3DDevice->CreateRenderTargetView(backBuffer, NULL, m_pRenderTargetView.GetAddressOf());
		backBuffer->Release();
		if (FAILED(hr))
		{
			panicF("Failed to create Render Target View for framebuffer!");
		}

		// Create the Depth buffer.
		CreateDepthBuffer(width, height);

		ID3D11RenderTargetView* targets[] = { m_pRenderTargetView.Get() };
		m_pDeviceContext->OMSetRenderTargets(1, targets, m_pDepthStencilView.Get());


		// Setup the viewport:
		D3D11_VIEWPORT vp;
		vp.Width = static_cast<float>(width);
		vp.Height = static_cast<float>(height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		m_pDeviceContext->RSSetViewports(1, &vp);


		// setup the depth stencil state.
		D3D11_DEPTH_STENCIL_DESC dsDesc;

		// Depth test parameters
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

		// Stencil test parameters
		dsDesc.StencilEnable = false;
		dsDesc.StencilReadMask = 0xFF;
		dsDesc.StencilWriteMask = 0xFF;

		// Stencil operations if pixel is front-facing
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		// Stencil operations if pixel is back-facing
		dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		// Create depth stencil state
		m_pD3DDevice->CreateDepthStencilState(&dsDesc, m_pDepthStencilState.GetAddressOf());
		m_pDeviceContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	}

	void CreateDepthBuffer(const UINT width, const UINT height)
	{
		HRESULT hr;

		// Create a depth buffer
		D3D11_TEXTURE2D_DESC descDepth;
		descDepth.Width = width;
		descDepth.Height = height;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // or DXGI_FORMAT_D16_UNORM
		descDepth.SampleDesc.Count = 1;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Usage = D3D11_USAGE_DEFAULT;
		descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags = 0;
		hr = m_pD3DDevice->CreateTexture2D(&descDepth, NULL, m_pDepthStencil.GetAddressOf());
		if (FAILED(hr))
		{
			panicF("Failed to create Depth Buffer swap chain!");
		}

		hr = m_pD3DDevice->CreateDepthStencilView(m_pDepthStencil.Get(), NULL, m_pDepthStencilView.GetAddressOf());
		if (FAILED(hr))
		{
			panicF("Failed to create Depth Stencil View for framebuffer!");
		}
	}

};



// ========================================================
// RenderInterfaceD3D11
// ========================================================

class RenderInterfaceD3D11 final
	: public dd::RenderInterface
{
public:

	RenderInterfaceD3D11(const ComPtr<ID3D11Device> & pDevice, const ComPtr<ID3D11DeviceContext> & pContext)
		: d3dDevice(pDevice)
		, deviceContext(pContext)
	{
		initShaders();
		initBuffers();
	}

	void setMvpMatrixPtr(const m4x4& mtx)
	{
		constantBufferData.mvpMatrix = DirectX::XMMATRIX(mtx);
	}

	void setCameraFrame(const v3 & up, const v3 & right, const v3 & origin)
	{
		camUp = up; camRight = right; camOrigin = origin;
	}

	void beginDraw() override
	{
		// Update and set the constant buffer for this frame
		deviceContext->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &constantBufferData, 0, 0);
		deviceContext->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

		// Disable culling for the screen text
		deviceContext->RSSetState(rasterizerState.Get());
	}

	void endDraw() override
	{
		// No work done here at the moment.
	}

	dd::GlyphTextureHandle createGlyphTexture(int width, int height, const void * pixels) override
	{
		UINT numQualityLevels = 0;
		d3dDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8_UNORM, 1, &numQualityLevels);

		D3D11_TEXTURE2D_DESC tex2dDesc = {};
		tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
		tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		tex2dDesc.Format = DXGI_FORMAT_R8_UNORM;
		tex2dDesc.Width = width;
		tex2dDesc.Height = height;
		tex2dDesc.MipLevels = 1;
		tex2dDesc.ArraySize = 1;
		tex2dDesc.SampleDesc.Count = 1;
		tex2dDesc.SampleDesc.Quality = numQualityLevels - 1;

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = pixels;
		initData.SysMemPitch = width;

		auto * texImpl = new TextureImpl{};

		if (FAILED(d3dDevice->CreateTexture2D(&tex2dDesc, &initData, &texImpl->d3dTexPtr)))
		{
			errorF("CreateTexture2D failed!");
			destroyGlyphTexture(texImpl);
			return nullptr;
		}
		if (FAILED(d3dDevice->CreateShaderResourceView(texImpl->d3dTexPtr, nullptr, &texImpl->d3dTexSRV)))
		{
			errorF("CreateShaderResourceView failed!");
			destroyGlyphTexture(texImpl);
			return nullptr;
		}
		if (FAILED(d3dDevice->CreateSamplerState(&samplerDesc, &texImpl->d3dSampler)))
		{
			errorF("CreateSamplerState failed!");
			destroyGlyphTexture(texImpl);
			return nullptr;
		}

		return static_cast<dd::GlyphTextureHandle>(texImpl);
	}

	void destroyGlyphTexture(dd::GlyphTextureHandle glyphTex) override
	{
		auto * texImpl = static_cast<TextureImpl *>(glyphTex);
		if (texImpl)
		{
			if (texImpl->d3dSampler) { texImpl->d3dSampler->Release(); }
			if (texImpl->d3dTexSRV) { texImpl->d3dTexSRV->Release(); }
			if (texImpl->d3dTexPtr) { texImpl->d3dTexPtr->Release(); }
			delete texImpl;
		}
	}

	void drawGlyphList(const dd::DrawVertex * glyphs, int count, dd::GlyphTextureHandle glyphTex) override
	{
		ASSERT(glyphs != nullptr);
		ASSERT(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

		auto * texImpl = static_cast<TextureImpl *>(glyphTex);
		ASSERT(texImpl != nullptr);

		// Map the vertex buffer:
		D3D11_MAPPED_SUBRESOURCE mapInfo;
		if (FAILED(deviceContext->Map(glyphVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapInfo)))
		{
			panicF("Failed to map vertex buffer!");
		}

		// Copy into mapped buffer:
		auto * verts = static_cast<Vertex *>(mapInfo.pData);
		for (int v = 0; v < count; ++v)
		{
			verts[v].pos.x = glyphs[v].glyph.x;
			verts[v].pos.y = glyphs[v].glyph.y;
			verts[v].pos.z = 0.0f;
			verts[v].pos.w = 1.0f;

			verts[v].uv.x = glyphs[v].glyph.u;
			verts[v].uv.y = glyphs[v].glyph.v;
			verts[v].uv.z = 0.0f;
			verts[v].uv.w = 0.0f;

			verts[v].color.x = glyphs[v].glyph.r;
			verts[v].color.y = glyphs[v].glyph.g;
			verts[v].color.z = glyphs[v].glyph.b;
			verts[v].color.w = 1.0f;
		}

		// Unmap and draw:
		deviceContext->Unmap(glyphVertexBuffer.Get(), 0);

		// Bind texture & sampler (t0, s0):
		deviceContext->PSSetShaderResources(0, 1, &texImpl->d3dTexSRV);
		deviceContext->PSSetSamplers(0, 1, &texImpl->d3dSampler);

		const float blendFactor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		deviceContext->OMSetBlendState(blendStateText.Get(), blendFactor, 0xFFFFFFFF);

		// Draw with the current buffer:
		drawHelper(count, glyphShaders, glyphVertexBuffer.Get(), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Restore default blend state.
		deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	}

	void drawPointList(const dd::DrawVertex * points, int count, bool depthEnabled) override
	{
		(void)depthEnabled; // TODO: not implemented yet - not required by this sample

		// Emulating points as billboarded quads, so each point will use 6 vertexes.
		// D3D11 doesn't support "point sprites" like OpenGL (gl_PointSize).
		const int maxVerts = DEBUG_DRAW_VERTEX_BUFFER_SIZE / 6;

		// OpenGL point size scaling produces gigantic points with the billboarding fallback.
		// This is some arbitrary down-scaling factor to more or less match the OpenGL samples.
		const float D3DPointSpriteScalingFactor = 0.01f;

		ASSERT(points != nullptr);
		ASSERT(count > 0 && count <= maxVerts);

		// Map the vertex buffer:
		D3D11_MAPPED_SUBRESOURCE mapInfo;
		if (FAILED(deviceContext->Map(pointVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapInfo)))
		{
			panicF("Failed to map vertex buffer!");
		}

		const int numVerts = count * 6;
		const int indexes[6] = { 0, 1, 2, 2, 3, 0 };

		int v = 0;
		auto * verts = static_cast<Vertex *>(mapInfo.pData);

		// Expand each point into a quad:
		for (int p = 0; p < count; ++p)
		{
			const float ptSize = points[p].point.size * D3DPointSpriteScalingFactor;
			const v3 halfWidth = (ptSize * 0.5f) * camRight; // X
			const v3 halfHeigh = (ptSize * 0.5f) * camUp;    // Y
			const v3 origin = v3{ points[p].point.x, points[p].point.y, points[p].point.z };

			v3 corners[4];
			corners[0] = origin + halfWidth + halfHeigh;
			corners[1] = origin - halfWidth + halfHeigh;
			corners[2] = origin - halfWidth - halfHeigh;
			corners[3] = origin + halfWidth - halfHeigh;

			for (int i : indexes)
			{
				verts[v].pos.x = corners[i].x;
				verts[v].pos.y = corners[i].y;
				verts[v].pos.z = corners[i].z;
				verts[v].pos.w = 1.0f;

				verts[v].color.x = points[p].point.r;
				verts[v].color.y = points[p].point.g;
				verts[v].color.z = points[p].point.b;
				verts[v].color.w = 1.0f;

				++v;
			}
		}
		ASSERT(v == numVerts);

		// Unmap and draw:
		deviceContext->Unmap(pointVertexBuffer.Get(), 0);

		// Draw with the current buffer:
		drawHelper(numVerts, pointShaders, pointVertexBuffer.Get(), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void drawLineList(const dd::DrawVertex * lines, int count, bool depthEnabled) override
	{
		(void)depthEnabled; // TODO: not implemented yet - not required by this sample

		ASSERT(lines != nullptr);
		ASSERT(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

		// Map the vertex buffer:
		D3D11_MAPPED_SUBRESOURCE mapInfo;
		if (FAILED(deviceContext->Map(lineVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapInfo)))
		{
			panicF("Failed to map vertex buffer!");
		}

		// Copy into mapped buffer:
		auto * verts = static_cast<Vertex *>(mapInfo.pData);
		for (int v = 0; v < count; ++v)
		{
			verts[v].pos.x = lines[v].line.x;
			verts[v].pos.y = lines[v].line.y;
			verts[v].pos.z = lines[v].line.z;
			verts[v].pos.w = 1.0f;

			verts[v].color.x = lines[v].line.r;
			verts[v].color.y = lines[v].line.g;
			verts[v].color.z = lines[v].line.b;
			verts[v].color.w = 1.0f;
		}

		// Unmap and draw:
		deviceContext->Unmap(lineVertexBuffer.Get(), 0);

		// Draw with the current buffer:
		drawHelper(count, lineShaders, lineVertexBuffer.Get(), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	}

	void onResize(u32 width, u32 height)
	{
		constantBufferData.screenDimensions.x = float(width);
		constantBufferData.screenDimensions.y = float(height);
	}

private:

	//
	// Local types:
	//

	struct ConstantBufferData
	{
		DirectX::XMMATRIX mvpMatrix = DirectX::XMMatrixIdentity();
		DirectX::XMFLOAT4 screenDimensions = { float(Window::s_width), float(Window::s_height), 0.0f, 0.0f };
	};

	struct Vertex
	{
		DirectX::XMFLOAT4A pos;   // 3D position
		DirectX::XMFLOAT4A uv;    // Texture coordinates
		DirectX::XMFLOAT4A color; // RGBA float
	};

	struct TextureImpl : public dd::OpaqueTextureType
	{
		ID3D11Texture2D          * d3dTexPtr = nullptr;
		ID3D11ShaderResourceView * d3dTexSRV = nullptr;
		ID3D11SamplerState       * d3dSampler = nullptr;
	};

	//
	// Members:
	//

	ComPtr<ID3D11Device>          d3dDevice;
	ComPtr<ID3D11DeviceContext>   deviceContext;
	ComPtr<ID3D11RasterizerState> rasterizerState;
	ComPtr<ID3D11BlendState>      blendStateText;

	ComPtr<ID3D11Buffer>          constantBuffer;
	ConstantBufferData            constantBufferData;

	ComPtr<ID3D11Buffer>          lineVertexBuffer;
	ComPtr<ID3D11Buffer>          pointVertexBuffer;
	ComPtr<ID3D11Buffer>          glyphVertexBuffer;

	ShaderSet                lineShaders;
	ShaderSet                pointShaders;
	ShaderSet                glyphShaders;

	// Camera vectors for the emulated point sprites
	v3                       camUp = v3{ 0.0f };
	v3                       camRight = v3{ 0.0f };
	v3                       camOrigin = v3{ 0.0f };

	void initShaders()
	{
		// Same vertex format used by all buffers to simplify things.
		const D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		const ShaderSet::InputLayoutDesc inputDesc{ layout, int(ARRAYSIZE(layout)) };

		constexpr char* s_frameworkShaders("Assets/Shaders/FrameworkShaders.fx");

		// 3D lines shader:
		lineShaders.init(d3dDevice.Get(), ShaderSetDesc::Create_VS_PS(s_frameworkShaders, "VS_LinePoint", "PS_LinePoint"), inputDesc, true);

		// 3D points shader:
		pointShaders.init(d3dDevice.Get(), ShaderSetDesc::Create_VS_PS(s_frameworkShaders, "VS_LinePoint", "PS_LinePoint"), inputDesc, true);

		// 2D glyphs shader:
		glyphShaders.init(d3dDevice.Get(), ShaderSetDesc::Create_VS_PS(s_frameworkShaders, "VS_TextGlyph", "PS_TextGlyph"), inputDesc, true);

		// Rasterizer state for the screen text:
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_NONE;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 0;
		rsDesc.DepthBiasClamp = 0.0f;
		rsDesc.SlopeScaledDepthBias = 0.0f;
		rsDesc.DepthClipEnable = false;
		rsDesc.ScissorEnable = false;
		rsDesc.MultisampleEnable = false;
		rsDesc.AntialiasedLineEnable = false;
		if (FAILED(d3dDevice->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf())))
		{
			errorF("CreateRasterizerState failed!");
		}

		// Blend state for the screen text:
		D3D11_BLEND_DESC bsDesc = {};
		bsDesc.RenderTarget[0].BlendEnable = true;
		bsDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		bsDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		bsDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		bsDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bsDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		bsDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		bsDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		if (FAILED(d3dDevice->CreateBlendState(&bsDesc, blendStateText.GetAddressOf())))
		{
			errorF("CreateBlendState failed!");
		}
	}

	void initBuffers()
	{
		D3D11_BUFFER_DESC bd;

		// Create the shared constant buffer:
		bd = {};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(ConstantBufferData);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		if (FAILED(d3dDevice->CreateBuffer(&bd, nullptr, constantBuffer.GetAddressOf())))
		{
			panicF("Failed to create shader constant buffer!");
		}

		// Create the vertex buffers for lines/points/glyphs:
		bd = {};
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(Vertex) * DEBUG_DRAW_VERTEX_BUFFER_SIZE;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		if (FAILED(d3dDevice->CreateBuffer(&bd, nullptr, lineVertexBuffer.GetAddressOf())))
		{
			panicF("Failed to create lines vertex buffer!");
		}
		if (FAILED(d3dDevice->CreateBuffer(&bd, nullptr, pointVertexBuffer.GetAddressOf())))
		{
			panicF("Failed to create points vertex buffer!");
		}
		if (FAILED(d3dDevice->CreateBuffer(&bd, nullptr, glyphVertexBuffer.GetAddressOf())))
		{
			panicF("Failed to create glyphs vertex buffer!");
		}
	}

	void drawHelper(const int numVerts, const ShaderSet & ss,
		ID3D11Buffer * vb, const D3D11_PRIMITIVE_TOPOLOGY topology)
	{
		const UINT offset = 0;
		const UINT stride = sizeof(Vertex);
		deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
		deviceContext->IASetPrimitiveTopology(topology);
		deviceContext->IASetInputLayout(ss.inputLayout.Get());

		ss.bind(deviceContext.Get());

		deviceContext->Draw(numVerts, 0);
	}
};

// ========================================================
// Framework demo Features.
// ========================================================
namespace DemoFeatures
{
void editorHud(dd::ContextHandle ctx)
{
	// HUD text:
	const ddVec3 textColor = { 1.0f,  1.0f,  1.0f };
	const ddVec3 textPos2D = { 10.0f, 15.0f, 0.0f };

	constexpr u32 kTextBufferSize = 512;

	char buffer[kTextBufferSize];
	sprintf_s(buffer, kTextBufferSize,
		"[WASD] + Mouse to adjust Camera\n"
		"Eye Position: %.2f %.2f %.2f\n"
		, camera.eye.x, camera.eye.y, camera.eye.z
	);

	dd::screenText(ctx, buffer, textPos2D, textColor, 0.55f);
}

void drawGrid(dd::ContextHandle ctx)
{
	if (!keys.showGrid)
	{
		return;
	}

	// Grid from -50 to +50 in both X & Z
	dd::xzSquareGrid(ctx, -50.0f, 50.0f, -1.0f, 1.7f, dd::colors::Green);
}

void drawLabel(dd::ContextHandle ctx, ddVec3_In pos, const char * name)
{
	if (!keys.showLabels)
	{
		return;
	}

	// Only draw labels inside the camera frustum.
	if (camera.pointInFrustum(v3(pos[0], pos[1], pos[2])))
	{
		const ddVec3 textColor = { 0.8f, 0.8f, 1.0f };
		dd::projectedText(ctx, name, pos, textColor, (float*)&camera.vpMatrix, 0, 0, Window::s_width, Window::s_height, 0.5f);
	}
}

void drawMiscObjects(dd::ContextHandle ctx)
{
	// Start a row of objects at this position:
	ddVec3 origin = { -15.0f, 0.0f, 0.0f };

	// Box with a point at it's center:
	drawLabel(ctx, origin, "box");
	dd::box(ctx, origin, dd::colors::Blue, 1.5f, 1.5f, 1.5f);
	dd::point(ctx, origin, dd::colors::White, 15.0f);
	origin[0] += 3.0f;

	// Sphere with a point at its center
	drawLabel(ctx, origin, "sphere");
	dd::sphere(ctx, origin, dd::colors::Red, 1.0f);
	dd::point(ctx, origin, dd::colors::White, 15.0f);
	origin[0] += 4.0f;

	// Two cones, one open and one closed:
	const ddVec3 condeDir = { 0.0f, 2.5f, 0.0f };
	origin[1] -= 1.0f;

	drawLabel(ctx, origin, "cone (open)");
	dd::cone(ctx, origin, condeDir, dd::colors::Yellow, 1.0f, 2.0f);
	dd::point(ctx, origin, dd::colors::White, 15.0f);
	origin[0] += 4.0f;

	drawLabel(ctx, origin, "cone (closed)");
	dd::cone(ctx, origin, condeDir, dd::colors::Cyan, 0.0f, 1.0f);
	dd::point(ctx, origin, dd::colors::White, 15.0f);
	origin[0] += 4.0f;

	// Axis-aligned bounding box:
	const ddVec3 bbMins = { -1.0f, -0.9f, -1.0f };
	const ddVec3 bbMaxs = { 1.0f,  2.2f,  1.0f };
	const ddVec3 bbCenter = {
		(bbMins[0] + bbMaxs[0]) * 0.5f,
		(bbMins[1] + bbMaxs[1]) * 0.5f,
		(bbMins[2] + bbMaxs[2]) * 0.5f
	};
	drawLabel(ctx, origin, "AABB");
	dd::aabb(ctx, bbMins, bbMaxs, dd::colors::Orange);
	dd::point(ctx, bbCenter, dd::colors::White, 15.0f);

	// Move along the Z for another row:
	origin[0] = -15.0f;
	origin[2] += 5.0f;

	// A big arrow pointing up:
	const ddVec3 arrowFrom = { origin[0], origin[1], origin[2] };
	const ddVec3 arrowTo = { origin[0], origin[1] + 5.0f, origin[2] };
	drawLabel(ctx, arrowFrom, "arrow");
	dd::arrow(ctx, arrowFrom, arrowTo, dd::colors::Magenta, 1.0f);
	dd::point(ctx, arrowFrom, dd::colors::White, 15.0f);
	dd::point(ctx, arrowTo, dd::colors::White, 15.0f);
	origin[0] += 4.0f;

	// Plane with normal vector:
	const ddVec3 planeNormal = { 0.0f, 1.0f, 0.0f };
	drawLabel(ctx, origin, "plane");
	dd::plane(ctx, origin, planeNormal, dd::colors::Yellow, dd::colors::Blue, 1.5f, 1.0f);
	dd::point(ctx, origin, dd::colors::White, 15.0f);
	origin[0] += 4.0f;

	// Circle on the Y plane:
	drawLabel(ctx, origin, "circle");
	dd::circle(ctx, origin, planeNormal, dd::colors::Orange, 1.5f, 15.0f);
	dd::point(ctx, origin, dd::colors::White, 15.0f);
	origin[0] += 3.2f;

	// Tangent basis vectors:
	const ddVec3 normal = { 0.0f, 1.0f, 0.0f };
	const ddVec3 tangent = { 1.0f, 0.0f, 0.0f };
	const ddVec3 bitangent = { 0.0f, 0.0f, 1.0f };
	origin[1] += 0.1f;
	drawLabel(ctx, origin, "tangent basis");
	dd::tangentBasis(ctx, origin, normal, tangent, bitangent, 2.5f);
	dd::point(ctx, origin, dd::colors::White, 15.0f);

	// And a set of intersecting axes:
	origin[0] += 4.0f;
	origin[1] += 1.0f;
	drawLabel(ctx, origin, "cross");
	dd::cross(ctx, origin, 2.0f);
	dd::point(ctx, origin, dd::colors::White, 15.0f);
}

void drawFrustum(dd::ContextHandle ctx)
{
	const ddVec3 color = { 0.8f, 0.3f, 1.0f };
	const ddVec3 origin = { -8.0f, 0.5f, 14.0f };
	drawLabel(ctx, origin, "frustum + axes");

	// The frustum will depict a fake camera:
	const m4x4 proj = m4x4::CreatePerspectiveFieldOfView(degToRad(45.0f), 800.0f / 600.0f, 0.5f, 4.0f);
	const m4x4 view = m4x4::CreateLookAt(v3(-8.0f, 0.5f, 14.0f), v3(-8.0f, 0.5f, -14.0f), v3::UnitY);

	m4x4 clip = view * proj;
	clip.Invert();

	dd::frustum(ctx, (float*)&clip, color);

	// A white dot at the eye position:
	dd::point(ctx, origin, dd::colors::White, 15.0f);

	// A set of arrows at the camera's origin/eye:
	const m4x4 transform = m4x4::CreateTranslation(v3(-8.0f, 0.5f, 14.0f)) * m4x4::CreateRotationZ(degToRad(60.0f));
	dd::axisTriad(ctx, (float*)&transform, 0.3f, 2.0f);
}

void drawText(dd::ContextHandle ctx)
{
	// HUD text:
	const ddVec3 textColor = { 1.0f,  1.0f,  1.0f };
	const ddVec3 textPos2D = { 10.0f, 15.0f, 0.0f };
	dd::screenText(ctx, "Welcome to the D3D11 Debug Draw demo.\n\n"
		"[SPACE]  to toggle labels on/off\n"
		"[RETURN] to toggle grid on/off",
		textPos2D, textColor, 0.55f);
}

}

static void inputUpdate(const Window & win)
{
	if (GetForegroundWindow() != win.m_hWnd)
	{
		return; // Don't update if window out of focus.
	}

	auto testKeyPressed = [](int key) -> bool
	{
		return (GetKeyState(key) & 0x80) > 0;
	};

	// Keyboard movement keys:
	keys.wDown = testKeyPressed('W') || testKeyPressed(VK_UP);
	keys.sDown = testKeyPressed('S') || testKeyPressed(VK_DOWN);
	keys.aDown = testKeyPressed('A') || testKeyPressed(VK_LEFT);
	keys.dDown = testKeyPressed('D') || testKeyPressed(VK_RIGHT);

	// Mouse buttons:
	mouse.leftButtonDown = testKeyPressed(VK_LBUTTON);
	mouse.rightButtonDown = testKeyPressed(VK_RBUTTON);
}

// ========================================================
// Main entry point for the framework.. 
// called directly from winmain.
// ========================================================

int framework_main(FrameworkApp& rApp, const char* pTitleString, HINSTANCE hInstance, int nCmdShow)
{

	RenderWindowD3D11 renderWindow(hInstance, nCmdShow, pTitleString);
	RenderInterfaceD3D11 renderInterface(renderWindow.m_pD3DDevice, renderWindow.m_pDeviceContext);


	// Initialise the debug drawing library
	dd::ContextHandle ddContext = nullptr;
	dd::initialize(&ddContext, &renderInterface);


	// Initialise the imgui library
	ImGui_ImplDX11_Init(renderWindow.m_hWnd, renderWindow.m_pD3DDevice.Get(), renderWindow.m_pDeviceContext.Get());

	SystemsInterface systems = {};
	systems.pDebugDrawContext = ddContext;
	systems.pD3DDevice = renderWindow.m_pD3DDevice.Get();
	systems.pD3DContext = renderWindow.m_pDeviceContext.Get();
	systems.pSwapRenderTarget = renderWindow.m_pRenderTargetView.Get();
	systems.pCamera = &camera;
	systems.width = Window::s_width;
	systems.height = Window::s_height;

	// Let the application initialise.
	rApp.on_init(systems);

	/////////////////////////////////////////////////////////////
	// Lambda for handling screen resize.
	/////////////////////////////////////////////////////////////
	renderWindow.m_pResizeCallback = [&systems, &renderInterface, &rApp, &renderWindow](u32 width, u32 height) {
		systems.width = width;
		systems.height = height;
		systems.pCamera->resizeViewport(width, height);
		renderInterface.onResize(width, height);

		// Swap chain has changed after a resize update the systems packet.
		systems.pSwapRenderTarget = renderWindow.m_pRenderTargetView.Get();

		// Let the app handle the resize.
		rApp.on_resize(systems);
	};

	/////////////////////////////////////////////////////////////
	// Lambda for handling rendering
	/////////////////////////////////////////////////////////////
	renderWindow.m_pRenderCallback = [&systems, &renderWindow, &renderInterface, &rApp]()
	{
		// Let Imgui prepare for a new frame.
		ImGui_ImplDX11_NewFrame();

		static double prevTime = getTimeSeconds();
		const double t0s = getTimeSeconds();

		deltaTime.seconds = static_cast<float>(t0s-prevTime);
		deltaTime.milliseconds = static_cast<std::int64_t>(deltaTime.seconds * 1000.0);

		
		prevTime = t0s;

		// Plot delta time.
		static float s_times[64];
		static u32 s_timesIdx = 0;
		s_times[s_timesIdx] = deltaTime.seconds;
		s_timesIdx = (s_timesIdx + 1) % 64;
		//ImGui::PlotLines("dT", s_times, 64, s_timesIdx, 0, 0.f, 2.0f/60.0f, ImVec2(400.f, 200.f));
		
		inputUpdate(renderWindow);

		// size may change so update window size
		systems.width = renderWindow.s_width;
		systems.height = renderWindow.s_height;


		if (mouse.rightButtonDown) {
			camera.checkKeyboardMovement();
			camera.checkMouseRotation();
		}

		camera.updateMatrices();

		// Let the application update.
		rApp.on_update(systems);


		m4x4 mvpMatrix = camera.vpMatrix.Transpose();

		renderInterface.setMvpMatrixPtr(mvpMatrix);
		renderInterface.setCameraFrame(camera.up, camera.right, camera.eye);

		// Let the application render.
		rApp.on_render(systems);

		// Flush the debug draw queues:
		dd::flush(systems.pDebugDrawContext);

		// Flush Imgui draw queues
		ImGui::Render();

#ifdef DEAD
		const double t1s = getTimeSeconds();

		deltaTime.seconds = static_cast<float>(prevTime - t0s);
		deltaTime.milliseconds = static_cast<std::int64_t>(deltaTime.seconds * 1000.0);
#endif // DEAD



		// Clear mouse delta
		mouse.deltaX = 0;
		mouse.deltaY = 0;
	};

	/////////////////////////////////////////////////////////////
	// Enter the game loop.
	/////////////////////////////////////////////////////////////
	renderWindow.runMessageLoop();


	/////////////////////////////////////////////////////////////
	// And shutdown
	/////////////////////////////////////////////////////////////
	ImGui_ImplDX11_Shutdown();

	dd::shutdown(ddContext);
	return 0;
}


Camera::Camera()
{
	right = v3(1.0f, 0.0f, 0.0f);
	up = v3(0.0f, 1.0f, 0.0f);
	forward = v3(0.0f, 0.0f, 1.0f);
	eye = v3(0.0f, 0.0f, 0.0f);
	viewMatrix = m4x4::Identity;
	vpMatrix = m4x4::Identity;
	fovY = degToRad(30.0f);
	nearClip = 0.1f;
	farClip = 100.0f;

	for (int i = 0; i < 6; ++i)
	{
		planes[i] = v4(0.0f);
	}

	resizeViewport(Window::s_width, Window::s_height);
}

void Camera::pitch(const float angle)
{
	// Pitches camera by 'angle' radians.
	forward = rotateAroundAxis(forward, right, angle); // Calculate new forward.
	up = forward.Cross(right);                   // Calculate new camera up vector.
}

void Camera::rotate(const float angle)
{
	// Rotates around world Y-axis by the given angle (in radians).
	const float sinAng = std::sin(angle);
	const float cosAng = std::cos(angle);
	float xxx, zzz;

	// Rotate forward vector:
	xxx = forward.x;
	zzz = forward.z;
	forward.x = xxx *  cosAng + zzz * sinAng;
	forward.z = xxx * -sinAng + zzz * cosAng;

	// Rotate up vector:
	xxx = up.x;
	zzz = up.z;
	up.x = xxx *  cosAng + zzz * sinAng;
	up.z = xxx * -sinAng + zzz * cosAng;

	// Rotate right vector:
	xxx = right.x;
	zzz = right.z;
	right.x = xxx *  cosAng + zzz * sinAng;
	right.z = xxx * -sinAng + zzz * cosAng;
}

void Camera::move(const MoveDir dir, const float amount)
{
	switch (dir)
	{
	case Camera::Forward: eye += forward * v3(amount); break;
	case Camera::Back: eye -= forward * v3(amount); break;
#ifdef SIMPLE_MATHS_LEFT_HANDED
	case Camera::Left: eye -= right   * v3(amount); break;
	case Camera::Right: eye += right   * v3(amount); break;
#else
	case Camera::Left: eye += right   * v3(amount); break;
	case Camera::Right: eye -= right   * v3(amount); break;
#endif // SIMPLE_MATHS_LEFT_HANDED
	}
}

void Camera::checkKeyboardMovement()
{
	const float moveSpeed = movementSpeed * deltaTime.seconds;
	if (keys.aDown) { move(Camera::Left, moveSpeed); }
	if (keys.dDown) { move(Camera::Right, moveSpeed); }
	if (keys.wDown) { move(Camera::Forward, moveSpeed); }
	if (keys.sDown) { move(Camera::Back, moveSpeed); }
}

void Camera::checkMouseRotation()
{
	static const float maxAngle = 89.5f; // Max degrees of rotation
	static float pitchAmt;

	const float rotateSpeed = lookSpeed * deltaTime.seconds;

	// Rotate left/right:
	float amtLR = static_cast<float>(mouse.deltaX) * rotateSpeed;
#ifdef SIMPLE_MATHS_LEFT_HANDED
	rotate(degToRad(amtLR));
#else
	rotate(degToRad(-amtLR));
#endif // SIMPLE_MATHS_LEFT_HANDED

	// Calculate amount to rotate up/down:
	float amt = static_cast<float>(mouse.deltaY) * rotateSpeed;

	// Clamp pitch amount:
	if ((pitchAmt + amt) <= -maxAngle)
	{
		amt = -maxAngle - pitchAmt;
		pitchAmt = -maxAngle;
	}
	else if ((pitchAmt + amt) >= maxAngle)
	{
		amt = maxAngle - pitchAmt;
		pitchAmt = maxAngle;
	}
	else
	{
		pitchAmt += amt;
	}

	pitch(degToRad(-amt));

	//debugF("amt=  %d, %d\n", mouse.deltaX, mouse.deltaY);
}


void Camera::resizeViewport(u32 width, u32 height)
{
	aspect = static_cast<float>(width) / static_cast<float>(height);
	projMatrix = m4x4::CreatePerspectiveFieldOfView(fovY, aspect, nearClip, farClip);
	updateMatrices();
}

void Camera::updateMatrices()
{
	viewMatrix = m4x4::CreateLookAt(eye, getTarget(), up);
	vpMatrix = viewMatrix * projMatrix;

	// Compute and normalize the 6 frustum planes:
	const float * const m = reinterpret_cast<float*>(&vpMatrix);
	planes[0].x = m[3] - m[0];
	planes[0].y = m[7] - m[4];
	planes[0].z = m[11] - m[8];
	planes[0].w = m[15] - m[12];
	planes[0].Normalize();
	planes[1].x = m[3] + m[0];
	planes[1].y = m[7] + m[4];
	planes[1].z = m[11] + m[8];
	planes[1].w = m[15] + m[12];
	planes[1].Normalize();
	planes[2].x = m[3] + m[1];
	planes[2].y = m[7] + m[5];
	planes[2].z = m[11] + m[9];
	planes[2].w = m[15] + m[13];
	planes[2].Normalize();
	planes[3].x = m[3] - m[1];
	planes[3].y = m[7] - m[5];
	planes[3].z = m[11] - m[9];
	planes[3].w = m[15] - m[13];
	planes[3].Normalize();
	planes[4].x = m[3] - m[2];
	planes[4].y = m[7] - m[6];
	planes[4].z = m[11] - m[10];
	planes[4].w = m[15] - m[14];
	planes[4].Normalize();
	planes[5].x = m[3] + m[2];
	planes[5].y = m[7] + m[6];
	planes[5].z = m[11] + m[10];
	planes[5].w = m[15] + m[14];
	planes[5].Normalize();
}

void Camera::look_at(const v3& vTarget)
{
	forward = vTarget - eye;
	forward.Normalize();

	up = v3::UnitY;

	right = up.Cross(forward);
	right.Normalize();

	up = forward.Cross(right);
	up.Normalize();
}

v3 Camera::getTarget() const
{
	return eye + forward;
}

bool Camera::pointInFrustum(const v3& v) const
{
	v4 t(v.x, v.y, v.z, 1.0f);
	for (int i = 0; i < 6; ++i)
	{
		if (planes[i].Dot(t) <= 0.0f)
		{
			return false;
		}
	}
	return true;
}
// ========================================================

v3 Camera::rotateAroundAxis(const v3 & vec, const v3 & axis, const float angle)
{
	const float sinAng = std::sin(angle);
	const float cosAng = std::cos(angle);
	const float oneMinusCosAng = (1.0f - cosAng);

	const float aX = axis.x;
	const float aY = axis.y;
	const float aZ = axis.z;

	float x = (aX * aX * oneMinusCosAng + cosAng)      * vec.x +
		(aX * aY * oneMinusCosAng + aZ * sinAng) * vec.y +
		(aX * aZ * oneMinusCosAng - aY * sinAng) * vec.z;

	float y = (aX * aY * oneMinusCosAng - aZ * sinAng) * vec.x +
		(aY * aY * oneMinusCosAng + cosAng)      * vec.y +
		(aY * aZ * oneMinusCosAng + aX * sinAng) * vec.z;

	float z = (aX * aZ * oneMinusCosAng + aY * sinAng) * vec.x +
		(aY * aZ * oneMinusCosAng - aX * sinAng) * vec.y +
		(aZ * aZ * oneMinusCosAng + cosAng)      * vec.z;

	return v3(x, y, z);
}

memtype_t* load_file(const char* pstrName, u32& rLengthOut, const u32 kAlignment, const u32 kZeroPadding)
{
	std::ifstream hFile;

	hFile.open(pstrName, std::ios::binary);
	if (hFile.good())
	{
		hFile.seekg(0, std::ios::end);
		u32 length = static_cast<u32>(hFile.tellg());
		hFile.seekg(0, std::ios::beg);

		rLengthOut = length;
		memtype_t* pMemory = (memtype_t*)_aligned_malloc(length + kZeroPadding, kAlignment);
		ASSERT(pMemory);

		hFile.read((char*)pMemory, length);
		if (kZeroPadding > 0)
		{
			memset(pMemory + length, 0, kZeroPadding);
		}

		hFile.close();

		return pMemory;
	}
	//ASSERT(false && "Couldn't load the file.");
	return nullptr;
}

void release_loaded_file(memtype_t* ptr)
{
	if (ptr) _aligned_free(ptr);
}
