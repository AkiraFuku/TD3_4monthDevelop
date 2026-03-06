#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <unordered_map>
#include <string>

struct IDxcBlob;
// パイプラインの種類
enum class PipelineType {
    Sprite,     // 2Dスプライト
    Object3d,   // 3Dモデル
    Particle,   // パーティクル
};

// ブレンドモード
enum class BlendMode {
    None, Normal, Add, Subtract, Multiply, Screen
};
enum class FillMode {
    kSolid,     // 塗りつぶし
    kWireFrame  // ワイヤーフレーム
};
// PSO取得用キー
struct PsoProperty {
    PipelineType type;
    BlendMode blendMode=BlendMode::None;
    FillMode fillMode=FillMode::kSolid;

    bool operator==(const PsoProperty& other) const {
        return type == other.type && blendMode == other.blendMode;
    }
};

// ハッシュ関数
struct PsoPropertyHasher {
    std::size_t operator()(const PsoProperty& p) const {
        return std::hash<int>()(static_cast<int>(p.type)) ^
            (std::hash<int>()(static_cast<int>(p.blendMode)) << 1);
    }
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

    // 必要な設定のPSOセットを取得（内部でRootSigもPSOも自動生成・キャッシュ）
    const PsoSet& GetPsoSet(const PsoProperty& property);

private:
    PSOManager() = default;
    ~PSOManager() = default;
    PSOManager(const PSOManager&) = delete;
    PSOManager& operator=(const PSOManager&) = delete;

    // --- 内部生成関数 ---
    // PSOを生成する
    void CreatePso(const PsoProperty& property);
    // RootSignatureを生成する（タイプごとに作成）
    void CreateRootSignature(PipelineType type);

    // ブレンド設定ヘルパー
    D3D12_BLEND_DESC CreateBlendDesc(BlendMode mode);

  

    void EnsureShaders(PipelineType type, Microsoft::WRL::ComPtr<IDxcBlob>& outVS, Microsoft::WRL::ComPtr<IDxcBlob>& outPS);
    
    static std::unique_ptr<PSOManager> instance_;

    // キャッシュ
    // PSOは「タイプ×ブレンド」で管理
    std::unordered_map<PsoProperty, PsoSet, PsoPropertyHasher> psoCache_;

    // RootSignatureは「タイプ」だけで管理（ブレンド違っても使い回すため）
    std::unordered_map<PipelineType, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSignatureCache_;

    struct ShaderSet {
        Microsoft::WRL::ComPtr<IDxcBlob> vs;
        Microsoft::WRL::ComPtr<IDxcBlob> ps;
    };
    std::unordered_map<PipelineType, ShaderSet> shaderCache_;
};

