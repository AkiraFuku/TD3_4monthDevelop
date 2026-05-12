#pragma once
#include <vector>
#include "Vector2.h"
#include "Vector4.h"
#include "Vector3.h"

#include "Object3d.h"
#include "Model.h"

#include "PSOManager.h"

class StageModel
{

public:

    void Initialize();

    void Finalize();

    void Update();

    void Draw();


    void Create(float wallHeight);
    void Release();

private:

    // 壁の1面（四角形）を頂点リストに追加する
    void AddWallPanel(
        float x1, float z1, float x2, float z2,
        float height, std::vector<Model::VertexData>& vertices, bool flipNormal = false);

    void AddCeilingPanel(float x1, float z1, float x2, float z2, float height,
    std::vector<Model::VertexData>& vertices);

private:
    std::unique_ptr<Object3d> object3dFloor_;
    std::unique_ptr<Object3d> object3dWall_;

    std::string modelNameFloor_;
    std::string modelNameWall_;

    std::string psoName_ = "StageModel";

    std::string textureFilePathFloor_ = "resources/uvChecker.png";
    std::string textureFilePathWall_ = "resources/uvChecker.png";

};

