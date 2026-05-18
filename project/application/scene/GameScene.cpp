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
#include "ParticleManager.h"

void GameScene::Initialize() {

    camera = std::make_unique<Camera>();
    camera->SetRotate({ 0.80f, 0.0f, 0.0f });
    camera->SetTranslate({ 0.0f, 30.0f, -30.0f });
    Object3dCommon::GetInstance()->SetDefaultCamera(camera.get());
    ParticleManager::GetInstance()->SetCamera(camera.get());

    //  BGMhandle_ = Audio::GetInstance()->LoadAudio("resources/fanfare.mp3");

  //    Audio::GetInstance()->PlayAudio(BGMhandle_, true);
      // LightManager::GetInstance()->AddDirectionalLight( { 1,1,1,1 }, { 0,-1,0
      // }, 1.0f); // メインライト LightManager::GetInstance()->AddDirectionalLight(
      // { 1,1,1,1 }, { 0,-1,0 }, 1.0f); // メインライト
    LightManager::GetInstance()->AddSpotLight(
        { 1.0f, 1.0f, 1.0f, 1.0f }, { 2.0f, 1.25f, 0.0f }, 4.0f,
        Normalize(Vector3{ -1.0f, -1.0f, 0.0f }), 7.0f, 2.0f,
        std::cos(std::numbers::pi_v<float> / 3.0f), 1.0f); // メインライト
    LightManager::GetInstance()->AddSpotLight(
        { 1.0f, 1.0f, 1.0f, 1.0f }, { 2.0f, 1.25f, 0.0f }, 4.0f,
        Normalize(Vector3{ -1.0f, -1.0f, 0.0f }), 7.0f, 2.0f,
        std::cos(std::numbers::pi_v<float> / 3.0f), 1.0f); // メインライト

    Vector3 point1 = { 0, 0, 0 };
    LightManager::GetInstance()->AddPointLight({ 1.0f, 1.0f, 1.0f, 1.0f }, point1,
        4.0f, 2.0f, 0.1f);
    LightManager::GetInstance()->AddPointLight({ 1.0f, 1.0f, 1.0f, 1.0f },
        { 0, 0, 0 }, 4.0f, 2.0f, 0.1f);
    TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");
    ParticleManager::ParticleEmitterFunc initializeFunc = [](const Vector3& emitterPosition, std::mt19937& randomEngine)-> ParticleManager::Particle {

        std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
        std::uniform_real_distribution<float> distTime(1.0f, 10.0f);
        ParticleManager::Particle particle;
        particle.transform.scale = { 1.0f,1.0f,1.0f };
        particle.transform.rotate = { 0.0f,0.0f,0.0f };
        Vector3 randamTranslate = { distribution(randomEngine),distribution(randomEngine) ,distribution(randomEngine) };
        particle.transform.translate = emitterPosition + randamTranslate;
        particle.velocity = { distribution(randomEngine),distribution(randomEngine),distribution(randomEngine) };

        particle.color = { distribution(randomEngine),distribution(randomEngine),distribution(randomEngine),1.0f };

        particle.lifeTime = distTime(randomEngine);
        particle.currentTime = 0.0f;
        return particle;
        };
    ParticleManager::ParticleUpdateFunc updateFunc = [](ParticleManager::Particle& particle, float deltaTime) {
        // パーティクルの更新処理
        // 例: 速度に基づいて位置を更新し、寿命を減少させる
        particle.uvTransform.offset.x += deltaTime;
        particle.transform.translate += particle.velocity * deltaTime;
        };
    ParticleManager::ParticleEmitterFunc initialize = [](const Vector3& emitterPosition, std::mt19937& randomEngine)-> ParticleManager::Particle {

        std::uniform_real_distribution<float> rotation(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
        ParticleManager::Particle particle;
         particle.transform.scale = { 2.0f,2.0f,2.0f };
        particle.transform.rotate = { 0.0f,0.0f,0.0f };
        particle.transform.translate = emitterPosition;
        particle.velocity = { 0.0f, 0.0f, 0.0f };

        particle.color = { 1.0f,1.0f,1.0f,1.0f };

        particle.lifeTime = 1.0f;
        particle.currentTime = 0.0f;
        return particle;
        };
    ParticleManager::ParticleUpdateFunc update = [](ParticleManager::Particle& particle, float deltaTime) {
        // パーティクルの更新処理
        // 例: 速度に基づいて位置を更新し、寿命を減少させる
        particle.uvTransform.offset.x += deltaTime/2;
        };
    TextureManager::GetInstance()->LoadTexture( "resources/gradationLine.png");
    ParticleManager::GetInstance()->CreateParticleGroup(
        "Test", "resources/gradationLine.png", ParticleManager::EffectType::Cylinder, initialize, update);
    /*   std::vector<Sprite*> sprites;
       for (uint32_t i = 0; i < 5; i++)
       {*/
    sprite = std::make_unique<Sprite>();
    // sprite->Initialize("resources/monsterBall.png");
    sprite->Initialize("resources/uvChecker.png");

    sprite->SetPosition(Vector2{ 25.0f + 100.0f, 100.0f });


    sprite->SetAnchorPoint(Vector2{ 0.5f, 0.5f });

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
    EulerTransform M = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    emitter = std::make_unique<ParicleEmitter>("Test", M, 1, 5.0f, 0.0f);

    /*player_ = new Player();
    player_->Initialize();

    terrain_ = new Terrain();
    terrain_->Initialize();*/

    CollisionMask::GetInstance()->Initialize();
    stageModel_ = std::make_unique<StageModel>();
    stageModel_->Initialize();
    stageModel_->Create(10.0f);

    playerPos_ = CollisionMask::GetInstance()->GetStartPos();
    eggPos = CollisionMask::GetInstance()->GetEggStartPos();
    goalPos = CollisionMask::GetInstance()->GetGoalPos();
    // JSONから座標リストのサイズを取得する
    size_t enemyCount = CollisionMask::GetInstance()->GetEnemyCount(); // サイズを返す関数を定義しておく
    size_t nestMaterialCount = CollisionMask::GetInstance()->GetNestMaterialCount(); // サイズを返す関数を定義しておく
    size_t brokenBlockCount = CollisionMask::GetInstance()->GetBrokenBlockCount(); // サイズを返す関数を定義しておく

    // vectorをJSONの数に合わせてリサイズ
    enemyPositions_.resize(enemyCount);
    nestMaterialPositions_.resize(nestMaterialCount);
    brokenBlockPos_.resize(brokenBlockCount);

    // 3. 一致したサイズ分だけループして代入
    for (int i = 0; i < enemyPositions_.size(); ++i)
    {
        enemyPositions_[i] = CollisionMask::GetInstance()->GetEnemyStartPos(i);
    }

    for (int i = 0; i < nestMaterialPositions_.size(); ++i)
    {
        nestMaterialPositions_[i] = CollisionMask::GetInstance()->GetNestMaterialPos(i);
    }

    for (int i = 0; i < brokenBlockPos_.size(); ++i)
    {
        brokenBlockPos_[i] = CollisionMask::GetInstance()->GetBrokenBlockPos(i);
    }

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
    player_->SetGameScene(this);

    // 卵の初期化
    egg_ = std::make_unique<Egg>();
    egg_->Initialize(eggPos);
    player_->SetEgg(egg_.get());
    egg_->SetPlayer(player_.get());
    egg_->SetGameScene(this);

    // ゴールの初期化
    goal_ = std::make_unique<Goal>();
    goal_->Initialize(goalPos);
    goal_->SetGameScene(this);

    goal_->SetEgg(egg_.get());
    goal_->SetPlayer(player_.get());
    goal_->SetNeedNestCount(static_cast<int>(CollisionMask::GetInstance()->GetNestMaterialCount()));
   
    Vector3 ePos=goalPos;
    ePos.y=-1.0f;

    emitter->SetTranslate(ePos);
    emitter->Emit();


    // 敵の初期化
    for (const auto& pos : enemyPositions_) {
        auto enemy = std::make_unique<Enemy>();
        enemy->Initialize(pos);
        enemy->SetGameScene(this);
        enemies_.push_back(std::move(enemy)); // リストに追加
    }


    // 巣の素材の初期化
    for (const auto& pos : nestMaterialPositions_)
    {
        auto nestMaterial = std::make_unique<NestMaterial>();
        nestMaterial->Initialize(pos);
        nestMaterial_.push_back(std::move(nestMaterial));
    }


    // ========================================================
    // ▼ ここから下（関数の最後）に以下のコードを丸ごと追加！ ▼
    // ========================================================
    stageOneWays_.clear();
    std::vector<OneWayObject*> playerOneWayPtrs;

    // CollisionMaskからJSONの登録数を取得
    size_t oneWayCount = CollisionMask::GetInstance()->GetOneWayObjectCount();

    // GameScene.cpp の LoadStageData() のループ内

    for (size_t j = 0; j < oneWayCount; ++j) {
        // JSONから座標とサイズを取得
        Vector3 pos = CollisionMask::GetInstance()->GetOneWayObjectPos(j);
        Vector3 scale = CollisionMask::GetInstance()->GetOneWayObjectScale(j);

        // ▼① JSONから向き（整数）を取得する ▼
        int32_t dirInt = CollisionMask::GetInstance()->GetOneWayObjectDir(j);

        // ▼② 整数(int32_t)を OneWayObject::Direction 型に変換(キャスト)する ▼
        OneWayObject::Direction dir = static_cast<OneWayObject::Direction>(dirInt);

        // OneWayObjectの実体を作る
        auto oneWay = std::make_unique<OneWayObject>();

        // ▼③ 固定値だった引数を、変換した dir に変更！ ▼
        oneWay->Initialize(pos, dir, scale.x, scale.z);

        // ポインタをリストにまとめる
        playerOneWayPtrs.push_back(oneWay.get());
        // 実体はシーンに保存する
        stageOneWays_.push_back(std::move(oneWay));
    }

    // まとめてプレイヤーにポインタを渡す！
    player_->SetOneWayObjects(playerOneWayPtrs);

    // 数回渡ったら壊れるオブジェクトの生成
    for (const auto& pos : brokenBlockPos_)
    {
        auto brokenBlock = std::make_unique<BrokenBlock>();
        brokenBlock->Initialize(pos, 10.0f, 9.0f);
        brokenBlocks_.push_back(std::move(brokenBlock));
    }

    // サウンド読み込み
    handle_ = Audio::GetInstance()->LoadAudio("resources/sounds/gameplay.wav");
    // サウンド再生
    Audio::GetInstance()->PlayAudio(handle_, true, 1.0f);

    // UIの初期化
    for (int i = 0; i < 10; ++i) {
        std::string path = "resources/numbers/" + std::to_string(i) + ".png";

        // 1つずつ生成する
        auto threadLimit = std::make_unique<Sprite>();
        threadLimit->Initialize(path);
        threadLimit->SetPosition(Vector2{ 260.0f,550.0f });
        threadLimitSprites_.push_back(std::move(threadLimit));

        auto threadCount = std::make_unique<Sprite>();
        threadCount->Initialize(path);
        threadCount->SetPosition(Vector2{ 60.0f,550.0f });
        threadCountSprites_.push_back(std::move(threadCount));

        auto nestLimit = std::make_unique<Sprite>();
        nestLimit->Initialize(path);
        nestLimit->SetPosition(Vector2{ 680.0f,550.0f });
        nestMaterialSprites_.push_back(std::move(nestLimit));

        auto nestCount = std::make_unique<Sprite>();
        nestCount->Initialize(path);
        nestCount->SetPosition(Vector2{ 480.0f,550.0f });
        nestCountSprites_.push_back(std::move(nestCount));
    }

    slashSprite_ = std::make_unique<Sprite>();
    slashSprite_->Initialize("resources/numbers/slash.png");
    slashNestSprite_ = std::make_unique<Sprite>();
    slashNestSprite_->Initialize("resources/numbers/slash.png");
    threadIconSprite_ = std::make_unique<Sprite>();
    threadIconSprite_->Initialize("resources/icon/thread.png");
    nestIconSprite_ = std::make_unique<Sprite>();
    nestIconSprite_->Initialize("resources/icon/nestMaterial.png");
    eggSprite_ = std::make_unique<Sprite>();
    eggSprite_->Initialize("resources/icon/egg.png");
    hpSprite_ = std::make_unique<Sprite>();
    hpSprite_->Initialize("resources/icon/hp.png");
    clearSprite_ = std::make_unique<Sprite>();
    clearSprite_->Initialize("resources/icon/clear.png");

    threadLimit_ = player_->GetThreadCount();
    threadCountSprites_[player_->GetThreadCount()]->SetPosition(Vector2{ 60.0f,550.0f });
    slashSprite_->SetPosition(Vector2{ 160.0f,550.0f });
    threadLimitSprites_[threadLimit_]->SetPosition(Vector2{ 260.0f,550.0f });
    nestCountSprites_[player_->GetNestMaterial()]->SetPosition(Vector2{ 480.0f,550.0f });
    slashNestSprite_->SetPosition(Vector2{ 580.0f,550.0f });
    nestMaterialSprites_[goal_->GetNeedNestCount()]->SetPosition(Vector2{ 680.0f,550.0f });
    threadIconSprite_->SetPosition(Vector2{ -10.0f,510.0f });
    nestIconSprite_->SetPosition(Vector2{ 380.0f,550.0f });
    eggSprite_->SetPosition(Vector2{ 800.0f,550.0f });
    hpSprite_->SetPosition(Vector2{ 960.0f,600.0f });
    clearSprite_->SetPosition(Vector2{ 00.0f,100.0f });

    // メニューUIの初期化
    for (int i = 0; i < 3; i++)
    {
        std::string path = "resources/Menu/" + std::to_string(i) + ".png";
        auto pauseSprite = std::make_unique<Sprite>();
        pauseSprite->Initialize(path);
        pauseSprite->SetPosition(Vector2{ 450.0f,(30.0f + (430.0f * i))});
        pauseSprite_.push_back(std::move(pauseSprite));
    }


    menuSprite_ = std::make_unique<Sprite>();
    menuSprite_->Initialize("resources/Menu/backGround.png");
    menuSprite_->SetPosition(Vector2{ 0.0f,0.0f });
    cursorSprite_ = std::make_unique<Sprite>();
    cursorSprite_->Initialize("resources/Menu/cursor.png");
    cursorSprite_->SetPosition(pauseSprite_[0]->GetPosition());

    fade_ = std::make_unique<Fade>();
    fade_->Initialize();
    fade_->StartFadeIn(0.05f); // シーン生成時にフェードインを開始

    // 背景の初期化
    ModelManager::GetInstance()->LoadModel("resources", "backGround.obj");
    backgroundModel_ = std::make_unique<Object3d>();
    backgroundModel_->Initialize();
    backgroundModel_->SetModel("backGround.obj");
    backgroundModel_->SetTranslate(Vector3{ 0.0f,-4.0f,0.0f });
    backgroundModel_->SetScale(Vector3{ 30.0f,30.0f,30.0f });


}
void GameScene::Finalize() {

    stageModel_->Finalize();

    LightManager::GetInstance()->ClearLights();

    ParticleManager::GetInstance()->ReleaseParticleGroup("Test");

    //CollisionMask::GetInstance()->Finalize();

    Audio::GetInstance()->StopAudio(handle_);

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

void GameScene::Update()
{
   /* emitter->SetTranslate(player_->GetPosition());
    emitter->Update();*/

    XINPUT_STATE state;

    // 現在のジョイスティックを取得

    Input::GetInstance()->GetJoyStick(0, state);

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

    stageModel_->Update();

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

    // =========================================================
    // ★ 追加: Player と BrokenBlock の当たり判定デバッグウィンドウ
    // =========================================================
    ImGui::Begin("Collision Debug");

    // 1. プレイヤーの情報
    Vector3 pPos = player_->GetPosition();
    AABB pAABB = player_->GetAABB();
    float pWidth = pAABB.max.x - pAABB.min.x;
    float pDepth = pAABB.max.z - pAABB.min.z;

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "--- Player Info ---");
    ImGui::Text("Position: (%.2f, %.2f, %.2f)", pPos.x, pPos.y, pPos.z);
    ImGui::Text("Size(W,D): %.2f, %.2f", pWidth, pDepth);
    ImGui::Text("AABB Min: (%.2f, %.2f, %.2f)", pAABB.min.x, pAABB.min.y, pAABB.min.z);
    ImGui::Text("AABB Max: (%.2f, %.2f, %.2f)", pAABB.max.x, pAABB.max.y, pAABB.max.z);
    ImGui::Separator();

    // 2. BrokenBlock の情報
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "--- BrokenBlocks Info ---");
    if (brokenBlocks_.empty()) {
        ImGui::Text("No BrokenBlocks.");
    }
    for (size_t i = 0; i < brokenBlocks_.size(); ++i) {
        auto& block = brokenBlocks_[i];
        if (block->IsBroken()) {
            ImGui::Text("Block[%zu] : Broken!", i);
            ImGui::Separator();
            continue;
        }

        Vector3 bPos = block->GetPosition();
        AABB bAABB = block->GetAABB();
        float bWidth = bAABB.max.x - bAABB.min.x;
        float bDepth = bAABB.max.z - bAABB.min.z;

        // 判定結果の取得
        bool isInside = block->IsInside(pPos);
        bool isRider = block->IsRider(player_.get());
        bool isImpassable = block->IsImpassable();

        ImGui::Text("Block[%zu]", i);
        ImGui::Text("  Position: (%.2f, %.2f, %.2f)", bPos.x, bPos.y, bPos.z);
        ImGui::Text("  Size(W,D): %.2f, %.2f", bWidth, bDepth);

        // IsInside (プレイヤーの中心座標がブロック内にあるか)
        if (isInside) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "  IsInside: True (Player center is in block)");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "  IsInside: False");
        }

        // IsRider (ブロックがプレイヤーの乗降をどう認識しているか)
        if (isRider) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "  IsRider : True (Player is riding)");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "  IsRider : False");
        }

        // isImpassable (次に降りたら壊れる状態かどうか)
        ImGui::Text("  IsImpassable: %s", isImpassable ? "True" : "False");

        ImGui::Separator();
    }

    ImGui::End();
    // =========================================================

