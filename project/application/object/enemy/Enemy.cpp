#include "Enemy.h"
#include "ModelManager.h"
#include "CollisionMask.h"
#include "ThreadManager.h"
#include "Egg.h"
#include "GameScene.h"
#include "Object3dCommon.h"
#include "ImGuiManager.h"

// キー生成関数 (SpiderWebManager.cpp と同じアルゴリズム)
static uint64_t GenerateWebKey(const ThreadManager::ThreadIntersection& intersect) {
    uint64_t key = 1469598103934665603ULL;
    auto mix = [&key](uint64_t v) {
        key ^= v;
        key *= 1099511628211ULL;
        };
    mix(static_cast<uint64_t>(intersect.threadIndexA));
    mix(static_cast<uint64_t>(intersect.threadIndexB));
    mix(static_cast<uint64_t>(intersect.segmentIndexA));
    mix(static_cast<uint64_t>(intersect.segmentIndexB));
    return key;
}

void Enemy::Initialize(const Vector3& pos) {
    object_ = std::make_unique<Object3d>();
    object_->Initialize();
    ModelManager::GetInstance()->LoadModel("resources", "enemy/enemy.obj");
    object_->AddModel("enemy/enemy.obj", "enemy"); // 敵のモデル
    object_->SetTranslate(pos);
    attack_ = Audio::GetInstance()->LoadAudio("resources/sounds/damage.wav");

    anima_ = std::make_unique<EnemyAnima>();
    anima_->Initialize(object_.get());
    anima_->Play();
    anima_->ChangeAnimation(EnemyAnima::AnimationState::Idle);

    // 蜘蛛の巣のエフェクト初期化
    webEffect_ = std::make_unique<EnemyWebEffect>();
    webEffect_->Initialize();
}

void Enemy::RecalculatePath(const Vector3& eggPos, ThreadManager* tm,
    const std::vector<std::unique_ptr<OneWayObject>>& oneWays, const std::vector < std::unique_ptr <BrokenBlock>>& brokenBlock) {
    // 1. 生のワールド座標を取得
    Vector3 myPos = object_->GetTranslate();
    Vector3 targetPos = eggPos;

    // 2. A*探索用のグリッド座標を計算 (これは内部の探索用)
    Point start = WorldToGrid(myPos);
    Point goal = WorldToGrid(targetPos);

    // 【重要】デバッグログ：生のワールド座標で判定してみる
    bool startIsWall = CollisionMask::GetInstance()->IsWall(myPos.x, myPos.z);
    bool goalIsWall = CollisionMask::GetInstance()->IsWall(targetPos.x, targetPos.z);

    char buf[256];
    sprintf_s(buf, "StartWorld(%.1f, %.1f) Wall: %s | GoalWorld(%.1f, %.1f) Wall: %s\n",
        myPos.x, myPos.z, startIsWall ? "YES" : "NO",
        targetPos.x, targetPos.z, goalIsWall ? "YES" : "NO");
    OutputDebugStringA(buf);

    // 3. 壁判定ラムダ式の修正
    auto IsInsideWall = [&](Point p) {
        if (p.x < 0 || p.x >= 512 || p.y < 0 || p.y >= 512) return true;

        // グリッド座標 p を 一度ワールド座標に戻してから IsWall に投げる
        Vector3 worldP = GridToWorld(p);
        bool isWall = CollisionMask::GetInstance()->IsWall(worldP.x, worldP.z);

        // ブロックの判定
        bool hasBlock = false;
        for (const auto& br : brokenBlock) {
            if (br->IsInside(worldP) && !br->IsBroken()) {
                hasBlock = true;
                break;
            }
        }

        bool hasThread = false;
        if (tm) {
            for (auto& physics : tm->GetPhysicsList()) {
                for (const auto& node : physics->GetNodes()) {
                    float dx = node.currentPos.x - worldP.x;
                    float dz = node.currentPos.z - worldP.z;
                    // 糸の判定（少し広めに 1.5マス分 = 2.25f）
                    if ((dx * dx + dz * dz) < 0.64f) {
                        hasThread = true; break;
                    }
                }
                if (hasThread) break;
            }
        }
        return isWall && !hasThread && !hasBlock;
        };

    // --- 【強化】スタートとゴールを確実に「道」へ引きずり出す ---
    auto RescuePoint = [&](Point& p) {
        if (!IsInsideWall(p)) return; // 最初から道なら何もしない

        // 渦巻き状、または同心円状に探索範囲を広げる（最大5マス先まで）
        for (int distance = 1; distance <= 5; ++distance) {
            for (int dx = -distance; dx <= distance; ++dx) {
                for (int dy = -distance; dy <= distance; ++dy) {
                    if (std::abs(dx) != distance && std::abs(dy) != distance) continue;
                    Point test = { p.x + dx, p.y + dy };
                    if (!IsInsideWall(test)) {
                        p = test; // 道を見つけたら更新
                        return;
                    }
                }
            }
        }
        };

    RescuePoint(start);
    RescuePoint(goal);

    // パスを再検索
    std::vector<Point> newPath = PathFinder::FindPath(start, goal, 512, 512, tm, oneWays, brokenBlock);

    path_.clear();
    if (newPath.empty()) {
        OutputDebugStringA("Path not found!\n");
    } else {
        for (const auto& p : newPath) path_.push_back(p);
        if (!path_.empty()) path_.pop_front();
        char buf[64];
        sprintf_s(buf, "Path found: size = %zu\n", path_.size());
        OutputDebugStringA(buf);
    }

}

