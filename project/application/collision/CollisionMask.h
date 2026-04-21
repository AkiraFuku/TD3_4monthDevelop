#pragma once
#include "Camera.h"
#include "Model.h"
#include "Object3D.h"
#include "DrawFunction.h"
#include "JSONManager.h"

class CollisionMask
{
public:

    struct TextureData
    {
        std::vector<uint8_t> data;
        int widthX, widthZ;
    };

    struct MaskData
    {
        // 名前
        std::unique_ptr<Object3d> object;
        TextureData textureData;
        Vector4 min_, max_;
        std::vector<float> sdfData;
    };

    struct StageData
    {
        std::string texturePass_;
        std::unique_ptr<MaskData> maskData_;
        Vector3 startPos_;
        Vector3 eggStartPos_;
        Vector3 goalPos_;
        std::vector<Vector3> enemyStartPos_;
        std::vector<Vector3> nestMaterialPos_;
        std::vector<Vector3> oneWayObjectPos_;
    };

    struct RayResult {
        bool hit = false;
        Vector2 hitPos;  // 壁の入り口
        Vector2 exitPos; // 壁の出口（貫通先）
    };

    enum class StageID
    {
        Map0,
        Map1,
        Map2,
        Map3,
        Map5,

        Unknown,
    };

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
    bool LoadFromFile(const std::string& filePath, TextureData& textureData);

    // マスクデータから簡易的な距離場を生成
    void GenerateSDF(MaskData* mask);

    float FindNearestWallDist(int startX, int startZ, MaskData* mask);

    MaskData* GetMaskData(int num){ return stageDatas_[num]->maskData_.get(); }

    float GetSDFValue(float worldX, float worldZ);

    Vector2 GetSDFNormal(float worldX, float worldZ);

    // 特定座標が壁かどうかを判定
    bool IsWall(float x, float z) const;

    bool IsCollisionWall(const float& x, const float& z, const float& width);

    int GetCurrentStageData() const { return static_cast<int>(currentStageID_); }

    RayResult CastRayThroughWall(Vector3 start, Vector3 direction, float maxDist);

   

    const Vector3& GetTranslate() {
        return translate_;
    }


public: // 外部入出力

    void SetMaskMapRequest(const StageID& maskMapRequest){ stageChangeRequest_ = maskMapRequest; }
    void SetCurrentStageID(const int& stageID) { stageID_ = stageID; }
    const int GetCurrentStageID() { return stageID_; }

    const Vector3 GetStartPos(){ return stageDatas_[static_cast<int>(currentStageID_)]->startPos_; }
    const Vector3 GetEggStartPos(){ return stageDatas_[static_cast<int>(currentStageID_)]->eggStartPos_; }
    const Vector3 GetGoalPos(){ return stageDatas_[static_cast<int>(currentStageID_)]->goalPos_; }
    const size_t GetEnemyCount() {
        // enemyStartPos の配列サイズを返す
        return stageDatas_[static_cast<int>(currentStageID_)]->enemyStartPos_.size();
    }
    const Vector3& GetEnemyStartPos(const size_t i)
    {
        if (i >= stageDatas_[static_cast<int>(currentStageID_)]->enemyStartPos_.size())
        {
            // エラー処理: インデックスが範囲外の場合は、デフォルトの位置を返す
            static Vector3 defaultPos = { 0.0f, 0.0f, 0.0f };
            return defaultPos;
        }

        return stageDatas_[static_cast<int>(currentStageID_)]->enemyStartPos_[i]; 
    }
    const size_t GetNestMaterialCount() {
        // nestMaterialPos の配列サイズを返す
        return stageDatas_[static_cast<int>(currentStageID_)]->nestMaterialPos_.size();
    }
    const Vector3& GetNestMaterialPos(const size_t i)
    { 
        if (i >= stageDatas_[static_cast<int>(currentStageID_)]->nestMaterialPos_.size())
        {
            // エラー処理: インデックスが範囲外の場合は、デフォルトの位置を返す
            static Vector3 defaultPos = { 0.0f, 0.0f, 0.0f };
            return defaultPos;
        }

        return stageDatas_[static_cast<int>(currentStageID_)]->nestMaterialPos_[i]; 
    }
    const Vector3& GetOneWayObjectPos(const size_t i)
    { 
        if (i >= stageDatas_[static_cast<int>(currentStageID_)]->oneWayObjectPos_.size())
        {
            // エラー処理: インデックスが範囲外の場合は、デフォルトの位置を返す
            static Vector3 defaultPos = { 0.0f, 0.0f, 0.0f };
            return defaultPos;
        }
        return stageDatas_[static_cast<int>(currentStageID_)]->oneWayObjectPos_[i]; 
    }

private:

    void CreateJsonData(int stageID);

    void LoadJsonData(int stageID);

    void SaveJsonData(int stageID);

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

    std::vector<std::unique_ptr<StageData>> stageDatas_;

    Vector3 translate_ = { 0.0f, -1.0f, 0.0f };

    StageID currentStageID_ = StageID::Map5;

    StageID stageChangeRequest_ = StageID::Unknown;

    int stageID_ = 0;

private:

    JSONManager::Group stageGroup_;

};

