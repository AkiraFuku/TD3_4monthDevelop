#pragma once

#include "SpiderWebRenderer.h"
#include <Vector4.h>
#include <memory>
#include <vector>

class Camera;

// 1個の蜘蛛の巣のパラメータ（位置と大きさ）
struct SpiderWebData {
    Vector3 position;
    float scale;
};

// 蜘蛛の巣全体の管理を行うクラス
// 実際の描画処理はRendererに委譲し、ここでは「どこに・どれくらいの大きさで配置するか」のデータのみを管理します。
class SpiderWebManager {
public:
    // 初期化：最大表示数とカメラへのポインタをセット
    void Initialize(int maxWebs, Camera* camera);

    // 毎フレームの更新処理（Rendererへデータを渡す）
    void Update();

    // 描画処理の呼び出し
    void Draw();

    // 蜘蛛の巣を新しく追加する
    void AddWeb(const Vector3& pos, float scale = 1.0f);

    // 配置した蜘蛛の巣をすべて消去する
    void ClearWebs();

private:
    std::vector<SpiderWebData> webs_;                 // 蜘蛛の巣のデータリスト
    std::unique_ptr<SpiderWebRenderer> renderer_;     // 実際の描画担当クラス
    Camera* camera_ = nullptr;                        // カメラ情報のポインタ
    int maxWebs_ = 0;                                 // 最大表示可能数
};