#endif // USE_IMGUI

    if (egg_->IsDead())
    {
        // 卵の更新処理
        egg_->Update();

        return;
    }

    if (openPause_)
    {
        Pause();
        return;
    } else
    {
        if (Input::GetInstance()->TriggerKeyDown(DIK_Q))
        {
            openPause_ = true;
            return;
        }
    }

    // クリアフラグが立っている場合
    if (isClear_)
    {
        Clear();
    }

    CollisionMask::GetInstance()->Update();

    sprite->Update();

    // 卵の更新処理
    egg_->Update();

    // 一方通行オブジェクトの更新
    for (auto& ow : stageOneWays_) {
        ow->Update();
    }

    // =========================================================
    // ★ 追加: プレイヤーに現在の BrokenBlock リストを渡す
    // =========================================================
    std::vector<BrokenBlock*> blockPtrs;
    for (auto& b : brokenBlocks_) {
        blockPtrs.push_back(b.get());
    }
    player_->SetBrokenBlocks(blockPtrs);

    player_->Update();

    // ゴールの更新処理
    goal_->Update();

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

    // 1. すでに捕まっている敵のキーを収集
    std::vector<uint64_t> occupiedKeys;
    for (auto& enemy : enemies_) {
        if (enemy->IsTrapped()) {
            occupiedKeys.push_back(enemy->GetTrappedWebKey());
        }
    }

    //// 2. 敵の更新（リストを渡す）
    //for (auto& enemy : enemies_) {
    //    // 第5引数に occupiedKeys を渡す
    //    enemy->Update(playerPos_, thread_.get(), oneWayObjects_, brokenBlocks_, occupiedKeys);
    //}

    // 【変更】すべての敵のUpdateを呼ぶ
    for (auto& enemy : enemies_) {
        /*if (enemy->GetCanMove())
        {
            enemy->Update(targetPos, thread_.get(), stageOneWays_, brokenBlocks_, occupiedKeys);
        }*/

        enemy->Update(targetPos, thread_.get(), stageOneWays_, brokenBlocks_, occupiedKeys);
    }

    // 糸の更新処理
    thread_->Update();
    spiderWeb_->Update(*thread_);

    player_->UpdateHeight();
    for (auto& enemy : enemies_) {
        enemy->UpdateHeight(thread_.get());
    }

    // 巣の素材の更新処理
    for (auto& nestMaterial : nestMaterial_)
    {
        if (!nestMaterial->IsDead())
        {
            nestMaterial->Update();
        }
    }

    // 壊れるブロックの更新処理
    for (auto& brokenBlock : brokenBlocks_)
    {
        brokenBlock->Update();
    }

    // UIの更新
    threadCountSprites_[player_->GetThreadCount()]->Update();
    slashSprite_->Update();
    threadLimitSprites_[threadLimit_]->Update();
    nestCountSprites_[player_->GetNestMaterial()]->Update();
    slashNestSprite_->Update();
    nestMaterialSprites_[goal_->GetNeedNestCount()]->Update();
    backgroundModel_->Update();
    threadIconSprite_->Update();
    nestIconSprite_->Update();
    eggSprite_->Update();
    hpSprite_->SetSize(Vector2{ 30.0f * egg_->GetHP(), 100.0f });
    hpSprite_->Update();


    if (!isClear_)
    {
        // 当たり判定の確認
        CheckAllCollisions();

        // ゴールクリアの判定
        goal_->Clear();
        egg_->Death();
    }

    // 破壊フラグの立ったブロックを削除
    brokenBlocks_.erase(
        std::remove_if(brokenBlocks_.begin(), brokenBlocks_.end(),
            [](const std::unique_ptr<BrokenBlock>& block) {
                return block->IsBroken();
            }),
        brokenBlocks_.end()
    );

    // ① Rキーを押したらフェードアウト開始
    if (!isClear_ && !isResetWaiting_)
    {
        if (Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_START) ||
            Input::GetInstance()->PushedKeyDown(DIK_R))
        {
            if (fade_->GetStatus() == Fade::Status::None && !isResetWaiting_)
            {
                fade_->StartFadeOut(0.02f);
                isResetWaiting_ = true; // リセット待機状態にする
            }
        }
    }

    // ② フェードアウトが完了（画面が真っ黒）になった瞬間の処理
    if (isResetWaiting_ && fade_->IsFinished())
    {
        // 1. オブジェクトの位置を初期状態に戻す（既存の関数を呼び出す）
        LoadStageData();

        // プレイヤーデータをリセット
        // ファイルを読み込む
        JSONManager::GetInstance()->LoadFile("Player");

        int32_t remaining;
        if (JSONManager::GetInstance()->TryGetInt("Player", "remainingThreadCount", remaining)) {
            player_->SetMaxThreadCount(remaining);
        }

        // 巣の素材データをリセット
        for (auto& nestMaterial : nestMaterial_)
        {
            nestMaterial->SetDead(false); // デスフラグをリセット
        }

        player_->ResetNestMaterial();

        // ※補足：位置以外の変数（体力、発射回数、スコアなど）をリセットする必要があればここに書きます
        // threadLimit_ = 0; 
        // player_->ResetHP(); // 例

        // 2. リセットが完了したら、フェードインを開始する
        fade_->StartFadeIn(0.02f);

        // 3. リセット待機フラグを解除して、通常のゲーム状態に戻す
        isResetWaiting_ = false;
    }

    // フェードの更新 (必ずUpdateの最後の方で呼ぶ)
    fade_->Update();

}

