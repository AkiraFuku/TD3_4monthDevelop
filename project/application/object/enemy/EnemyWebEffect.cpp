#define NOMINMAX
#include "EnemyWebEffect.h"
#include "ModelManager.h"
#include <cmath>

#include "MathFunction.h"
#include "Object3d.h"

void EnemyWebEffect::Initialize() {
    renderer_ = std::make_unique<ThreadRenderer>();
    // maxThreads=1, nodesPerThread=300, radius=0.04f, radialSegments=6
    renderer_->Initialize(1, 300, 0.01f, 6);

    // 繭玉モデルの読み込みと初期化
    ModelManager::GetInstance()->LoadModel("resources", "cocoon/cocoon.obj");
    cocoonObject_ = std::make_unique<Object3d>();
    cocoonObject_->Initialize();
    cocoonObject_->SetModel("cocoon/cocoon.obj");
    cocoonObject_->SetBlendMode(BlendMode::Normal); // アルファブレンド用
    cocoonScale_ = 0.0f;
    cocoonObject_->SetScale({cocoonScale_, cocoonScale_, cocoonScale_});

    isActive_ = false;
}

void EnemyWebEffect::Update(const Vector3& targetPos, Camera* camera)
{

    cocoonObject_->SetTranslate(targetPos);
    cocoonObject_->Update();

    if (state_ == WebState::None) return;

    if (state_ == WebState::Winding) {
        if (totalGeneratedNodes_ < MAX_TOTAL_NODES) {
            for (int i = 0; i < 3; ++i) {
                wrapAngle_ += 0.5f;
                wrapHeight_ += wrapDir_ * 0.03f;
                if (wrapHeight_ > 1.6f) wrapDir_ = -1.0f;
                if (wrapHeight_ < 0.0f) wrapDir_ = 1.0f;

                // --- 半径の計算をここから変更 ---

                // エネミーの高さの中心 (0.0f と 1.6f の中間)
                float centerHeight = 0.6f;

                // 現在の高さが中心からどれくらい離れているかを -1.0(下端) ～ 0.0(中心) ～ 1.0(上端) で正規化
                float t = (wrapHeight_ - centerHeight) / centerHeight;

                // ★変更点1：縦長にするために最大半径を小さくする（エネミーの幅に合わせて微調整してください）
                float maxRadius = 0.8f;  // 例：直径1.0fになるので、高さ1.6fに対して縦長になります
                float minRadius = 0.3f;  // 上下端も少し細くする

                // ★変更点2：放物線ではなく、真の「楕円のカーブ(平方根)」を使う
                // tは -1.0 ～ 1.0 なので、1.0f - t * t は必ず 0.0 ～ 1.0 になります
                float curve = std::sqrt(1.0f - t * t);

                float radius = minRadius + (maxRadius - minRadius) * curve;

                // --------------------------------

                Vector3 offset = {std::cos(wrapAngle_) * radius, wrapHeight_, std::sin(wrapAngle_) * radius};
                Vector3 pos = targetPos + offset;

                PhysicsNode node;
                node.currentPos = pos;
                node.isFixed = true;

                nodes_.push_back(node);
                totalGeneratedNodes_++; // 総生成数をカウント

                // 軌跡の長さを制限（古い尻尾から消す）
                if (nodes_.size() > MAX_TRAIL_LENGTH) {
                    nodes_.erase(nodes_.begin());
                }

                if (totalGeneratedNodes_ >= MAX_TOTAL_NODES) {
                    state_ = WebState::Formed;
                    break;
                }
            }
        }
    }

    // 巻き終わり後、残っている軌跡（尻尾）をシュッと最後まで巻き取って消す
    if (state_ == WebState::Formed) {
        if (!nodes_.empty()) {
            // 毎フレーム複数個消すことで、スピーディにシュッと消える演出にする
            int eraseCount = std::min(3, static_cast<int>(nodes_.size()));
            nodes_.erase(nodes_.begin(), nodes_.begin() + eraseCount);
        }
    }

    std::vector<std::vector<PhysicsNode>> allNodes;
    if (!nodes_.empty()) {
        allNodes.push_back(nodes_);
    }

    // nodes_が空でもカメラ更新などは必要かもしれないため、状況に合わせてrenderer_->Updateを呼ぶ
    if (!allNodes.empty()) {
        renderer_->Update(allNodes, camera);
    }

    // ▼ 徐々にスケールを大きくしていく ▼
    if (state_ == WebState::Winding || state_ == WebState::Formed) {
        if (scaleProgress_ < 1.0f) {
            // 進行度を進める（元の0.05fだと一瞬で終わってバウンスが見えにくいので、少し遅めの 0.02f にしています）
            scaleProgress_ += 0.02f;
            if (scaleProgress_ > 1.0f) {
                scaleProgress_ = 1.0f;
            }

            // ★ 進行度(0.0~1.0)をイージング関数に渡して、スケールを決定する
            cocoonScale_ = EaseOutBounce(scaleProgress_);

            // もし最終的な大きさを 1.0f 以外にしたい場合は、ここで掛け算します
            // 例: cocoonObject_->SetScale({cocoonScale_ * 1.5f, cocoonScale_ * 1.5f, cocoonScale_ * 1.5f});
            cocoonObject_->SetScale({cocoonScale_, cocoonScale_, cocoonScale_});
        }
    }
}

void EnemyWebEffect::Draw()
{
    // isActive_ が false なら描画しない
    if (!isActive_ || state_ == WebState::None) return;

    // ノードが残っていれば糸を描画
    if (!nodes_.empty()) {
        renderer_->Draw();
    }

    // 繭のモデル描画
    if (cocoonScale_ > 0.0f) {
        cocoonObject_->Draw();
    }
}

void EnemyWebEffect::Start()
{
    isActive_ = true;
    state_ = WebState::Winding;
    nodes_.clear();
    wrapAngle_ = 0.0f;
    wrapHeight_ = 0.0f;
    wrapDir_ = 1.0f;
    totalGeneratedNodes_ = 0; // カウンターをリセット

    scaleProgress_ = 0.0f;
    cocoonScale_ = 0.0f;
    if (cocoonObject_) cocoonObject_->SetScale({cocoonScale_, cocoonScale_, cocoonScale_});
}

void EnemyWebEffect::Stop()
{
    isActive_ = false;
    state_ = WebState::None;
    nodes_.clear();

    scaleProgress_ = 0.0f;
    cocoonScale_ = 0.0f;
    if (cocoonObject_) cocoonObject_->SetScale({cocoonScale_, cocoonScale_, cocoonScale_});
}

float EnemyWebEffect::EaseOutBounce(float x) {
    if (x < 0.4f) {
        float t = x / 0.4f; // 0.0 ～ 1.0 に正規化
        float skewedT = std::sqrt(t);

        return 0.1f * t + 0.6f * std::sin(skewedT * PI);
    } else {
        float t = (x - 0.4f) / 0.6f; // 0.0 ～ 1.0 に正規化
        float skewedT = std::sqrt(t);

        return (0.1f + 0.9f * t) + 0.8f * std::sin(skewedT * PI);
    }
}