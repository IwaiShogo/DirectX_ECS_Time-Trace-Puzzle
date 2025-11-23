/*****************************************************************//**
 * @file	MovementSystem.h
 * @brief	移動ロジック
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

#ifndef ___MOVEMENT_SYSTEM_H___
#define ___MOVEMENT_SYSTEM_H___

// ===== インクルード =====
#include "ECS/ECS.h"
#include "Components/Components.h"

class MovementSystem
	: public ISystem
{
public:
	void Update(Registry& registry) override
	{
		float dt = Time::DeltaTime();

		registry.view<Transform, Velocity>([&](Entity e, Transform& t, Velocity& v) {
			t.position.x += v.velocity.x * dt;
			t.position.y += v.velocity.y * dt;
			t.position.z += v.velocity.z * dt;
		});
	}
};

#endif // !___MOVEMENT_SYSTEM_H___