void GameScene::Draw() {

    backgroundModel_->Draw();

    if (isReset_)
    {
        return;
    }

    // player_->Draw();
    // terrain_->Draw();

    stageModel_->Draw();

    // 卵の描画処理
    egg_->Draw();

    // ゴールの描画処理
    goal_->Draw();

    // 敵の描画処理
    for (auto& enemy : enemies_) {
        enemy->Draw();
    }

    // 巣の素材の描画処理
    for (auto& nestMaterial : nestMaterial_)
    {
        if (!nestMaterial->IsDead())
        {
            nestMaterial->Draw();
        }
    }

    // 一方通行オブジェクトの描画
    for (auto& ow : stageOneWays_) {
        ow->Draw();
    }

    // 壊れるブロックの更新処理
    for (auto& brokenBlock : brokenBlocks_)
    {
        brokenBlock->Draw();
    }

    ParticleManager::GetInstance()->Draw();
    ///////スプライトの描画
    // sprite->Draw();

    player_->Draw();

    thread_->Draw();

    spiderWeb_->Draw();

    if (isVisibleCollisionMask_)
    {
        CollisionMask::GetInstance()->Draw();
    }

    if (openPause_)
    {
        menuSprite_->Draw();

        for (auto& pauseSprite : pauseSprite_)
        {
            pauseSprite->Draw();
        }

        cursorSprite_->Draw();
    } else  if (isClear_)
    {
        if (t_ >= 1.0f)
        {

            for (int i = 1; i < 3; i++)
            {
                pauseSprite_[i]->Draw();
            }

            cursorSprite_->Draw();
            clearSprite_->Draw();
        }
    } else
    {
        threadCountSprites_[player_->GetThreadCount()]->Draw();
        slashSprite_->Draw();
        threadLimitSprites_[threadLimit_]->Draw();
        nestCountSprites_[player_->GetNestMaterial()]->Draw();
        slashNestSprite_->Draw();
        nestMaterialSprites_[goal_->GetNeedNestCount()]->Draw();
        threadIconSprite_->Draw();
        nestIconSprite_->Draw();
        eggSprite_->Draw();

        if (egg_->IsDead())
        {
            hpSprite_->Draw();
        }
    }


    fade_->Draw();
}

