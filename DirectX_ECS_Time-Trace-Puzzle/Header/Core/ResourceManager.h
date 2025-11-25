/*****************************************************************//**
 * @file	ResourceManager.h
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

#ifndef ___RESOURCE_MANAGER_H___
#define ___RESOURCE_MANAGER_H___

// ===== インクルード =====
#include <map>
#include <string>
#include <memory>
#include <d3d11.h>
#include "Graphics/Texture.h"
#include "json.hpp"

class ResourceManager
{
public:
	// シングルトン取得
	static ResourceManager& Instance()
	{
		static ResourceManager instance;
		return instance;
	}

	// 初期化（Deviceが必要）
	void Initialize(ID3D11Device* device);

	// マニフェスト（JSON）を読み込んでパスを登録
	void LoadManifest(const std::string& jsonPath);

	// キー名からテクスチャを取得（"player"など）
	std::shared_ptr<Texture> GetTexture(const std::string& key);

private:
	// 内部ロード関数（パス指定）
	std::shared_ptr<Texture> LoadTextureFromFile(const std::string& filepath);

	ResourceManager() = default;	// コンストラクタ隠蔽
	~ResourceManager() = default;

	ID3D11Device* m_device = nullptr;

	// キー名 -> ファイルパス の辞書
	std::map<std::string, std::string> m_texturePaths;

	// ファイルパス -> 実データ のキャッシュ
	std::map<std::string, std::shared_ptr<Texture>> m_textures;
};

#endif // !___RESOURCE_MANAGER_H___