//

void Enemy::Update(const Vector3& eggPos, ThreadManager* tm,
    const std::vector<std::unique_ptr<OneWayObject>>& oneWays,
    const std::vector<std::unique_ptr<BrokenBlock>>& brokenBlock,
    std::vector<uint64_t>& occupiedWebKeys) {

#ifdef USE_IMGUI
    DrawImGui();
#endif

    Vector3 currentPos = object_->GetTranslate();

    if (tm && !gameScene_->IsClear() && !isOnBridge_ && !isTrapped_) {
        // 1. 現在地が「壁や穴」の判定（＝糸がないと落ちる場所）かチェック
        bool isOverPit = CollisionMask::GetInstance()->IsWall(currentPos.x, currentPos.z);

        // 2. 壊れるブロックの上なら「穴ではない」とみなす
        if (isOverPit) {
            for (const auto& br : brokenBlock) {
                if (br->IsInside(currentPos) && !br->IsBroken()) {
                    isOverPit = false;
                    break;
                }
            }
        }

        // 3. 足元が本当に穴の場合のみ、糸へのスナップ（吸着）を有効にする
        if (isOverPit) {
            ThreadManager::ThreadQueryResult query;
            if (tm->FindNearestThread(currentPos, 0.5f, query)) {
                // 糸の先端（t=0 や t=1）に無理やり引っ張られるのを防ぐため、
                // 糸の中間部分にいる時だけX, Z座標を糸の上に補正する
                if (query.t > 0.01f && query.t < 0.99f) {
                    currentPos.x += (query.closestPoint.x - currentPos.x) * 0.15f;
                    currentPos.z += (query.closestPoint.z - currentPos.z) * 0.15f;
                }
            }
        }
    }

    Vector3 expectedPos = currentPos;
    bool isMoving = false;

    if (webEffect_) {
        webEffect_->Update(object_->GetTranslate(), Object3dCommon::GetInstance()->GetDefaultCamera());
    }

    // 卵に当たっている、または既に巣に捕まっている場合は移動計算をスキップ
    if (!isHit_ && !isTrapped_ && !gameScene_->IsClear())
    {

        // ==========================================
        // 1. 移動方向と「予定座標(expectedPos)」の計算
        // ==========================================


        // 現在位置が橋（OneWayObject）の上かどうかを判定
        bool currentlyOnBridge = false;
        for (const auto& ow : oneWays) {
            if (ow->IsInside(currentPos)) {
                currentlyOnBridge = true;
                break;
            }
        }

        // 橋に乗った瞬間の判定（フラグの切り替わり）
        if (currentlyOnBridge && !isOnBridge_) {
            isOnBridge_ = true;
            // 橋に乗った瞬間、一度だけ「橋の先」を見据えた最新のパスを計算する
            RecalculatePath(eggPos, tm, oneWays, brokenBlock);
            recalculateTimer_ = 0;
        }
        // 橋から降りた瞬間の判定
        else if (!currentlyOnBridge && isOnBridge_) {
            isOnBridge_ = false;
            // 橋を降りたので、改めて状況（糸の有無など）を確認して再計算
            shouldReplanNextUpdate_ = true;
        }

        recalculateTimer_++;

        // 再計算の実行条件
        bool timeToReplan = (recalculateTimer_ > 30);

        // 【重要】橋の上にいる間は、時間が経っても再計算を「させない」
        // ただし、外部からの強制リプランニング(shouldReplanNextUpdate_)がある場合は例外
        if (!isOnBridge_) {
            if (timeToReplan || shouldReplanNextUpdate_) {
                RecalculatePath(eggPos, tm, oneWays, brokenBlock);
                recalculateTimer_ = 0;
                shouldReplanNextUpdate_ = false;
            }
        }
        else {
            // 橋の上で詰まった時のための保険：パスが空になったら再計算
            if (path_.empty() || shouldReplanNextUpdate_) {
                RecalculatePath(eggPos, tm, oneWays, brokenBlock);
                shouldReplanNextUpdate_ = false;
            }
        }

        if (!path_.empty()) {
            // 1. 本来目指すマスの中心
            Vector3 targetGridPos = GridToWorld(path_.front());

            // 2. 【修正】そのマスの「中」にある実際の糸の座標を探す
            Vector3 actualThreadTarget = targetGridPos; // 見つからない時のバックアップ
            float closestDist = 2.5f; // 探す範囲（1.5〜2.0くらいが適当）

            if (tm) {
                for (auto& physics : tm->GetPhysicsList()) {
                    for (const auto& node : physics->GetNodes()) {
                        // そのマス（targetGridPos）の近くにノードがあるか？
                        float dx = node.currentPos.x - targetGridPos.x;
                        float dz = node.currentPos.z - targetGridPos.z;
                        float dSq = dx * dx + dz * dz;

                        if (dSq < closestDist) {
                            closestDist = dSq;
                            actualThreadTarget = node.currentPos;
                        }
                    }
                }
            }
            actualThreadTarget.y = 0.0f;

            // 3. GridToWorld の座標ではなく、見つけた糸の座標(actualThreadTarget)へ向かう
            Vector3 nextTarget = actualThreadTarget;

            Vector3 diff = { nextTarget.x - currentPos.x, 0.0f, nextTarget.z - currentPos.z };
            float distance = std::sqrt(diff.x * diff.x + diff.z * diff.z);

            if (distance < 0.2f) {
                path_.pop_front(); // 目標地点に到達したらリストから消す
            }
            else {
                expectedPos.x += (diff.x / distance) * moveSpeed_;
                expectedPos.z += (diff.z / distance) * moveSpeed_;
                isMoving = true;
                // 2. 【追加】移動方向を向く
                float angle = std::atan2(diff.x, diff.z);
                object_->SetRotate({ 0.0f, angle, 0.0f });
            }
        }


        // ==========================================
        // 2. 交差点（トラップ）での捕縛判定
        // ==========================================
        if (tm && isMoving) {
            for (const auto& intersect : tm->GetIntersections()) {

                // ① この交差点のキー（ID）を生成
                uint64_t key = GenerateWebKey(intersect);

                // ② すでに他の敵がこのWeb(交差点)を占有していたら無視して次の交差点へ
                bool alreadyOccupied = false;
                for (uint64_t occupied : occupiedWebKeys) {
                    if (key == occupied) {
                        alreadyOccupied = true;
                        break;
                    }
                }
                if (alreadyOccupied) continue;

                // ③ 距離判定 (XZ平面)
                Vector3 diff = { currentPos.x - intersect.position.x, 0.0f, currentPos.z - intersect.position.z };
                float distSq = diff.x * diff.x + diff.z * diff.z;

                // 交差点の半径内に入ったかチェック
                float hitRadius = intersect.radius + 0.3f;

                if (distSq <= hitRadius * hitRadius) {
                    // 捕獲成功！
                    isTrapped_ = true;      // 蜘蛛の巣フラグをON
                    if (webEffect_) 
                    {
                        webEffect_->Start();
                    }
                    canMove_ = false;       // 移動不可にする
                    trappedWebKey_ = key;   // どの巣に捕まったか記録

                    // 占有リストに追加（このフレームの後の敵が引っかからなくなる）
                    occupiedWebKeys.push_back(key);

                    // 移動をキャンセルしてその場に留める
                    expectedPos = currentPos;
                    break;
                }
            }
        }

        // 壊れるブロックの判定
        for (auto& brokenBlock : brokenBlock)
        {
            brokenBlock->CheckRiding(currentPos, this);
        }
        if (isTrapped_)
        {
            // トラップにかかったら強制的にピタッと止まる状態（Default）にする
            anima_->ChangeAnimation(EnemyAnima::AnimationState::Default);
        }else if (isMoving)
        {
            anima_->ChangeAnimation(EnemyAnima::AnimationState::Walk);


        }
        else {
            anima_->ChangeAnimation(EnemyAnima::AnimationState::Idle);


        }
        object_->SetTranslate(expectedPos);
    }

    // ==========================================
    // 重みの適用 (Y座標の変更はここでは行わない)
    // ==========================================
    Vector3 finalPos = object_->GetTranslate();

    if (tm && !gameScene_->IsClear()) {
        if (!isOnBridge_) {
            // 1. 周辺の糸に下向きの力(重さ)を加える（これだけ残す）
            tm->ApplyWeight(finalPos, weightRadius_, weight_);
        }
    }

    // ==========================================
    // 3. 座標の確定とモデルの更新
    // ==========================================
    anima_->Update();
    object_->Update();

    if (isTrapped_ && webEffect_) {
        // 同フレームでトラップされた場合の初回の更新
        webEffect_->Update(expectedPos, Object3dCommon::GetInstance()->GetDefaultCamera());
    }
}