void GameScene::CheckAllCollisions() {
    // プレイヤーと卵の判定（ここはそのまま）
    AABB playerAABB = player_->GetAABB();
    AABB eggAABB = egg_->GetAABB();

    if (isCollision(playerAABB, eggAABB)) {
        if(!egg_->IsOnPlayer())
        {
            egg_->OnCollision(player_.get());
            ResolveCollision(player_.get(), playerAABB, eggAABB);
        }
    }
    else {
        egg_->SetHitFlag(false);
    }

    // 【変更】すべての敵と卵の判定
    for (auto& enemy : enemies_) {

        if (enemy->IsTrapped()) {
            continue;
        }

        AABB enemyAABB = enemy->GetAABB();

        if (isCollision(enemyAABB, eggAABB)) {
            enemy->OnCollision(egg_.get());

            if(!egg_->IsOnPlayer())
            {
                ResolveCollision(enemy.get(), enemyAABB, eggAABB);
            }
        }
        else {
            enemy->SetHitFlag(false);
        }

    }

    // 巣の素材の座標
    for (auto& nestMaterial : nestMaterial_)
    {
        if (!nestMaterial->IsDead())
        {
            AABB nestAABB = nestMaterial->GetAABB();

            if (isCollision(nestAABB, playerAABB))
            {
                // プレイヤーが素材に接触していたら
                nestMaterial->OnCollision();
                player_->SetNestMaterial(1);
            }
        }
    }

    // =========================================================
    // ★ 追加: BrokenBlock とプレイヤーの乗降判定
    // =========================================================
    for (auto& brokenBlock : brokenBlocks_) {
        // プレイヤーがブロックに乗っているかを毎フレームチェック
        brokenBlock->CheckRiding(player_->GetPosition(), player_.get());
    }

}

