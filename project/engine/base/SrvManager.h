#pragma once
#include <wrl.h>
#include<d3d12.h>
#include<cstdint>
#include <memory>
/// SRV(シェーダーリソースビュー)用の記述子ヒープを一括管理するシングルトンクラス
class SrvManager
{
public:
    //最大テクスチャ数
    static const uint32_t kMaxSRVCount= 512;

        // シングルトンインスタンス取得
    static SrvManager* GetInstance();
    friend struct std::default_delete<SrvManager>;

    void Initialize();
    void Finalize();

    ///  コマンドリストにSRV用の記述子ヒープをセットする（描画前処理）
     void PreDraw();

    /// 新しいSRV用のインデックス（スロット）を1つ確保する
    /// 確保されたヒープ内のインデックス
    uint32_t AllocateSRV();
     
    /// <summary>
   /// 指定インデックスのCPU記述子ハンドルを取得
   /// </summary>
   /// <param name="index">< /param>
   /// <returns></returns>
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
    /// <summary>
    /// 指定インデックスのGPU記述子ハンドルを取得
    /// </summary>
    /// <param name="index"></param>
    /// <returns></returns>
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);
    
    /// 2Dテクスチャ用のSRVを作成
    void CreateSRVForTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels);

    /// 構造化バッファ(StructuredBuffer)用のSRVを作成
    void CreateSRVForStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);

    ///  グラフィックスコマンドリストにルート記述子テーブルをシームレスに設定
    void SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex);
   
    /// 管理している生の記述子ヒープポインタを取得
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() {
        return descriptorHeap_;
    }

    /// ヒープの最大容量に達しているかチェック
    /// 達していればtrue、空きがあればfalse
    bool HasReachedMaxCount();
private:

    SrvManager() = default;
    ~SrvManager() = default;

    // コピー禁止
    SrvManager(const SrvManager&) = delete;
    SrvManager& operator=(const SrvManager&) = delete;

    // インスタンス保持用スマートポインタ
  static std::unique_ptr<SrvManager> instance;

    uint32_t descriptorSize_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
    uint32_t useIndex = 0;
};

