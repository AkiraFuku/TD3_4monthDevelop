#include "GameScene.h"
#include "Input.h"
#include "LightManager.h"
#include "ModelManager.h"
#include "PSOManager.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "imgui.h"
#include <numbers>
#include "Transform.h"

void GameScene::Initialize() {

    camera = std::make_unique<Camera>();
    camera->SetRotate({0.80f, 0.0f, 0.0f});
    camera->SetTranslate({0.0f, 30.0f, -30.0f});
    Object3dCommon::GetInstance()->SetDefaultCamera(camera.get());
    ParticleManager::GetInstance()->Setcamera(camera.get());

    /* handle_ = Audio::GetInstance()->LoadAudio("resources/fanfare.mp3");

     Audio::GetInstance()->PlayAudio(handle_, true);*/
     // LightManager::GetInstance()->AddDirectionalLight( { 1,1,1,1 }, { 0,-1,0
     // }, 1.0f); // メインライト LightManager::GetInstance()->AddDirectionalLight(
     // { 1,1,1,1 }, { 0,-1,0 }, 1.0f); // メインライト
    LightManager::GetInstance()->AddSpotLight(
        {1.0f, 1.0f, 1.0f, 1.0f}, {2.0f, 1.25f, 0.0f}, 4.0f,
        Normalize(Vector3 {-1.0f, -1.0f, 0.0f}), 7.0f, 2.0f,
        std::cos(std::numbers::pi_v<float> / 3.0f), 1.0f); // メインライト
    LightManager::GetInstance()->AddSpotLight(
        {1.0f, 1.0f, 1.0f, 1.0f}, {2.0f, 1.25f, 0.0f}, 4.0f,
        Normalize(Vector3 {-1.0f, -1.0f, 0.0f}), 7.0f, 2.0f,
        std::cos(std::numbers::pi_v<float> / 3.0f), 1.0f); // メインライト

    Vector3 point1 = {0, 0, 0};
    LightManager::GetInstance()->AddPointLight({1.0f, 1.0f, 1.0f, 1.0f}, point1,
        4.0f, 2.0f, 0.1f);
    LightManager::GetInstance()->AddPointLight({1.0f, 1.0f, 1.0f, 1.0f},
        {0, 0, 0}, 4.0f, 2.0f, 0.1f);
    TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

    ParticleManager::GetInstance()->CreateParticleGroup(
        "Test", "resources/uvChecker.png");
    /*   std::vector<Sprite*> sprites;
       for (uint32_t i = 0; i < 5; i++)
       {*/
    sprite = std::make_unique<Sprite>();
    // sprite->Initialize("resources/monsterBall.png");
    sprite->Initialize("resources/uvChecker.png");

    sprite->SetPosition(Vector2 {25.0f + 100.0f, 100.0f});
    // sprite->SetSize(Vector2{ 100.0f,100.0f });
    // sprites.push_back(sprite);

    sprite->SetAnchorPoint(Vector2 {0.5f, 0.5f});

    //}

    // object3d の初期化
    object3d2 = std::make_unique<Object3d>();
    object3d2->Initialize();

    object3d = std::make_unique<Object3d>();
    object3d->Initialize();

    ModelManager::GetInstance()->LoadModel("resources", "plane.obj");
    ModelManager::GetInstance()->LoadModel("resources", "axis.obj");
    ModelManager::GetInstance()->LoadModel("resources", "terrain.obj");
    ModelManager::GetInstance()->CreateSphereModel("MySphere", 16);
    // object3d2->SetTranslate(Vector3{ 0.0f,10.0f,0.0f });
    object3d2->SetModel("terrain.obj");
    object3d->SetModel("MySphere");

    /*camera->SetTranslate({ 0.0f,0.0f,-10.0f });*/
    camera->SetFarCrip(1000.0f);
    EulerTransform M = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    emitter = std::make_unique<ParicleEmitter>("Test", M, 10, 5.0f, 0.0f);

    /*player_ = new Player();
    player_->Initialize();

    terrain_ = new Terrain();
    terrain_->Initialize();*/

    // ----- Thread -----
    thread_ = std::make_unique<ThreadManager>();
    thread_->Initialize(50, 20, camera.get());
    //thread_->AddThread({0.0f, 0.0f, 0.0f}, {8.0f, 0.0f, 0.0f});
    //thread_->AddThread({-5.0f, 0.0f, -5.0f}, {-5.0f, 0.0f, 5.0f});
    spiderWeb_ = std::make_unique<SpiderWebManager>();
    spiderWeb_->Initialize(camera.get());

    // プレイヤーの初期化
    player_ = std::make_unique<Player>();
    player_->Initialize(playerPos_, thread_.get());
    player_->SetMaxThreadCount(5);

    // 卵の初期化
    egg_ = std::make_unique<Egg>();
    egg_->Initialize(eggPos);
    player_->SetEgg(egg_.get());
    egg_->SetPlayer(player_.get());

    // ゴールの初期化
    goal_ = std::make_unique<Goal>();
    goal_->Initialize(goalPos);

    goal_->SetEgg(egg_.get());
    goal_->SetPlayer(player_.get());
    goal_->SetNeedNestCount(1);


    collisionMask_ = CollisionMask::GetInstance();
    collisionMask_->Initialize();

    // 敵の初期化

    /*enemy_ = std::make_unique<Enemy>();
    enemy_->Initialize(enemyPos_);*/

    enemyPositions_ = {
        {3.0f, 0.0f, 10.0f},
        {8.0f, 0.0f, -5.0f}
    };

    for (const auto& pos : enemyPositions_) {
        auto enemy = std::make_unique<Enemy>();
        enemy->Initialize(pos);
        enemies_.push_back(std::move(enemy)); // リストに追加
    }


    // 巣の素材の初期化
    nestMaterial_ = std::make_unique<NestMaterial>();
    nestMaterial_->Initialize(nestMaterialPos_);

}
void GameScene::Finalize() {

    LightManager::GetInstance()->ClearLights();

    ParticleManager::GetInstance()->ReleaseParticleGroup("Test");

    collisionMask_->Finalize();

    /*player_->Finalize();
    delete player_;

    terrain_->Finalize();
    delete terrain_;

    egg_->Finalize();
    delete egg_;

    goal_->Finalize();
    delete goal_;*/

#ifdef _DEBUG



#endif

}

