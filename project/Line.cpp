#include "Line.h"
#include "DXCommon.h"
#include "PSOManager.h"
#include "Logger.h"
#include "Line3dCommon.h"


void Line3d::Initialize() {
    // 頂点リソースの作成
    vertexResource_ =
        DXCommon::GetInstance()->
        CreateBufferResource(sizeof(VertexData) * 2);
    vertexBufferView_.BufferLocation =
        vertexResource_.Get()->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * 2;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
    vertexResource_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    // 座標変換リソースの作成
    transformationResource_ =
        DXCommon::GetInstance()->
        CreateBufferResource(sizeof(TransformationMatrix));
    transformationResource_->
        Map(0, nullptr, reinterpret_cast<void**>(&transformationData_));
}

void Line3d::Update(const Vector3& start, const Vector3& end, const Vector4& color) {
    // 頂点データの更新
    vertexData_[0].position = start;
    vertexData_[0].color = color;
    vertexData_[1].position = end;
    vertexData_[1].color = color;

    // 行列の計算 (Object3dと同様にカメラの行列を使用)
    Matrix4x4 viewProjection = camera_->GetViewProtectionMatrix();
    transformationData_->WVP = viewProjection; 
}

void Line3d::Draw() {
    auto commandList = DXCommon::GetInstance()->GetCommandList();
    Line3dCommon::GetInstance()->Line3dCommonDraw();
    
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->SetGraphicsRootConstantBufferView(0, transformationResource_->GetGPUVirtualAddress());
    
    // 2頂点描画
    commandList->DrawInstanced(2, 1, 0, 0);
}