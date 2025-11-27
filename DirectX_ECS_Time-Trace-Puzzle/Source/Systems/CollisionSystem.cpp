/*****************************************************************//**
 * @file	CollisionSystem.cpp
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

// ===== インクルード =====
#define NOMINMAX
#include "Systems/CollisionSystem.h"
#include <cmath>
#include <iostream>

void CollisionSystem::Update(Registry& registry)
{
	// 対象となるEntityの情報を集める
	struct ColliderData {
		Entity entity;
		XMFLOAT3 globalPosition; // ワールド座標
		XMFLOAT3 globalScale;	 // ワールドスケール
		BoxCollider* collider;
	};
	std::vector<ColliderData> colliders;

	registry.view<Transform, BoxCollider>([&](Entity e, Transform& t, BoxCollider& b) {
		// ★ここが修正ポイント
		// worldMatrix から「ワールド座標」と「ワールドスケール」を取り出す
		XMVECTOR scale, rot, pos;
		XMMatrixDecompose(&scale, &rot, &pos, t.worldMatrix);

		XMFLOAT3 gPos, gScale;
		XMStoreFloat3(&gPos, pos);
		XMStoreFloat3(&gScale, scale);

		colliders.push_back({ e, gPos, gScale, &b });
		});

	size_t count = colliders.size();
	if (count < 2) return;

	// 総当たり判定
	for (size_t i = 0; i < count; ++i)
	{
		for (size_t j = i + 1; j < count; ++j)
		{
			// 判定実行 (抽出したワールド座標を使う)
			// ※スケールも考慮して判定サイズを変えるとなお良いです

			// スケール考慮版のボックスサイズを計算
			BoxCollider scaledBoxA = *colliders[i].collider;
			scaledBoxA.size.x *= colliders[i].globalScale.x;
			scaledBoxA.size.y *= colliders[i].globalScale.y;
			scaledBoxA.size.z *= colliders[i].globalScale.z;

			BoxCollider scaledBoxB = *colliders[j].collider;
			scaledBoxB.size.x *= colliders[j].globalScale.x;
			scaledBoxB.size.y *= colliders[j].globalScale.y;
			scaledBoxB.size.z *= colliders[j].globalScale.z;

			if (CheckAABB(colliders[i].globalPosition, scaledBoxA,
						  colliders[j].globalPosition, scaledBoxB))
			{
				// 1. Entity IDを取得
				Entity entA = colliders[i].entity;
				Entity entB = colliders[j].entity;

				// 2. Tagコンポーネントを確認して正体を判別
				// (Tagを持っていない場合のエラー回避のため has<Tag> をチェック)
				bool isPlayerA = registry.has<Tag>(entA) && registry.get<Tag>(entA).name == "Player";
				bool isEnemyA = registry.has<Tag>(entA) && registry.get<Tag>(entA).name == "Enemy";

				bool isPlayerB = registry.has<Tag>(entB) && registry.get<Tag>(entB).name == "Player";
				bool isEnemyB = registry.has<Tag>(entB) && registry.get<Tag>(entB).name == "Enemy";

				// 3. 「プレイヤー」と「敵」の衝突の場合
				if ((isPlayerA && isEnemyB) || (isPlayerB && isEnemyA))
				{
					Logger::LogError("Player Hit Enemy!");

					// プレイヤーの方を小さくするリアクション
					// colliders[i].collider はポインタなので -> でアクセスします
					if (isPlayerA) {
						colliders[i].collider->size = XMFLOAT3(0.5f, 0.5f, 0.5f);
					}
					if (isPlayerB) {
						colliders[j].collider->size = XMFLOAT3(0.5f, 0.5f, 0.5f);
					}
				}
			}
		}
	}
}

// レイとAABBの交差判定 (Slabs method)
bool IntersectRayAABB(const XMFLOAT3& origin, const XMFLOAT3& dir, const XMFLOAT3& boxPos, const BoxCollider& box, float& t) {
	// AABBの最小・最大点
	float minX = boxPos.x - box.size.x * 0.5f;
	float maxX = boxPos.x + box.size.x * 0.5f;
	float minY = boxPos.y - box.size.y * 0.5f;
	float maxY = boxPos.y + box.size.y * 0.5f;
	float minZ = boxPos.z - box.size.z * 0.5f;
	float maxZ = boxPos.z + box.size.z * 0.5f;

	float tmin = 0.0f;
	float tmax = 10000.0f;

	// X軸
	if (abs(dir.x) < 1e-6f) {
		if (origin.x < minX || origin.x > maxX) return false;
	}
	else {
		float ood = 1.0f / dir.x;
		float t1 = (minX - origin.x) * ood;
		float t2 = (maxX - origin.x) * ood;
		if (t1 > t2) std::swap(t1, t2);
		tmin = std::max(tmin, t1);
		tmax = std::min(tmax, t2);
		if (tmin > tmax) return false;
	}

	// Y軸
	if (abs(dir.y) < 1e-6f) {
		if (origin.y < minY || origin.y > maxY) return false;
	}
	else {
		float ood = 1.0f / dir.y;
		float t1 = (minY - origin.y) * ood;
		float t2 = (maxY - origin.y) * ood;
		if (t1 > t2) std::swap(t1, t2);
		tmin = std::max(tmin, t1);
		tmax = std::min(tmax, t2);
		if (tmin > tmax) return false;
	}

	// Z軸
	if (abs(dir.z) < 1e-6f) {
		if (origin.z < minZ || origin.z > maxZ) return false;
	}
	else {
		float ood = 1.0f / dir.z;
		float t1 = (minZ - origin.z) * ood;
		float t2 = (maxZ - origin.z) * ood;
		if (t1 > t2) std::swap(t1, t2);
		tmin = std::max(tmin, t1);
		tmax = std::min(tmax, t2);
		if (tmin > tmax) return false;
	}

	t = tmin;
	return true;
}

Entity CollisionSystem::Raycast(Registry& registry, const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDir, float& outDist) {
	Entity closestEntity = NullEntity;
	float closestDist = FLT_MAX;

	registry.view<Transform, BoxCollider>([&](Entity e, Transform& t, BoxCollider& b) {
		float dist = 0.0f;
		if (IntersectRayAABB(rayOrigin, rayDir, t.position, b, dist)) {
			if (dist < closestDist && dist > 0.0f) {
				closestDist = dist;
				closestEntity = e;
			}
		}
		});

	outDist = closestDist;
	return closestEntity;
}

// AABB判定ロジック
bool CollisionSystem::CheckAABB(const XMFLOAT3& posA, const BoxCollider& boxA, const XMFLOAT3& posB, const BoxCollider& boxB)
{
	// Aの最小・最大
	float minXA = posA.x - boxA.size.x / 2.0f;
	float maxXA = posA.x + boxA.size.x / 2.0f;
	float minYA = posA.y - boxA.size.y / 2.0f;
	float maxYA = posA.y + boxA.size.y / 2.0f;
	float minZA = posA.z - boxA.size.z / 2.0f;
	float maxZA = posA.z + boxA.size.z / 2.0f;
	
	// Bの最小・最大
	float minXB = posB.x - boxB.size.x / 2.0f;
	float maxXB = posB.x + boxB.size.x / 2.0f;
	float minYB = posB.y - boxB.size.y / 2.0f;
	float maxYB = posB.y + boxB.size.y / 2.0f;
	float minZB = posB.z - boxB.size.z / 2.0f;
	float maxZB = posB.z + boxB.size.z / 2.0f;

	// 全軸で重なっていれば衝突
	bool collisionX = maxXA >= minXB && maxXB >= minXA;
	bool collisionY = maxYA >= minYB && maxYB >= minYA;
	bool collisionZ = maxZA >= minZB && maxZB >= minZA;

	return collisionX && collisionY && collisionZ;
}