bool GameScene::isCollision(const AABB& aabb1, const AABB& aabb2)
{
    if (aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x && aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y && aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z) {
        return true;
    }
    return false;
}

bool GameScene::isCollisionXZ(const AABB& aabb1, const AABB& aabb2)
{
    // Y軸 (min.y / max.y) の条件を削除し、XとZのみで重なりを判定する
    if (aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x &&
        aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z) {
        return true;
    }
    return false;
}

void GameScene::ResolveCollision(Player* player, const AABB& playerAABB, const AABB& otherAABB)
{

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

void GameScene::ResolveCollision(Enemy* enemy, const AABB& enemyAABB, const AABB& otherAABB)
{
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

void GameScene::Clear()
{
    // コントローラー入力を取得
    XINPUT_STATE joyState{};
    bool stickRightTrigger = false;
    bool stickLeftTrigger = false;

    if (Input::GetInstance()->GetJoyStick(0, joyState)) {
        float stickX = (float)joyState.Gamepad.sThumbLX / kStickMax;

        if (std::abs(stickX) > kDeadZone) {

            // 右に倒した瞬間
            if (stickX > 0.5f) {
                if (!isStickPushed) {
                    stickRightTrigger = true; // 倒した瞬間だけオン
                    isStickPushed = true;
                }
            }
            // 左に倒した瞬間
            else if (stickX < -0.5f) {
                if (!isStickPushed) {
                    stickLeftTrigger = true; // 倒した瞬間だけオン
                    isStickPushed = true;
                }
            }
            // スティックが中央に戻ったらリセット
            else {
                isStickPushed = false;
            }
        }
    }

    if (t_ < 1.0f)
    {
        t_ += 0.01f; // tを徐々に増加させる
        // カメラをプレイヤーの前へ
        Vector3 cameraPos = camera->GetTranslate();
        Vector3 newPos = Vector3Lerp(cameraPos, player_->GetPosition() + cameraOffset_, t_);
        Vector3 cameraRotate = camera->GetRotate();
        Vector3 newRotate = Vector3Lerp(player_->GetForward(), Vector3{ 0.0f,3.0f,0.0f }, t_);
        camera->SetTranslate(newPos);
        player_->SetForward(newRotate);

        // メニューUIの初期化
        for (int i = 1; i < 3; i++)
        {
            pauseSprite_[i]->SetPosition(Vector2{ (20.0f + (500.0f * (i - 1))), 500.0f });
        }
        Vector2 pos = pauseSprite_[1]->GetPosition();
        pos.y += 200.0f;
        pos.x -= 400.0f;
        cursorSprite_->SetPosition(pos);
        pauseIndex_ = 1;
    } else
    {
        if (isFadeStart_)
        {
            // フェードの更新
            fade_->Update();

            if (fade_->IsFinished())
            {
                if (pauseIndex_ == 1)
                {
                    SceneManager::GetInstance()->ChangeScene("SelectScene");
                }
                else if (pauseIndex_ == 2)
                {
                    // ステージナンバーを設定
                    int num = CollisionMask::GetInstance()->GetCurrentStageID();

                    int maxNum = static_cast<int>(CollisionMask::GetInstance()->GetMaxStageID());

                    if (num == maxNum)
                    {
                        CollisionMask::GetInstance()->SetCurrentStageID(0);
                    } else
                    {
                        CollisionMask::GetInstance()->SetCurrentStageID(num + 1);
                    }

                    SceneManager::GetInstance()->ChangeScene("GameScene");
                }
            }
        } else
        {
            if (Input::GetInstance()->TriggerKeyDown(DIK_RIGHTARROW) || stickRightTrigger ||
                Input::GetInstance()->TriggerKeyDown(DIK_D) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_RIGHT))
            {
                if (pauseIndex_ < 2)
                {
                    pauseIndex_++;
                } else
                {
                    pauseIndex_ = 1;
                }
            } else if (Input::GetInstance()->TriggerKeyDown(DIK_LEFTARROW) || stickLeftTrigger ||
                Input::GetInstance()->TriggerKeyDown(DIK_A) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_LEFT))
            {
                if (pauseIndex_ > 1)
                {
                    pauseIndex_--;
                } else
                {
                    pauseIndex_ = 2;
                }
            } else if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_A))
            {
                fade_->StartFadeOut(0.02f);
                isFadeStart_ = true;
            }

        }

        Vector2 pos = pauseSprite_[pauseIndex_]->GetPosition();
        pos.y -= 500.0f;
        pos.x -= 0.0f;
        cursorSprite_->SetPosition(pos);

        for (auto& pauseSprite : pauseSprite_)
        {
            pauseSprite->Update();
        }
        menuSprite_->Update();
        cursorSprite_->Update();
        clearSprite_->Update();
        
    }

}

