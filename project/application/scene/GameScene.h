#pragma once
#include "Audio.h"
#include "Camera.h"
#include "DebugCamera.h"
#include "Egg.h"
#include "Goal.h"
#include "MathFunction.h"
#include "Model.h"
#include "Object3D.h"
#include "ParicleEmitter.h"
#include "Player.h"
#include "Scene.h"
#include "Sprite.h"
#include "Terrain.h"
#include "TextureManager.h"
#include "Enemy.h"
#include <memory>
#include "CollisionMask.h"

#include "ThreadManager.h"

class GameScene : public Scene {
public:

    void Initialize() override;
    void Finalize() override;
    void Update() override;
    void Draw() override;
  
  void CheckAllCollisions(); // 全ての当たり判定を確認
  bool isCollision(const AABB& aabb1, const AABB& aabb2); // 当たり判定
  void ResolveCollision(Player* player, const AABB& playerAABB, const AABB& otherAABB); // プレイヤーを押し戻す関数

private:
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Sprite> sprite;
    std::unique_ptr<Object3d> object3d2;
    std::unique_ptr<Object3d> object3d;
    std::unique_ptr<ParicleEmitter> emitter;
    uint32_t handle_ = 0;

    DebugCamera debugCamera_;
    bool isDebugCamera_ = false;

    /*Player *player_;
    Terrain *terrain_;*/
    std::unique_ptr<Egg> egg_;
    std::unique_ptr<Goal> goal_;
    /*Player* player_;
    Terrain* terrain_;*/

    CollisionMask* collisionMask_;

  // ----- Player -----
  std::unique_ptr<Player> player_;

  // プレイヤーの位置
  Vector3 playerPos_ = {-10.0f, 0.0f, 0.0f};

  // ----- Thread -----
  std::unique_ptr<ThreadManager> thread_;

  // 敵
  std::unique_ptr<Enemy> enemy_;
  // 敵の位置
  Vector3 enemyPos_ = { 10.0f,0.0f,4.0f };
};
