/*****************************************************************//**
 * @file	Application.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/23	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Core/Application.h"
#include "Core/Time.h"
#include "Core/Input.h"
#include "Core/ResourceManager.h"
#include "Core/AudioManager.h"
#include "main.h"
#include <string>
#include <stdexcept>

Application::Application(HWND hwnd)
	: m_hwnd(hwnd)
{
}

Application::~Application()
{
#ifdef _DEBUG
	// ImGui 終了処理
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif // _DEBUG

	// オーディオ
	AudioManager::Instance().Finalize();

	// ComPtrを使用しているため、明示的なReleaseは不要
}

void Application::Initialize()
{
	// 1. スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC scd = {};
	scd.BufferCount = 1;
	scd.BufferDesc.Width = Config::SCREEN_WIDTH;
	scd.BufferDesc.Height = Config::SCREEN_HEIGHT;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.RefreshRate.Numerator = Config::FRAME_RATE;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = m_hwnd;
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;
	scd.Windowed = TRUE;

	// 2. デバイスとスワップチェーンの作成
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL featureLevel;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		featureLevels,
		1,
		D3D11_SDK_VERSION,
		&scd,
		&m_swapChain,
		&m_device,
		&featureLevel,
		&m_context
	);

	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create D3D11 device");
	}

	// 3. レンダーターゲットビュー（RTV）の作成
	ComPtr<ID3D11Texture2D> backBuffer;
	hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to get back buffer");
	}

	hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create RTV");
	}

	// 4. 深度バッファ（Z-Buffer）の作成
	D3D11_TEXTURE2D_DESC depthBufferDesc = {};
	depthBufferDesc.Width = Config::SCREEN_WIDTH;
	depthBufferDesc.Height = Config::SCREEN_HEIGHT;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	// テクスチャ作成
	ComPtr<ID3D11Texture2D> depthStencilBuffer;
	hr = m_device->CreateTexture2D(&depthBufferDesc, nullptr, &depthStencilBuffer);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create Depth Stencil Buffer");
	}

	// ビュー作成
	hr = m_device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, &m_depthStencilView);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create Depth Stencil View");
	}

	// レンダーターゲットをセット
	m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

	// ビューポート設定
	D3D11_VIEWPORT vp = {};
	vp.Width = static_cast<float>(Config::SCREEN_WIDTH);
	vp.Height = static_cast<float>(Config::SCREEN_HEIGHT);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_context->RSSetViewports(1, &vp);

	// 時間管理の初期化
	Time::Initialize();
	Time::SetFrameRate(Config::FRAME_RATE);

	// 入力
	Input::Initialize();

#ifdef _DEBUG
	// --- ImGui ---
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// ドッキングとマルチビューポートを有効化
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	// キーボード操作有効
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		// ウィンドウドッキング有効
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;		// 別ウィンドウ化を有効

	// スタイル調整
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowBorderHoverPadding = 1.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
	
	// Win32 / DX11 バインディングの初期化
	ImGui_ImplWin32_Init(m_hwnd);
	ImGui_ImplDX11_Init(m_device.Get(), m_context.Get());
#endif // _DEBUG

	// マネージャー
	ResourceManager::Instance().Initialize(m_device.Get());
	AudioManager::Instance().Initialize();	// オーディオ初期化
	ResourceManager::Instance().LoadManifest("Resources/resources.json");
	ResourceManager::Instance().LoadAll();

	// 3D描画作成
	m_primitiveRenderer = std::make_unique<PrimitiveRenderer>(m_device.Get(), m_context.Get());
	m_primitiveRenderer->Initialize();

	// 2D描画作成
	m_spriteRenderer = std::make_unique<SpriteRenderer>(m_device.Get(), m_context.Get());
	m_spriteRenderer->Initialize();

	// モデル描画作成
	m_modelRenderer = std::make_unique<ModelRenderer>(m_device.Get(), m_context.Get());
	m_modelRenderer->Initialize();

	// ビルボードレンダラー作成
	m_billboardRenderer = std::make_unique<BillboardRenderer>(m_device.Get(), m_context.Get());
	m_billboardRenderer->Initialize();

	Context context;
	context.renderer = m_primitiveRenderer.get();
	context.spriteRenderer = m_spriteRenderer.get();
	context.modelRenderer = m_modelRenderer.get();
	context.billboardRenderer = m_billboardRenderer.get();

	// シーンマネージャ
	m_sceneManager.SetContext(context);
	m_appContext = context;
	m_sceneManager.SetContext(m_appContext);
	m_sceneManager.Initialize(SceneType::Title);
}

void Application::Update()
{
	// 毎フレーム時間を更新
	Time::Update();

	// 入力
	Input::Update();
	// オーディオ
	AudioManager::Instance().Update();

#ifdef _DEBUG
	// --- ImGui ---
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif // _DEBUG

	// シーン更新
	m_sceneManager.Update();
}

void Application::Render()
{
	// 1. レンダーターゲットをセット
	m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

	// 2. ビューポート設定
	D3D11_VIEWPORT vp = {};
	vp.Width = static_cast<float>(Config::SCREEN_WIDTH);
	vp.Height = static_cast<float>(Config::SCREEN_HEIGHT);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_context->RSSetViewports(1, &vp);

	float color[] = { 0.1f, 0.1f, 0.3f, 1.0f };
	m_context->ClearRenderTargetView(m_renderTargetView.Get(), color);

	// 深度バッファのクリア
	if (m_depthStencilView)
	{
		m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	// シーン描画
	m_sceneManager.Render();

	// 後始末
	m_context->OMSetDepthStencilState(nullptr, 0);
	m_context->OMSetBlendState(nullptr, nullptr, 0xffffffff);

#ifdef _DEBUG
	// --- ImGui ---
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
#endif // _DEBUG

	m_swapChain->Present(Config::VSYNC_ENABLED ? 1: 0, 0);
}

void Application::Run()
{
	// 1. 更新と描画
	Update();
	Render();

	// 2. フレームレート調整（待機）
	// Timeクラスが自動で残りの時間を計算して待ってくれます
	Time::WaitFrame();
}