void GameScene::Pause()
{
    if (isFadeStart_)
    {
        // フェードの更新
        fade_->Update();

        if (fade_->IsFinished())
        {
            if (pauseIndex_ == 1)
            {
                SceneManager::GetInstance()->ChangeScene("SelectScene");
            }
        }
    } else
    {
        // コントローラー入力を取得
        XINPUT_STATE joyState{};
        bool stickUpTrigger = false;
        bool stickDownTrigger = false;

        if (Input::GetInstance()->GetJoyStick(0, joyState)) {
            float stickX = (float)joyState.Gamepad.sThumbLY / kStickMax;

            if (std::abs(stickX) > kDeadZone) {

                // 右に倒した瞬間
                if (stickX > 0.5f) {
                    if (!isStickPushed) {
                        stickUpTrigger = true; // 倒した瞬間だけオン
                        isStickPushed = true;
                    }
                }
                // 左に倒した瞬間
                else if (stickX < -0.5f) {
                    if (!isStickPushed) {
                        stickDownTrigger = true; // 倒した瞬間だけオン
                        isStickPushed = true;
                    }
                }
                // スティックが中央に戻ったらリセット
                else {
                    isStickPushed = false;
                }
            }
        }

        if (Input::GetInstance()->TriggerKeyDown(DIK_DOWNARROW) || stickDownTrigger ||
            Input::GetInstance()->TriggerKeyDown(DIK_S) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_DOWN))
        {
            if (pauseIndex_ < 1)
            {
                pauseIndex_++;
            } else
            {
                pauseIndex_ = 0;
            }
        } else if (Input::GetInstance()->TriggerKeyDown(DIK_UPARROW) || stickUpTrigger ||
            Input::GetInstance()->TriggerKeyDown(DIK_W) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_UP))
        {
            if (pauseIndex_ > 0)
            {
                pauseIndex_--;
            } else
            {
                pauseIndex_ = 1;
            }
        } else if (Input::GetInstance()->TriggerKeyDown(DIK_Q) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_START))
        {
            openPause_ = false;
            return;
        } else if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE) || Input::GetInstance()->TriggerPadDown(0, XINPUT_GAMEPAD_A))
        {
            if (pauseIndex_ == 0)
            {
                openPause_ = false;
                return;
            } else
            {
                fade_->StartFadeOut(0.02f);
                isFadeStart_ = true;
            }

        }
    }


    Vector2 pos = pauseSprite_[pauseIndex_]->GetPosition();
    pos.y -= 500.0f;
    pos.x -= 200.0f;
    cursorSprite_->SetPosition(pos);

    for (auto& pauseSprite : pauseSprite_)
    {
        pauseSprite->Update();
    }
    menuSprite_->Update();
    cursorSprite_->Update();
}

