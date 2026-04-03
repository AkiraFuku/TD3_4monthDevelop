#include "DebugCamera.h"
#include "Input.h"
#include "ImGuiManager.h"
#include <imgui_internal.h>

void DebugCamera::Initialize()
{

	viewMatrix_ = MakeIdentity4x4();
	projectionMatrix_ = MakeIdentity4x4();
}

void DebugCamera::Update(EulerTransform originCamera)
{
#ifdef USE_DEBUGCAM

	input_ = Input::GetInstance();

	Vector3 move = { 0.0f, 0.0f, 0.0f };

	EulerTransform camera = { 0.0f, 0.0f, 0.0f };

	ImGui::Begin("DebugCameraWindow");

	if (input_->PushMouseDown(Input::MouseButton::Middle) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
	{
		// ウィンドウ上ではない領域でのドラッグ
		ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle, 0.0f);

		// カメラ角度に反映
		phi -= delta.x * rotationSpeed;
		theta -= delta.y * rotationSpeed;

		if (phi > pi)
		{
			phi = phi - 2.0f * pi;
		} else if (phi < -pi)
		{
			phi = phi + 2.0f * pi;
		}

		// ピッチを上下90°未満にクランプ
		const float limit = 0.4999f * pi; // 約85°
		theta = std::clamp(theta, -limit, limit);

		ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);

		ImGui::Text("Delta x : %3.6f, y : %3.6f", delta.x, delta.y);

	}

	LONG wheel = input_->GetMouseMove().z; // 前フレームとの差分
	radius -= wheel * 0.005f;              // 感度は 0.1f などで調整
	radius = max(1.0f, min(radius, 100.0f)); // クランプして近づきすぎ防止

	camera.rotate.x = theta;
	camera.rotate.y = phi;

    camera.translate.x = radius * std::sin(phi) * std::cos(theta);
    camera.translate.y = -radius * std::sin(theta); 
    camera.translate.z = -radius * std::cos(phi) * std::cos(theta);

	worldMatrix_ = Inverse(MakeLookAtMatrix(camera.translate, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }));

    // 位置 (Translate)
    debugCamera_.translate = camera.translate;

	ImGui::End();

    ImGui::Begin("DebugCamera Info");

    ImGui::DragFloat3("translate", &camera.translate.x, 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_ReadOnly);

    ImGui::InputFloat("radius", &radius, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);

    ImGui::End();

#endif
}


