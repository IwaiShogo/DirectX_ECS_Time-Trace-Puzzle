/*****************************************************************//**
 * @file	CollisionSystem.h
 * @brief	衝突検出
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/24	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___COLLISION_SYSTEM_H___
#define ___COLLISION_SYSTEM_H___

// ===== インクルード =====
#include "ECS/ECS.h"
#include "Components/Components.h"

class CollisionSystem
	: public ISystem
{
public:
	CollisionSystem() { m_systemName = "Collision System"; }

	void Update(Registry& registry) override;

	/**
	 * @brief	レイキャスト判定
	 * @param	[in] registry	レジストリ
	 * @param	[in] rayOrigin	開始点
	 * @param	[in] rayDir		方向（正規化済み）
	 * @param	[in] outDist	衝突距離
	 * @return	
	 * @note	（省略可）
	 */
	static Entity Raycast(Registry& registry, const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDir, float& outDist);

private:
	// AABB同士の交差判定関数
	bool CheckAABB(	const XMFLOAT3& posA, const BoxCollider& boxA,
					const XMFLOAT3& posB, const BoxCollider& boxB);
};

#endif // !___COLLISION_SYSTEM_H___