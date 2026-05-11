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
        float height, std::vector<Model::VertexData>& vertices);

    void AddCeilingPanel(float x, float z, float height,
        std::vector<Model::VertexData>& vertices);

private:
    std::unique_ptr<Object3d> object3d_;

    std::string modelName_;

    std::string psoName_ = "StageModel";

    std::string textureFilePath_ = "resources/uvChecker.png";
   

};