void GameScene::Update() {
    emitter->Update();

    XINPUT_STATE state;

    // 現在のジョイスティックを取得

    Input::GetInstance()->GetJoyStick(0, state);

    // Aボタンを押していたら

    //if (Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_A)) {

    //    // Aボタンを押したときの処理


       /* if (Audio::GetInstance()->IsPlaying(handle_)) {

            Audio::GetInstance()->StopAudio(handle_);
        }*/

        //    GetSceneManager()->ChangeScene("TitleScene");
        //}
        //if (Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_B)) {
        //}

        // マウスホイールの入力取得

    if (Input::GetInstance()->GetMouseMove().z) {
        Vector3 cameraTranslate = camera->GetTranslate();
        cameraTranslate =
            Add(cameraTranslate,
                Vector3 {0.0f, 0.0f,
                static_cast<float>(Input::GetInstance()->GetMouseMove().z) *
                0.1f});
        camera->SetTranslate(cameraTranslate);
    }
    /*if (Input::GetInstance()->TriggerMouseDown(0))
    {
        if (!Audio::GetInstance()->IsPlaying(handle_))
        {
            Audio::GetInstance()->PlayAudio(handle_);
        }

    }*/
    /*if (Input::GetInstance()->TriggerMouseDown(0)) {
        if (Audio::GetInstance()->IsPlaying(handle_)) {
            Audio::GetInstance()->PauseAudio(handle_);
        }
        else {
            Audio::GetInstance()->ResumeAudio(handle_);
        }
            }*/

            //if (Input::GetInstance()->GetJoyStick(0, state)) {
            //    {
            //        // 左スティックの値を取得
            //        float x = (float) state.Gamepad.sThumbLX;
            //        float y = (float) state.Gamepad.sThumbLY;

            //        // 数値が大きいので正規化（-1.0 ～ 1.0）して使うのが一般的
            //        float normalizedX = x / 32767.0f;
            //        float normalizedY = y / 32767.0f;
            //        Vector3 cameraTranslate = camera->GetTranslate();
            //        cameraTranslate =
            //            Add(cameraTranslate,
            //                Vector3 {normalizedX / 60.0f, normalizedY / 60.0f, 0.0f});
            //        camera->SetTranslate(cameraTranslate);
            //    }
            //    {
            //        //// 左スティックの値を取得
            //        float x = (float) state.Gamepad.sThumbRX;
            //        float y = (float) state.Gamepad.sThumbRY;
            //        //// 数値が大きいので正規化（-1.0 ～ 1.0）して使うのが一般的
            //        float normalizedX = x / 32767.0f;
            //        float normalizedY = y / 32767.0f;

            //        Vector3 point = LightManager::GetInstance()->GetSpotLight(0).direction;
            //        point =
            //            Add(point, Vector3 {normalizedX / 60.0f, normalizedY / 60.0f, 0.0f});
            //        LightManager::GetInstance()->SetSpotLightDirection(0, point);
            //    }
            //}


    if (isDebugCamera_)
    {
        debugCamera_.Update(camera->GetTransform());
        camera->SetTranslate(debugCamera_.GetTranslate());
        camera->SetWorldMatrix(debugCamera_.GetWorldMatrix());
    } else
    {
        camera->Update();
    }
    camera->UpdateView();
    camera->UpdateViewProjection();
    object3d->Update();
    object3d2->Update();

#ifdef USE_IMGUI

    ImGui::Begin("Camera Setting");

    // カメラの現在位置を取得
    Vector3 camPos = camera->GetTranslate();
    // カメラの現在の回転を取得（※CameraクラスにGetRotate関数が実装されている前提です）
    Vector3 camRot = camera->GetRotate();

    // 位置の調整 (0.1f単位でドラッグして変更)
    if (ImGui::DragFloat3("Camera Position", &camPos.x, 0.1f)) {
        camera->SetTranslate(camPos);
    }

    // 回転の調整
    // (0.01f単位でドラッグして変更、ラジアンか度数法に合わせて調整してください)
    if (ImGui::DragFloat3("Camera Rotation", &camRot.x, 0.01f)) {
        camera->SetRotate(camRot);
    }

    ImGui::End();

    // ImGui::Begin("Debug");
    // ImGui::Text("Sphere");
    // Vector3 pos = object3d->GetTranslate();
    // Vector3 scale = object3d->GetScale();
    // ImGui::SliderFloat3("Pos", &(pos.x), 0.1f, 1000.0f);
    // ImGui::DragFloat3("scale", &(scale.x), 0.1f, 1000.0f);
    // object3d->SetTranslate(pos);
    // object3d->SetScale(scale);
    // if (LightManager::GetInstance()->GetPointLightCount() > 0) {
    //     ImGui::Begin("Light Setting");

    //    // 0番目のポイントライトのデータを参照で取得
    //    // "auto&"
    //    にすることで、ここで書き換えた内容が直接LightManager内のデータに反映されます
    //    auto& pointLight2 = LightManager::GetInstance()->GetPointLight(1);

    //    // 位置の調整
    //    ImGui::DragFloat3("Point Light2 Pos", &pointLight2.position.x, 0.1f);

    //    // 色の調整
    //    ImGui::ColorEdit4("Point Light2 Color", &pointLight2.color.x);

    //    // 強度の調整
    //    ImGui::DragFloat("Point Light2 Intensity", &pointLight2.intensity, 0.1f,
    //    0.0f, 100.0f);

    //    // 減衰率の調整
    //    ImGui::DragFloat("Point Light2 Decay", &pointLight2.decay, 0.1f,
    //    0.0f, 10.0f); auto& pointLight1 =
    //    LightManager::GetInstance()->GetPointLight(0);

    //    // 位置の調整
    //    ImGui::DragFloat3("Point Light Pos", &pointLight1.position.x, 0.1f);

    //    // 色の調整
    //    ImGui::ColorEdit4("Point Light Color", &pointLight1.color.x);

    //    // 強度の調整
    //    ImGui::DragFloat("Point Light Intensity", &pointLight1.intensity, 0.1f,
    //    0.0f, 100.0f);

    //    // 減衰率の調整
    //    ImGui::DragFloat("Point Light Decay", &pointLight1.decay, 0.1f,
    //    0.0f, 10.0f); ImGui::DragFloat("Point Light rad", &pointLight1.radius,
    //    0.1f, 0.0f, 10.0f); auto& spotLight2 =
    //    LightManager::GetInstance()->GetSpotLight(1);

    //    // 位置の調整
    //    ImGui::DragFloat3("spot Light2 Pos", &spotLight2.position.x, 0.1f);
    //    ImGui::DragFloat3("spot Light Pos", &spotLight2.direction.x, 0.1f);

    //    // 色の調整
    //    ImGui::ColorEdit4("spot Light2 Color", &spotLight2.color.x);

    //    // 強度の調整
    //    ImGui::DragFloat("spot Light2 Intensity", &spotLight2.intensity, 0.1f,
    //    0.0f, 100.0f);

    //    // 減衰率の調整
    //    ImGui::DragFloat("spot Light2 Decay", &spotLight2.decay, 0.1f,
    //    0.0f, 10.0f); auto& spotLight1 =
    //    LightManager::GetInstance()->GetSpotLight(0);

    //    // 位置の調整
    //    ImGui::DragFloat3("spot Light Pos", &spotLight1.position.x, 0.1f);
    //    ImGui::DragFloat3("spot Light ", &spotLight1.direction.x, 0.1f);

    //    // 色の調整
    //    ImGui::ColorEdit4("spot Light Color", &spotLight1.color.x);

    //    // 強度の調整
    //    ImGui::DragFloat("spot Light Intensity", &spotLight1.intensity, 0.1f,
    //    0.0f, 100.0f);

    //    // 減衰率の調整
    //    ImGui::DragFloat("spot Light Decay", &spotLight1.decay, 0.1f,
    //    0.0f, 10.0f); ImGui::DragFloat("spot Light rad", &spotLight1.distance,
    //    0.1f, 0.0f, 10.0f);

    //    ImGui::End();
    //}

    ///*  if (ImGui::ColorEdit4("LightColor", &lightColor.x)) {

    //      object3d->SetDirectionalLightColor(lightColor);
    //  }
    //  Vector3 direction= object3d->GetDirectionalLightDirection();
    //  if(ImGui::DragFloat3("Light Direction", &direction.x)){
    //  object3d->SetDirectionalLightDirection(direction);
    //  }
    //  float intensity= object3d->GetDirectionalLightIntensity();
    //  if(ImGui::InputFloat("intensity",&intensity)){
    //   object3d->SetDirectionalLightIntensity(intensity);
    //  }*/
    // ImGui::Text("Sprite");
    // Vector2 Position =
    //    sprite->GetPosition();
    // ImGui::SliderFloat2("Position", &(Position.x), 0.1f, 1000.0f);
    // sprite->SetPosition(Position);

    // ImGui::End();

    ImGui::Begin("DebugCamera Setting");

    Vector3 newSaveRotation, newSaveTranslate;

    if (ImGui::Checkbox("DebugCamera", &isDebugCamera_))
    {
        newSaveRotation = debugSaveCameraRotation_;
        newSaveTranslate = debugSaveCameraTranslate_;

        debugSaveCameraRotation_ = camera->GetRotate();
        debugSaveCameraTranslate_ = camera->GetTranslate();

        camera->SetRotate(newSaveRotation);
        camera->SetTranslate(newSaveTranslate);
    }

    ImGui::InputFloat3("SaveRotation", &debugSaveCameraRotation_.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("SaveTranslate", &debugSaveCameraTranslate_.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::End();

    ImGui::Begin("CollisionMask");
    ImGui::Checkbox("isVisibleCollisionMask", &isVisibleCollisionMask_);
    if (player_->OnThread()) {
        // 糸の上なら 緑色 で表示
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "ON THREAD: YES");
    } else {
        // 地面なら 赤色 で表示
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "ON THREAD: NO (GROUND)");
    }
    ImGui::End();

#endif // USE_IMGUI

    // Rキーを押したらリセット
    if (Input::GetInstance()->PushedKeyDown(DIK_R))
    {
        // シーン遷移
        SceneManager::GetInstance()->ChangeScene("GameScene");
    }

    // sprite->SetRotation(sprite->GetRotation() + 0.1f);
    sprite->Update();

    // terrain_->Update();

    player_->Update();

    // 卵の更新処理
    egg_->Update();

    // 糸の更新処理
    thread_->Update();
    spiderWeb_->Update(*thread_);

    // ゴールの更新処理
    goal_->Update();


    //// 敵の目的地を決定する
    //Vector3 targetPos;
    //if (egg_->IsOnPlayer()) {
    //    // 持ち上げ中ならプレイヤーの足元の座標を使う
    //    targetPos = player_->GetPosition();
    //}
    //else {
    //    // 置いてあるなら卵自身の座標を使う
    //    targetPos = egg_->GetWorldPosition();
    //}


    //// プレイヤーが糸を撃った瞬間を検知
    //if (player_->GetAndResetDidFireThread()) {
    //    // 敵に「道が変わったぞ！」と教える
    //    enemy_->RequestPathReplan();

    //    // デバッグ用ログ
    //    OutputDebugStringA("Player fired thread! Enemy replanning path...\n");
    //}

    //// 決定した目的地を敵に渡す
    //enemy_->Update(targetPos, thread_.get());

    // 敵の目的地を決定する
    Vector3 targetPos;
    if (egg_->IsOnPlayer()) {
        targetPos = player_->GetPosition();
    } else {
        targetPos = egg_->GetWorldPosition();
    }

    // プレイヤーが糸を撃った瞬間を検知
    if (player_->GetAndResetDidFireThread()) {
        OutputDebugStringA("Player fired thread! Enemy replanning path...\n");
        // 【変更】すべての敵に経路再計算をリクエスト
        for (auto& enemy : enemies_) {
            enemy->RequestPathReplan();
        }
    }

    // 【変更】すべての敵のUpdateを呼ぶ
    for (auto& enemy : enemies_) {
        if (enemy->GetCanMove())
        {
            enemy->Update(targetPos, thread_.get());
        }
    }

    // 巣の素材の更新処理
    nestMaterial_->Update();


    collisionMask_->Update();


    // 当たり判定の確認
    CheckAllCollisions();

    // ゴールクリアの判定
    goal_->Clear();
    egg_->Death();

}

