/*****************************************************************//**
 * @file	SpatialGrid.h
 * @brief	空間をセルに分割して管理するクラス
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/01	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___SPATIAL_GRID_H___
#define ___SPATIAL_GRID_H___

// ===== インクルード =====
#include "Engine/ECS/ECS.h"
#include <unordered_map>
#include <vector>
#include <DirectXMath.h>
#include <cmath>

namespace Physics
{
	struct GridKey {
		int x, y, z;
		bool operator==(const GridKey& other) const {
			return x == other.x && y == other.y && z == other.z;
		}
	};

	struct GridKeyHash {
		std::size_t operator()(const GridKey& k) const {
			return ((std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1)) >> 1) ^ (std::hash<int>()(k.z) << 1);
		}
	};

	class SpatialGrid {
	public:
		SpatialGrid(float cellSize = 5.0f) : m_cellSize(cellSize) {}

		void Clear() {
			m_grid.clear();
		}

		// オブジェクトをグリッドに登録
		void Insert(Entity entity, const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max) {

			// 1. 異常値チェック (NaN, Infinity)
			// これがないと、座標が壊れた瞬間にフリーズします
			if (std::isnan(min.x) || std::isnan(min.y) || std::isnan(min.z) ||
				std::isnan(max.x) || std::isnan(max.y) || std::isnan(max.z) ||
				std::isinf(min.x) || std::isinf(max.x))
			{
				// Logger::LogWarning("SpatialGrid: Invalid AABB (NaN/Inf). Entity skipped.");
				return; // 登録しない
			}

			int minX = (int)std::floor(min.x / m_cellSize);
			int maxX = (int)std::floor(max.x / m_cellSize);
			int minY = (int)std::floor(min.y / m_cellSize);
			int maxY = (int)std::floor(max.y / m_cellSize);
			int minZ = (int)std::floor(min.z / m_cellSize);
			int maxZ = (int)std::floor(max.z / m_cellSize);

			// 2. 巨大オブジェクト対策 (ループ回数制限)
			// 床や空などの巨大すぎるオブジェクトが来たら、中心付近のみに登録してフリーズ回避
			const int LIMIT = 50;
			if (std::abs(maxX - minX) > LIMIT ||
				std::abs(maxY - minY) > LIMIT ||
				std::abs(maxZ - minZ) > LIMIT)
			{
				// 範囲を強制的に狭める (中心の1セルだけにする)
				minX = maxX = (minX + maxX) / 2;
				minY = maxY = (minY + maxY) / 2;
				minZ = maxZ = (minZ + maxZ) / 2;
			}

			for (int x = minX; x <= maxX; ++x) {
				for (int y = minY; y <= maxY; ++y) {
					for (int z = minZ; z <= maxZ; ++z) {
						m_grid[{x, y, z}].push_back(entity);
					}
				}
			}
		}

		const std::vector<Entity>& GetCell(const DirectX::XMFLOAT3& position) {
			int x = (int)std::floor(position.x / m_cellSize);
			int y = (int)std::floor(position.y / m_cellSize);
			int z = (int)std::floor(position.z / m_cellSize);
			auto it = m_grid.find({ x, y, z });
			if (it != m_grid.end()) return it->second;
			static const std::vector<Entity> empty;
			return empty;
		}

		const std::unordered_map<GridKey, std::vector<Entity>, GridKeyHash>& GetMap() const {
			return m_grid;
		}

	private:
		float m_cellSize;
		std::unordered_map<GridKey, std::vector<Entity>, GridKeyHash> m_grid;
	};
}

#endif // !___SPATIAL_GRID_H___