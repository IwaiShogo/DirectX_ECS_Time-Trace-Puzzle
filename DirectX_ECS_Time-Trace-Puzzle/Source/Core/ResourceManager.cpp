/*****************************************************************//**
 * @file	ResourceManager.cpp
 * @brief	リソースマネージャー
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/25	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Core/ResourceManager.h"
#include <DirectXTex.h>
#include <filesystem>
#include <iostream>
#include <fstream>

// ワイド文字列変換用ヘルパー
std::wstring ToWString(const std::string& str)
{
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

void ResourceManager::Initialize(ID3D11Device* device)
{
	m_device = device;
}

void ResourceManager::LoadManifest(const std::string& jsonPath) {
	std::ifstream file(jsonPath);
	if (!file.is_open()) {
		OutputDebugStringA(("Failed to open manifest: " + jsonPath + "\n").c_str());
		return;
	}

	try {
		nlohmann::json j;
		file >> j;

		// "textures" セクションを読み込む
		if (j.contains("textures")) {
			for (auto& element : j["textures"].items()) {
				std::string key = element.key();			// "player"
				std::string path = element.value()["path"]; // "Assets/Textures/..."

				// パスを登録
				m_texturePaths[key] = path;

				// ログ (任意)
				// OutputDebugStringA(("Registered Texture: " + key + " -> " + path + "\n").c_str());
			}
		}
	}
	catch (const std::exception& e) {
		OutputDebugStringA(("JSON Parse Error: " + std::string(e.what()) + "\n").c_str());
	}
}

// キー名から取得
std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string& key) {
	// 1. キーが登録されているか確認
	auto itPath = m_texturePaths.find(key);
	if (itPath == m_texturePaths.end()) {
		OutputDebugStringA(("Texture Key Not Found: " + key + "\n").c_str());

		// 未登録の場合、キー自体をパスとみなしてトライする（開発中の利便性のため）
		// return LoadTextureFromFile(key); 
		return nullptr;
	}

	std::string filepath = itPath->second;

	// 2. キャッシュチェック
	auto itCache = m_textures.find(filepath);
	if (itCache != m_textures.end()) {
		return itCache->second;
	}

	// 3. ロードしてキャッシュ
	return LoadTextureFromFile(filepath);
}

// 内部ロード関数 (前回のLoadTextureの中身を移動)
std::shared_ptr<Texture> ResourceManager::LoadTextureFromFile(const std::string& filepath) {
	// ※ 前回の LoadTexture の中身をそのままここにコピペしてください ※
	// (キャッシュチェック部分は削除してOKですが、残っていても二重チェックになるだけなので問題ありません)

	// --- 以下コピペ用再掲 ---
	if (!std::filesystem::exists(filepath)) {
		OutputDebugStringA(("Texture Not Found: " + filepath + "\n").c_str());
		return nullptr;
	}

	DirectX::ScratchImage image;
	HRESULT hr = E_FAIL;
	std::wstring wpath = ToWString(filepath);
	std::string ext = std::filesystem::path(filepath).extension().string();

	if (ext == ".dds" || ext == ".DDS")
		hr = DirectX::LoadFromDDSFile(wpath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	else if (ext == ".tga" || ext == ".TGA")
		hr = DirectX::LoadFromTGAFile(wpath.c_str(), nullptr, image);
	else
		hr = DirectX::LoadFromWICFile(wpath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);

	if (FAILED(hr)) return nullptr;

	auto texture = std::make_shared<Texture>();
	texture->filepath = filepath;
	texture->width = static_cast<int>(image.GetMetadata().width);
	texture->height = static_cast<int>(image.GetMetadata().height);

	hr = DirectX::CreateShaderResourceView(m_device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), &texture->srv);
	if (FAILED(hr)) return nullptr;

	m_textures[filepath] = texture; // パスでキャッシュ
	return texture;
}