#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <d3dx12.h> // CD3DX12 構造体のために推奨
#include <dxcapi.h> // IDxcBlobのために推奨

// シェーダーバイナリをまとめて保持する構造体
struct ShaderBlobs {

    Microsoft::WRL::ComPtr<IDxcBlob> vertexShader;
    Microsoft::WRL::ComPtr<IDxcBlob> ps;
};
// パイプラインステート生成のための各種設定
struct PipelineStateConfig {
    std::wstring vertexShaderPath;
    std::wstring pixelShaderPath;

    // ルートシグネチャ生成用コールバック関数
    using RootSignatureGenerator = std::function<Microsoft::WRL::ComPtr<ID3D12RootSignature>()>;
    RootSignatureGenerator rootSignatureGenerator;

    // インプットレイアウト生成用コールバック関数
    using InputLayoutGenerator = std::function<std::vector<D3D12_INPUT_ELEMENT_DESC>()>;
    InputLayoutGenerator inputLayoutGenerator;

    D3D12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
    bool depthEnable = true;
    D3D12_DEPTH_WRITE_MASK depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
};

enum class BlendMode {
    None, Normal, Add, Subtract, Multiply, Screen
};
enum class FillMode {
    kSolid, kWireFrame
};

/// 生成されたルートシグネチャとパイプラインステートのペア
struct PipelineStateSet {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
};
/// パイプラインステート(PSO)の生成・キャッシュ・一括管理を行うクラス
class PipelineStateManager {
public:
    ///  シングルトンインスタンスを取得
    static PipelineStateManager* GetInstance();
    friend struct std::default_delete<PipelineStateManager>;

    /// キャッシュの初期化
    void Initialize();

    /// リソースの解放と後処理
    void Finalize();

    /// 指定した名前でパイプラインステートの設定を登録する
    /// param name 識別用のユニークな名前
    /// param configuration 各種シェーダーパスや生成コールバックを含む設定
    void RegisterConfiguration(const std::string& name, const PipelineStateConfig& configuration);

    ///  条件に合致するパイプラインステートを取得（キャッシュになければ自動生成）
    /// return ルートシグネチャとパイプラインステートが格納された構造体
    const PipelineStateSet& GetOrCreatePipelineState(const std::string& name, BlendMode blendMode = BlendMode::None, FillMode fillMode = FillMode::kSolid);

    /// 標準的なリニアサンプラーの設定を生成
    D3D12_STATIC_SAMPLER_DESC CreateDefaultStaticSamplerDescription();

private:
    PipelineStateManager() = default;
    ~PipelineStateManager() = default;

     // コピー禁止
    PipelineStateManager(const PipelineStateManager&) = delete;
    PipelineStateManager& operator=(const PipelineStateManager&) = delete;

    // 内部的なPSO生成関数
    void CreatePipelineState(const std::string& name, BlendMode blend, FillMode fill);

    // ブレンドステートのディスクリプションを生成
    D3D12_BLEND_DESC CreateBlendDesc(BlendMode mode);

    // シェーダーがコンパイル済みか確認し、未コンパイルならコンパイルを実行
    void EnsureShaders(const std::string& name, Microsoft::WRL::ComPtr<IDxcBlob>& outVS, Microsoft::WRL::ComPtr<IDxcBlob>& outPS);

    struct CacheKey {
        std::string name;
        BlendMode blendMode;
        FillMode fillMode;
        bool operator==(const CacheKey& other) const {
            return name == other.name && blendMode == other.blendMode && fillMode == other.fillMode;
        }
    };
    // 複合キー用のハッシュ関数
    struct KeyHasher {
        std::size_t operator()(const CacheKey& key) const {
            size_t h1 = std::hash<std::string>()(key.name);
            size_t h2 = std::hash<int>()(static_cast<int>(key.blendMode));
            size_t h3 = std::hash<int>()(static_cast<int>(key.fillMode));
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    static std::unique_ptr<PipelineStateManager> instance_;
    std::unordered_map<std::string, PipelineStateConfig> configurations_;
    std::unordered_map<CacheKey, PipelineStateSet, KeyHasher> pipelineStateCache_;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSignatureCache_;
    std::unordered_map<std::string, ShaderBlobs> shaderCache_;
};