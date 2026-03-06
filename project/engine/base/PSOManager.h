#pragma once
#include <functional>
#include <vector>
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <unordered_map>
#include <string>

struct IDxcBlob;
// パイプラインの種類
//enum class PipelineType {
//    Sprite,     // 2Dスプライト
//    Object3d,   // 3Dモデル
//    Particle,   // パーティクル
//};

// ブレンドモード
enum class BlendMode {
    None, Normal, Add, Subtract, Multiply, Screen
};
enum class FillMode {
    kSolid,     // 塗りつぶし
    kWireFrame  // ワイヤーフレーム
};
// PSO取得用キー
//struct PsoProperty {
//    PipelineType type;
//    BlendMode blendMode=BlendMode::None;
//    FillMode fillMode=FillMode::kSolid;
//
//    bool operator==(const PsoProperty& other) const {
//        return type == other.type && blendMode == other.blendMode;
//    }
//};

//// ハッシュ関数
//struct PsoPropertyHasher {
//    std::size_t operator()(const PsoProperty& p) const {
//        return std::hash<int>()(static_cast<int>(p.type)) ^
//            (std::hash<int>()(static_cast<int>(p.blendMode)) << 1);
//    }
//};

struct PsoSettings {
    std::wstring vsPath;
    std::wstring psPath;
    // ルートパラメータを構築するための関数を保持
    std::function<void(std::vector<D3D12_ROOT_PARAMETER>&, std::vector<D3D12_STATIC_SAMPLER_DESC>&)> rootSigBuilder;

    BlendMode blendMode = BlendMode::Normal;
    FillMode fillMode = FillMode::kSolid;
    D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
    bool depthWrite = true;

    // キャッシュのキーとして識別するための名前（"Sprite", "PlayerModel" など）
    std::string pipelineName;
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

    //// 必要な設定のPSOセットを取得（内部でRootSigもPSOも自動生成・キャッシュ）
    //const PsoSet& GetPsoSet(const PsoProperty& property);
    const PsoSet& GetPsoSet(const PsoSettings& settings);
private:
    PSOManager() = default;
    ~PSOManager() = default;
    PSOManager(const PSOManager&) = delete;
    PSOManager& operator=(const PSOManager&) = delete;
  static std::unique_ptr<PSOManager> instance_;

    // --- 内部生成関数 ---
    // PSOを生成する
    //void CreatePso(const PsoProperty& property);
    //// RootSignatureを生成する（タイプごとに作成）
    //void CreateRootSignature(PipelineType type);
    void CreatePso(const PsoSettings& settings);
    //void CreateRootSignature(const PsoSettings& settings);
    // ブレンド設定ヘルパー
    D3D12_BLEND_DESC CreateBlendDesc(BlendMode mode);

  

 //   void EnsureShaders(const PsoSettings& settings, Microsoft::WRL::ComPtr<IDxcBlob>& outVS, Microsoft::WRL::ComPtr<IDxcBlob>& outPS);
    
  
    //// キャッシュ
    //// PSOは「タイプ×ブレンド」で管理
    //std::unordered_map<PsoProperty, PsoSet, PsoPropertyHasher> psoCache_;

    //// RootSignatureは「タイプ」だけで管理（ブレンド違っても使い回すため）
    //std::unordered_map<PipelineType, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSignatureCache_;
    std::unordered_map<std::string, PsoSet> psoCache_;

    struct ShaderSet {
        Microsoft::WRL::ComPtr<IDxcBlob> vs;
        Microsoft::WRL::ComPtr<IDxcBlob> ps;
    };
   std::unordered_map<std::string, ShaderSet> shaderCache_;
};