void GameScene::Draw() {

    // player_->Draw();
    // terrain_->Draw();

    // 卵の描画処理
    egg_->Draw();

    // ゴールの描画処理
    goal_->Draw();

    // 敵の描画処理
    //enemy_->Draw();

    for (auto& enemy : enemies_) {
        enemy->Draw();
    }

    // 巣の素材の描画処理
    nestMaterial_->Draw();

    // ParticleManager::GetInstance()->Draw();
    ///////スプライトの描画
    // sprite->Draw();

    player_->Draw();

    thread_->Draw();

    spiderWeb_->Draw();

    if (isVisibleCollisionMask_)
    {
        collisionMask_->Draw();
    }
}

void GameScene::CheckAllCollisions() {
    // プレイヤーと卵の判定（ここはそのまま）
    AABB playerAABB = player_->GetAABB();
    AABB eggAABB = egg_->GetAABB();

    if (isCollision(playerAABB, eggAABB)) {
        egg_->OnCollision(player_.get());
        ResolveCollision(player_.get(), playerAABB, eggAABB);
    } else {
        egg_->SetHitFlag(false);
    }

    // 【変更】すべての敵と卵の判定
    for (auto& enemy : enemies_) {
        AABB enemyAABB = enemy->GetAABB();

        if (isCollision(enemyAABB, eggAABB)) {
            enemy->OnCollision(egg_.get());
            ResolveCollision(enemy.get(), enemyAABB, eggAABB);
        } else {
            enemy->SetHitFlag(false);
        }
    }

    // 巣の素材の座標
    AABB nestAABB = nestMaterial_->GetAABB();

    if (isCollision(nestAABB, playerAABB))
    {
        //　巣の素材がなかったらリターン
        if (nestMaterial_->IsDead())
        {
            return;
        }

        // プレイヤーが素材に接触していたら
        nestMaterial_->OnCollision();
        player_->SetNestMaterial(1);
    }
}

