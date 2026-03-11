#pragma once
#include "Camera.h"
#include "Model.h"
#include "Object3D.h"


class CollisionMask
{
public: 

    // シングルトンインスタンスの取得
    static CollisionMask* GetInstance();
    /// <summary>
    /// 終了
    /// </summary>
    void Finalize();

public:

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();
    
   

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

public:

    // マスクデータを読み込む
    bool LoadFromFile(const std::string& filePath);

    // 特定座標が壁かどうかを判定
    bool IsWall(float x, float z) const;

private: // シングルトンインスタンス

    // unique_ptr が delete するために使用する構造体
    struct Deleter
    {
        void operator()(CollisionMask* p) const
        {
            // クラス内部のスコープなので private なデストラクタを呼べる
            delete p;
        }
    };

    // インスタンス
    static std::unique_ptr<CollisionMask, Deleter> instance_;

    CollisionMask() = default;
    ~CollisionMask() = default;
    CollisionMask(const CollisionMask&) = delete;
    CollisionMask& operator=(const CollisionMask&) = delete;

private: 

    std::unique_ptr<Object3d> object_;

    std::vector<uint8_t> data;
    int widthX, widthZ;


};

