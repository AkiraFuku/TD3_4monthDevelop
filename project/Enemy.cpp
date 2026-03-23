#include "Enemy.h"
#include "ModelManager.h"
#include "CollisionMask.h"
#include "ThreadManager.h"
#include "Egg.h"

void Enemy::Initialize(const Vector3& pos) {
    object_ = std::make_unique<Object3d>();
    object_->Initialize();
    object_->SetModel("enemy.obj"); // 敵のモデル
    object_->SetTranslate(pos);
}

void Enemy::RecalculatePath(const Vector3& eggPos, ThreadManager* tm) {
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

        bool hasThread = false;
        if (tm) {
            for (auto& physics : tm->GetPhysicsList()) {
                for (const auto& node : physics->GetNodes()) {
                    float dx = node.currentPos.x - worldP.x;
                    float dz = node.currentPos.z - worldP.z;
                    // 糸の判定（少し広めに 1.5マス分 = 2.25f）
                    if ((dx * dx + dz * dz) < 2.25f) { hasThread = true; break; }
                }
                if (hasThread) break;
            }
        }
        return isWall && !hasThread;
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
    std::vector<Point> newPath = PathFinder::FindPath(start, goal, 512, 512, tm);

    path_.clear();
    if (newPath.empty()) {
        OutputDebugStringA("Path not found!\n");
    }
    else {
        for (const auto& p : newPath) path_.push_back(p);
        if (!path_.empty()) path_.pop_front();
        char buf[64];
        sprintf_s(buf, "Path found: size = %zu\n", path_.size());
        OutputDebugStringA(buf);
    }

}

bool Enemy::IsPathClear(const Vector3& start, const Vector3& end, ThreadManager* tm)
{

    Vector3 diff = { end.x - start.x, 0.0f, end.z - start.z };
    float distance = std::sqrt(diff.x * diff.x + diff.z * diff.z);
    if (distance < 0.5f) return true;

    int steps = static_cast<int>(distance * 2.0f); // 精度を上げるため 0.5 単位でチェック
    Vector3 unitDiff = { diff.x / distance, 0.0f, diff.z / distance };

    for (int i = 1; i <= steps; ++i) {
        float checkX = start.x + unitDiff.x * (i * 0.5f);
        float checkZ = start.z + unitDiff.z * (i * 0.5f);

        //if (tm) {
        //    bool hitIntersection = false;
        //    for (const auto& intersection : tm->GetIntersections()) {
        //        float dx = intersection.position.x - checkX;
        //        float dz = intersection.position.z - checkZ;
        //        if ((dx * dx + dz * dz) < (intersection.radius * intersection.radius)) {
        //            hitIntersection = true;
        //            break;
        //        }
        //    }
        //    if (hitIntersection) {
        //        return false; // 交差点があるので直進不可
        //    }
        //}

        // --- ここを PathFinder と同じロジックにする ---
        bool isWall = CollisionMask::GetInstance()->IsWall(checkX, checkZ);
        bool hasThread = false;

        if (tm) {
            for (auto& physics : tm->GetPhysicsList()) {
                for (const auto& node : physics->GetNodes()) {
                    float dx = node.currentPos.x - checkX;
                    float dz = node.currentPos.z - checkZ;
                    if ((dx * dx + dz * dz) < 0.64f) {
                        hasThread = true;
                        break;
                    }
                }
                if (hasThread) break;
            }
        }

        // 壁なのに糸がない場所が1つでもあれば、直進はできない
        if (isWall && !hasThread) {
            return false;
        }
    }
    return true;
}

void Enemy::Update(const Vector3& eggPos, ThreadManager* tm) {

    if (isHit_) {
        // 卵に接触していたら何もしない
        return;
    }

    Vector3 currentPos = object_->GetTranslate();
    Vector3 expectedPos = currentPos;
    bool isMoving = false;


    // ==========================================
    // 1. 移動方向と「予定座標(expectedPos)」の計算
    // ==========================================


    recalculateTimer_++;

    // タイマーが満了したか、外部からリクエストがあった場合に再計算
    if (recalculateTimer_ > 60 || shouldReplanNextUpdate_) {
        RecalculatePath(eggPos, tm);
        recalculateTimer_ = 0;
        shouldReplanNextUpdate_ = false; // フラグを戻す
    }

    if (!path_.empty()) {
        // 1. 本来目指すマスの中心
        Vector3 targetGridPos = GridToWorld(path_.front());

        // 2. 【修正】そのマスの「中」にある実際の糸の座標を探す
        Vector3 actualThreadTarget = targetGridPos; // 見つからない時のバックアップ
        float closestDist = 2.0f; // 探す範囲（1.5〜2.0くらいが適当）

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
        }
    }


    // ==========================================
    // 2. 交差点（トラップ）での捕縛判定
    // ==========================================
    if (tm && isMoving) {
        bool isTrapped = false;

        for (const auto& intersection : tm->GetIntersections()) {
            // 「現在の座標」が交差点の範囲に踏み込んでいるかチェック
            Vector3 diff = {
                currentPos.x - intersection.position.x,
                0.0f,
                currentPos.z - intersection.position.z
            };

            float distSq = diff.x * diff.x + diff.z * diff.z;

            // 敵の半径を足さずに、交差点の範囲(0.8f)に入り込んだら捕まるようにする
            float trapRadius = intersection.radius;

            // 交差点の中に足を踏み入れた！
            if (distSq <= trapRadius * trapRadius) {
                isTrapped = true;
                break; // 1つでも捕まっていればOK
            }
        }

        // 罠に捕まっている場合は移動をキャンセル（その場から動けなくなる）
        if (isTrapped) {
            expectedPos = currentPos;

            // ※もし「捕まった瞬間に交差点のド真ん中に引きずり込みたい」場合は
            // ここで expectedPos = intersection.position; などにすることもできます
        }
    }

    // ==========================================
    // 3. 座標の確定とモデルの更新
    // ==========================================
    object_->SetTranslate(expectedPos);
    object_->Update();
}

void Enemy::Draw() { object_->Draw(); }

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
        // 一定間隔で卵のHPを減らす
        egg->SetHP(1.0f);
        attackTimer_ = 60;
    }
    else
    {
        attackTimer_--;
    }


}