bool GameScene::isCollision(const AABB& aabb1, const AABB& aabb2) {
    if (aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x && aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y && aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z) {
        return true;
    }
    return false;
}

void GameScene::ResolveCollision(Player* player, const AABB& playerAABB, const AABB& otherAABB) {

    // 各軸ごとのめり込み量を計算
    // min(右側のめり込み, 左側のめり込み) をとる
    float overlapX = std::min(playerAABB.max.x - otherAABB.min.x, otherAABB.max.x - playerAABB.min.x);
    float overlapZ = std::min(playerAABB.max.z - otherAABB.min.z, otherAABB.max.z - playerAABB.min.z);

    Vector3 currentPos = player->GetPosition();

    // 最もめり込みが小さい軸（最短分離軸）を特定して押し戻す
    if (overlapX < overlapZ) {
        // X軸方向の押し戻し
        if (playerAABB.min.x < otherAABB.min.x)
        {
            currentPos.x -= overlapX;
        }// 左へ
        else
        {
            currentPos.x += overlapX; // 右へ
        }
    } else
    {
        // Z軸方向の押し戻し
        if (playerAABB.min.z < otherAABB.min.z)
        {
            currentPos.z -= overlapZ; // 手前へ
        } else
        {
            currentPos.z += overlapZ; // 奥へ
        }
    }

    // 押し戻した先が壁だったら
    if (CollisionMask::GetInstance()->IsWall(currentPos.x, currentPos.z))
    {
        if (overlapX < overlapZ) {
            // X軸方向の押し戻し
            if (playerAABB.min.x < otherAABB.min.x)
            {
                currentPos.x += (overlapX * 2.0f);
            } else
            {
                currentPos.x -= (overlapX * 2.0f);
            }
        } else
        {
            // Z軸方向の押し戻し
            if (playerAABB.min.z < otherAABB.min.z)
            {
                currentPos.z += (overlapZ * 2.0f);
            } else
            {
                currentPos.z -= (overlapZ * 2.0f);
            }
        }
    }

    // 修正した座標を反映
    player->SetPosition(currentPos);
}