void Enemy::Draw() {
    object_->Draw();

    if (isTrapped_ && webEffect_) {
        // PSO をそのまま引き継ぐため、Object3dの描画の直後に呼ぶ
        webEffect_->Draw();
    }
}

void Enemy::Reset(const Vector3& pos) {
    object_->SetTranslate(pos);
    path_.clear();             // 保存されている古い経路を消去
    isTrapped_ = false;        // 捕縛状態を解除
    if (webEffect_) 
    {
        webEffect_->Stop();
    }
    canMove_ = true;           // 移動可能に戻す
    isHit_ = false;            // 当たり判定フラグをリセット
    recalculateTimer_ = 0;     // 再計算タイマーを戻す
    attackTimer_ = 0;
    // 必要に応じてアニメーションを Idle に戻す
    anima_->ChangeAnimation(EnemyAnima::AnimationState::Idle);
}

void Enemy::UpdateHeight(ThreadManager* tm) {
    // 1. 早期リターン
    if (!tm || gameScene_->IsClear()) return;

    // 2. 現在座標と糸の高さの準備
    Vector3 finalPos = object_->GetTranslate();
    float threadY = 0.0f;

    // 3. 橋の上にいなくて、かつ糸の高さを取得できた場合（糸の上にいる）
    if (!isOnBridge_ && tm->GetThreadHeight(finalPos, 0.5f, threadY)) {
        float targetY = threadY;
        float followSpeed = 0.2f;
        // 現在のY座標から糸のY座標へ滑らかに移動させる
        finalPos.y += (targetY - finalPos.y) * followSpeed;
    } else {
        // 4. 糸から降りた場合、または橋の上にいる場合はY座標を強制的に -0.4f にする
        finalPos.y = -0.4f;

        // ※もし -0.4f に向かって「スッ」と滑らかに着地・移動させたい場合は、
        // 上の行の代わりに以下のコードを使用してください。
        // finalPos.y += (-0.4f - finalPos.y) * 0.2f;
    }

    // 5. 高さを反映させた最終的な座標をセット
    object_->SetTranslate(finalPos);
    object_->Update();
}

