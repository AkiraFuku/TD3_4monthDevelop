#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>

// 前方宣言
struct IDxcBlob;

// 設定値
enum class BlendMode {
    None, Normal, Add, Subtract, Multiply, Screen
};
enum class FillMode {
    kSolid, kWireFrame
};

// PSO生成に必要な情報をまとめた構造体
struct PsoConfig {
    std::wstring vsPath;
    std::wstring psPath;
    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    D3D12_STATIC_SAMPLER_DESC samplers{};
    // std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
    D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
    bool depthEnable = true;
    D3D12_DEPTH_WRITE_MASK depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
};

// PSOセット
struct PsoSet {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
};

class PSOManager {
public:
    // 生成関数を外部から登録するための型定義
    using PsoGenerator = std::function<PsoConfig()>;

    static PSOManager* GetInstance();
    friend struct std::default_delete<PSOManager>;
    void Initialize();
    void Finalize();

    // パイプラインの構成（レシピ）を登録する
    void RegisterPsoGenerator(const std::string& name, PsoGenerator generator);

    // 名前と設定を指定してPSOを取得
    const PsoSet& GetPso(const std::string& name, BlendMode blendMode = BlendMode::None, FillMode fillMode = FillMode::kSolid);

private:
    PSOManager() = default;
    ~PSOManager() = default;

    // 内部生成
    void CreatePso(const std::string& name, BlendMode blend, FillMode fill);
    D3D12_BLEND_DESC CreateBlendDesc(BlendMode mode);

    // キャッシュ用キー
    struct CacheKey {
        std::string name;
        BlendMode blend;
        FillMode fill;
        bool operator==(const CacheKey& o) const {
            return name == o.name && blend == o.blend && fill == o.fill;
        }
    };
    struct KeyHasher {
        std::size_t operator()(const CacheKey& k) const {
            return std::hash<std::string>()(k.name) ^ (std::hash<int>()((int)k.blend) << 1);
        }
    };

    static std::unique_ptr<PSOManager> instance_;

    // 生成レシピの保管庫
    std::unordered_map<std::string, PsoGenerator> generators_;
    // 実体のキャッシュ
    std::unordered_map<CacheKey, PsoSet, KeyHasher> psoCache_;
    // RootSignatureのキャッシュ（名前単位）
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSigCache_;

    std::unordered_map<std::string, PsoConfig> configCache_;
};