void GameScene::ResolveCollision(Enemy* enemy, const AABB& enemyAABB, const AABB& otherAABB) {
    // 各軸ごとのめり込み量を計算
    // min(右側のめり込み, 左側のめり込み) をとる
    float overlapX = std::min(enemyAABB.max.x - otherAABB.min.x, otherAABB.max.x - enemyAABB.min.x);
    float overlapY = std::min(enemyAABB.max.y - otherAABB.min.y, otherAABB.max.y - enemyAABB.min.y);
    float overlapZ = std::min(enemyAABB.max.z - otherAABB.min.z, otherAABB.max.z - enemyAABB.min.z);

    Vector3 currentPos = enemy->GetWorldPosition();

    // 最もめり込みが小さい軸（最短分離軸）を特定して押し戻す
    if (overlapX < overlapY && overlapX < overlapZ) {
        // X軸方向の押し戻し
        if (enemyAABB.min.x < otherAABB.min.x)
        {
            currentPos.x -= overlapX;
        }// 左へ
        else
        {
            currentPos.x += overlapX; // 右へ
        }
    } else if (overlapZ < overlapX && overlapZ < overlapY) {
        // Z軸方向の押し戻し
        if (enemyAABB.min.z < otherAABB.min.z)
        {
            currentPos.z -= overlapZ; // 手前へ
        } else
        {
            currentPos.z += overlapZ; // 奥へ
        }
    } else {
        // Y軸方向の押し戻し（床や天井）
        if (enemyAABB.min.y < otherAABB.min.y)
        {
            currentPos.y -= overlapY; // 下へ
        } else
        {
            currentPos.y += overlapY; // 上へ
        }
    }
    // 修正した座標を反映
    enemy->SetPosition(currentPos);
}
