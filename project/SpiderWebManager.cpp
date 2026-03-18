#include "SpiderWebManager.h"
#include <cassert>

void SpiderWebManager::Initialize(int maxWebs, Camera* camera) {
    // カメラがnullでないことを保証
    assert(camera);

    camera_ = camera;
    maxWebs_ = maxWebs;

    // パフォーマンス最適化：あらかじめ最大数分のメモリを確保しておくことで、
    // push_back時の不要なメモリ再確保（コピー処理）を防ぐ
    webs_.reserve(maxWebs_);

    // Rendererの生成と初期化
    renderer_ = std::make_unique<SpiderWebRenderer>();
    renderer_->Initialize(maxWebs_);
}

void SpiderWebManager::Update() {
    // データリストをそのままRendererに渡し、Renderer側で行列計算などを行わせる
    renderer_->Update(webs_, camera_);
}

void SpiderWebManager::Draw() {
    renderer_->Draw();
}

void SpiderWebManager::AddWeb(const Vector3& pos, float scale) {
    // 最大数を超過しようとした場合は追加せずに無視する（エラー落ちを防ぐ）
    if (webs_.size() >= maxWebs_) {
        return;
    }

    // リストに新しい蜘蛛の巣データを追加
    webs_.push_back({pos, scale});
}

void SpiderWebManager::ClearWebs() {
    // リストを空にする（メモリのcapacityは維持されるので軽量）
    webs_.clear();
}