void Enemy::DrawImGui() {
#ifdef USE_IMGUI
    // ImGuiのウィンドウを作成（"Enemy Debug"という名前）
    ImGui::Begin("Enemy Debug");

    // 現在の座標を取得
    Vector3 pos = object_->GetTranslate();

    // DragFloat3 でXYZ座標を操作できるようにする
    // （第3引数 0.1f はドラッグ時の変化量）
    if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
        // 値が変更されたらオブジェクトに反映
        object_->SetTranslate(pos);
        object_->Update();
    }

    ImGui::End();
#endif
}

Point Enemy::WorldToGrid(const Vector3& pos) {
    // CollisionMaskが512x512で、中心を(0,0)としたい場合
    const float offset = 256.0f;

    int gx = (int)std::floor(pos.x + offset);
    int gz = (int)std::floor(pos.z + offset);

    // 範囲外チェック（これをしないと配列外参照でクラッシュします）
    if (gx < 0) gx = 0; if (gx >= 512) gx = 511;
    if (gz < 0) gz = 0; if (gz >= 512) gz = 511;

    return { gx, gz };
}

Vector3 Enemy::GridToWorld(const Point& grid) {
    const float offset = 256.0f;
    return {
        (float)grid.x - offset + 0.5f,
        0.0f,
        (float)grid.y - offset + 0.5f
    };
}

Vector3 Enemy::GetWorldPosition() const
{
    // ワールド座標を入れる変数
    Vector3 worldPos;
    // ワールド行列の平行移動成分を取得
    worldPos = object_->GetTranslate();
    return worldPos;
}

AABB Enemy::GetAABB() const
{
    Vector3 worldPos = GetWorldPosition();
    AABB aabb;

    aabb.min = { worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f };
    aabb.max = { worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f };

    return aabb;
}

void Enemy::SetPosition(const Vector3& pos)
{
    object_->SetTranslate(pos);
}


void Enemy::OnCollision(Egg* egg)
{
    if (attackTimer_ <= 0)
    {
        isHit_=true;
        // 一定間隔で卵のHPを減らす
        egg->SetHP(1.0f);
        attackTimer_ = 60;
        // サウンド再生
        Audio::GetInstance()->PlayAudio(attack_, false, 1.0f);

    } else
    {
        isHit_=false;
        attackTimer_--;
    }


}
