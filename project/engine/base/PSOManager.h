#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <unordered_map>
#include <string>

struct IDxcBlob;

// PSO生成に必要な情報をまとめた構造体
struct PsoConfig {
    std::wstring vsPath;
    std::wstring psPath;
    std::vector<D3D12_ROOT_PARAMETER> rootParameters;
    std::vector<D3D12_STATIC_SAMPLER_DESC> samplers;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
    D3D12_DEPTH_STENCIL_DESC depth{};
    D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
    bool depthEnable = true;
    D3D12_DEPTH_WRITE_MASK depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
};

// ブレンドモード
enum class BlendMode {
    None, Normal, Add, Subtract, Multiply, Screen
};
enum class FillMode {
    kSolid,     // 塗りつぶし
    kWireFrame  // ワイヤーフレーム
};


// PSOセット（RootSig + PSO）
struct PsoSet {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
};
class PSOManager
{
public:
    static PSOManager* GetInstance();
    friend struct std::default_delete<PSOManager>;
    void Initialize();
    void Finalize();

    // 名前と設定を指定してPSOを取得
    const PsoSet& GetPso(const std::string& name, BlendMode blendMode = BlendMode::None, FillMode fillMode = FillMode::kSolid);
    using PsoGenerator = std::function<PsoConfig()>;
    // パイプラインの構成（レシピ）を登録する
    void RegisterPsoGenerator(const std::string& name, PsoGenerator generator);

private:
    PSOManager() = default;
    ~PSOManager() = default;
    PSOManager(const PSOManager&) = delete;
    PSOManager& operator=(const PSOManager&) = delete;

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
    void EnsureShaders(const std::string& name, Microsoft::WRL::ComPtr<IDxcBlob>& outVS, Microsoft::WRL::ComPtr<IDxcBlob>& outPS);

    // 生成レシピの保管庫
    std::unordered_map<std::string, PsoGenerator> generators_;
    std::unordered_map<std::string, PsoConfig> psoConfig_;
    // 実体のキャッシュ
    std::unordered_map<CacheKey, PsoSet, KeyHasher> psoCache_;
    // RootSignatureのキャッシュ（名前単位）
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSigCache_;
    struct ShaderSet {
        Microsoft::WRL::ComPtr<IDxcBlob> vs;
        Microsoft::WRL::ComPtr<IDxcBlob> ps;
    };
    std::unordered_map<std::string, ShaderSet> shaderCache_;
};


