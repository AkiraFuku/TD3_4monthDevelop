#include "Enemy.h"
#include "ModelManager.h"
#include "CollisionMask.h"
#include "ThreadManager.h"
#include "Egg.h"

void Enemy::Initialize(const Vector3& pos) {
    object_ = std::make_unique<Object3d>();
    object_->Initialize();
    object_->SetModel("player/player.obj"); // 敵のモデル
    object_->SetTranslate(pos);
}

void Enemy::RecalculatePath(const Vector3& eggPos, ThreadManager* tm) {
    Point start = WorldToGrid(object_->GetTranslate());
    Point goal = WorldToGrid(eggPos);

    auto IsInsideWall = [&](Point p) {
        if (p.x < 0 || p.x >= 512 || p.y < 0 || p.y >= 512) return true;

        bool isWall = CollisionMask::GetInstance()->IsWall((float) p.x, (float) p.y);
        bool hasThread = false;        if (tm) {
            for (auto& physics : tm->GetPhysicsList()) {
                for (const auto& node : physics->GetNodes()) {
                    float dx = node.currentPos.x - (float)p.x;
                    float dz = node.currentPos.z - (float)p.y;
                    if ((dx * dx + dz * dz) < 0.64f) { hasThread = true; break; }
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

        if (tm) {
            bool hitIntersection = false;
            for (const auto& intersection : tm->GetIntersections()) {
                float dx = intersection.position.x - checkX;
                float dz = intersection.position.z - checkZ;
                if ((dx * dx + dz * dz) < (intersection.radius * intersection.radius)) {
                    hitIntersection = true;
                    break;
                }
            }
            if (hitIntersection) {
                return false; // 交差点があるので直進不可
            }
        }

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
    if (IsPathClear(currentPos, eggPos, tm)) {
        // --- 直進できる場合 ---
        Vector3 diff = {eggPos.x - currentPos.x, 0.0f, eggPos.z - currentPos.z};
        float dist = std::sqrt(diff.x * diff.x + diff.z * diff.z);
        if (dist > 0.1f) {
            expectedPos.x += (diff.x / dist) * moveSpeed_;
            expectedPos.z += (diff.z / dist) * moveSpeed_;
            isMoving = true;
        }
        path_.clear(); // 経路リストを空にしておく
    } else {
        // --- 経路探索（A*）を使う場合 ---
        recalculateTimer_++;
        if (recalculateTimer_ > 30) {
            RecalculatePath(eggPos, tm);
            recalculateTimer_ = 0;
        }

        if (!path_.empty()) {
            Vector3 nextTarget = GridToWorld(path_.front());
            Vector3 diff = {nextTarget.x - currentPos.x, 0.0f, nextTarget.z - currentPos.z};
            float distance = std::sqrt(diff.x * diff.x + diff.z * diff.z);

            if (distance < 0.2f) {
                path_.pop_front(); // 目標地点に到達したらリストから消す
            } else {
                expectedPos.x += (diff.x / distance) * moveSpeed_;
                expectedPos.z += (diff.z / distance) * moveSpeed_;
                isMoving = true;
            }
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
    // 高さを無視し、地面の座標だけで判定
    return { (int)std::floor(pos.x), (int)std::floor(pos.z) };
}

Vector3 Enemy::GridToWorld(const Point& grid) {
    return {
        (float)grid.x + 0.5f,
        0.0f,
        (float)grid.y + 0.5f
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