void GameScene::LoadStageData()
{
    player_->SetPosition(CollisionMask::GetInstance()->GetStartPos());
    egg_->SetTranslate(CollisionMask::GetInstance()->GetEggStartPos());
    goal_->SetTranslate(CollisionMask::GetInstance()->GetGoalPos());
    size_t i = 0;
    for (auto itEnemy = enemies_.begin(); itEnemy != enemies_.end(); ++itEnemy)
    {
        (*itEnemy)->Reset(CollisionMask::GetInstance()->GetEnemyStartPos(i));
        ++i;
    }
    /* i = 0;
     for (auto itNestMaterial = nestMaterials_.begin(); itNestMaterial != nestMaterials_.end(); ++itNestMaterial)
     {
         (*itNestMaterial)->SetTranslate(collisionMask_->GetNestMaterialPos(i));
         ++i;
     }*/

    for (auto itNestMaterial = nestMaterial_.begin(); itNestMaterial != nestMaterial_.end(); ++itNestMaterial)
    {
        (*itNestMaterial)->SetTranslate(CollisionMask::GetInstance()->GetNestMaterialPos(i));
        ++i;
    }

    i = 0;
    for (auto itOnWayObject = stageOneWays_.begin(); itOnWayObject != stageOneWays_.end(); ++itOnWayObject)
    {
        (*itOnWayObject)->SetTranslate(CollisionMask::GetInstance()->GetOneWayObjectPos(i));
        ++i;
    }

    std::vector<Vector3> oneWayObjectPos_;
    thread_->ClearThreads();
}
