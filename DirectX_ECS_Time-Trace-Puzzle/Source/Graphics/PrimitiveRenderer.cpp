/*****************************************************************//**
 * @file	PrimitiveRenderer.cpp
 * @brief	箱を描画するための簡易クラスの実装
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
#include "Graphics/PrimitiveRenderer.h"
#include <d3dcompiler.h>
#include <stdexcept>

#pragma comment(lib, "d3dcompiler.lib")

// 立方体の頂点データ
struct Vertex
{
	XMFLOAT3 position;
};

PrimitiveRenderer::PrimitiveRenderer(ID3D11Device* device, ID3D11DeviceContext* context)
	: m_device(device)
	, m_context(context)
{
}

void PrimitiveRenderer::Initialize()
{
	// 1. シェーダーコンパイル
	ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
	UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;

	// Vertex Shader
	HRESULT hr = D3DCompileFromFile(L"Resources/Shaders/DebugPrimitive.hlsl", nullptr, nullptr, "VS", "vs_5_0", compileFlags, 0, &vsBlob, &errorBlob);
	if (FAILED(hr))
	{
		if (errorBlob)OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		throw std::runtime_error("Failed to compile VS");
	}
	m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);

	// Pixel Shader
	hr = D3DCompileFromFile(L"Resources/Shaders/DebugPrimitive.hlsl", nullptr, nullptr, "PS", "ps_5_0", compileFlags, 0, &psBlob, &errorBlob);
	if (FAILED(hr))
	{
		if (errorBlob)OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		throw std::runtime_error("Failed to compile PS");
	}
	m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);

	// 2. 入力レイアウト
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	m_device->CreateInputLayout(layout, 1, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);

	// 3. 頂点バッファ（単位立方体: -0.5f ~ +0.5f）
	float hs = 0.5f;
	Vertex vertices[] = {
		{ XMFLOAT3(-hs, -hs, -hs) }, { XMFLOAT3(-hs, +hs, -hs) },
		{ XMFLOAT3(+hs, +hs, -hs) }, { XMFLOAT3(+hs, -hs, -hs) },
		{ XMFLOAT3(-hs, -hs, +hs) }, { XMFLOAT3(-hs, +hs, +hs) },
		{ XMFLOAT3(+hs, +hs, +hs) }, { XMFLOAT3(+hs, -hs, +hs) },
	};
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex) * 8;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices;
	m_device->CreateBuffer(&bd, &initData, &m_vertexBuffer);

	// 4. インデックスバッファ（ワイヤーフレーム用 LineList）
	// 立方体は6面、1面当たり三角形2枚、計12枚の三角形 = 36インデックス
	uint16_t indices[] = {
		// 手前（Z-）
		0, 1, 2,   0, 2, 3,
		// 奥（Z+）
		4, 6, 5,   4, 7, 6,
		// 上（Y+）
		1, 5, 6,   1, 6, 2,
		// 下（Y-）
		0, 3, 7,   0, 7, 4,
		// 左（X-）
		0, 4, 5,   0, 5, 1,
		// 右（X+）
		3, 2, 6,   3, 6, 7,
	};
	bd.ByteWidth = sizeof(indices);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	initData.pSysMem = indices;
	m_device->CreateBuffer(&bd, &initData, &m_indexBuffer);

	// 5. 定数バッファ
	bd.ByteWidth = sizeof(ConstantBufferData);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	m_device->CreateBuffer(&bd, nullptr, &m_constantBuffer);

	// 線描画用の動的頂点バッファ
	bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * 2;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	m_device->CreateBuffer(&bd, nullptr, &m_lineVertexBuffer);

	// ラスタライザステート作成
	D3D11_RASTERIZER_DESC rd = {};
	rd.CullMode = D3D11_CULL_NONE;	// 両面描画
	rd.FrontCounterClockwise = FALSE;
	rd.DepthClipEnable = TRUE;

	// ワイヤーフレーム用
	rd.FillMode = D3D11_FILL_WIREFRAME;
	m_device->CreateRasterizerState(&rd, &m_rsWireframe);

	// ソリッド用
	rd.FillMode = D3D11_FILL_SOLID;
	m_device->CreateRasterizerState(&rd, &m_rsSolid);
}

void PrimitiveRenderer::Begin(const XMMATRIX& view, const XMMATRIX& projection)
{
	// 共通設定
	m_context->IASetInputLayout(m_inputLayout.Get());
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	m_context->VSSetShader(m_vs.Get(), nullptr, 0);
	m_context->PSSetShader(m_ps.Get(), nullptr, 0);
	m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	m_context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	// ビュー・プロジェクション行列を保存
	m_cbData.view = XMMatrixTranspose(view);
	m_cbData.projection = XMMatrixTranspose(projection);
}

void PrimitiveRenderer::DrawBox(const XMFLOAT3& position, const XMFLOAT3& size, const XMFLOAT4& color)
{
	// ワールド行列作成（Scale -> Rotate -> Translate）
	XMMATRIX world = XMMatrixScaling(size.x, size.y, size.z) * XMMatrixTranslation(position.x, position.y, position.z);

	// 定数バッファ更新
	m_cbData.world = XMMatrixTranspose(world);
	m_cbData.color = color;

	m_context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &m_cbData, 0, 0);

	// 描画
	m_context->DrawIndexed(36, 0, 0);
}

void PrimitiveRenderer::DrawLine(const XMFLOAT3& p1, const XMFLOAT3& p2, const XMFLOAT4& color)
{
	// 1. 定数バッファ更新（ワールド行列は単位行列にする）
	m_cbData.world = XMMatrixIdentity();
	m_cbData.color = color;
	m_context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &m_cbData, 0, 0);

	// 2. 動的頂点バッファの書き換え
	D3D11_MAPPED_SUBRESOURCE ms;
	if (SUCCEEDED(m_context->Map(m_lineVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
	{
		Vertex* v = (Vertex*)ms.pData;
		v[0].position = p1;
		v[1].position = p2;
		m_context->Unmap(m_lineVertexBuffer.Get(), 0);
	}

	// 3. パイプライン設定変更
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	m_context->IASetVertexBuffers(0, 1, m_lineVertexBuffer.GetAddressOf(), &stride, &offset);
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// 4.描画
	m_context->Draw(2, 0);

	// 5. バッファをBox用のものに戻しておく
	m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void PrimitiveRenderer::DrawArrow(const XMFLOAT3& start, const XMFLOAT3& end, const XMFLOAT4& color)
{
	// 1. 軸の線を書く
	DrawLine(start, end, color);

	// 2. 先端に小さな箱を書く
	float boxSize = 0.2f;
	DrawBox(end, XMFLOAT3(boxSize, boxSize, boxSize), color);
}

void PrimitiveRenderer::SetFillMode(bool wireframe)
{
	if (wireframe)
	{
		m_context->RSSetState(m_rsWireframe.Get());
	}
	else
	{
		m_context->RSSetState(m_rsSolid.Get());
	}
}

void PrimitiveRenderer::DrawGrid(float spacing, int lines)
{
	// グリッドは常にワイヤーフレームで書く
	m_context->RSSetState(m_rsWireframe.Get());

	float size = static_cast<float>(lines) * spacing;
	XMFLOAT4 color = { 0.5f, 0.5f, 0.5f, 1.0f };
	
	// X軸に平行な線
	for (int i = -lines; i <= lines; ++i)
	{
		float pos = static_cast<float>(i) * spacing;

		// 横線
		DrawLine(XMFLOAT3(-size, 0, pos), XMFLOAT3(size, 0, pos), color);
		// 縦線
		DrawLine(XMFLOAT3(pos, 0, -size), XMFLOAT3(pos, 0, size), color);
	}
}

// 座標軸描画
void PrimitiveRenderer::DrawAxis(float length)
{
	// X軸（赤）
	DrawLine(XMFLOAT3(0, 0, 0), XMFLOAT3(length, 0, 0), XMFLOAT4(1, 0, 0, 1));
	// Y軸（緑）
	DrawLine(XMFLOAT3(0, 0, 0), XMFLOAT3(0, length, 0), XMFLOAT4(0, 1, 0, 1));
	// Z軸（青）
	DrawLine(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, length), XMFLOAT4(0, 0, 1, 1));
}