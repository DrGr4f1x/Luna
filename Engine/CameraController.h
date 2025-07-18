//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

namespace Luna
{

// Forward declarations
class Camera;
class InputSystem;


enum class CameraMode
{
	WASD,
	ArcBall
};


class CameraController
{
public:
	// Assumes worldUp is not the X basis vector
	CameraController(Camera& camera, Math::Vector3 worldUp);

	void Update(InputSystem* inputSystem, float deltaTime, bool ignoreInput = false);

	void RefreshFromCamera();

	void SetCameraMode(CameraMode mode);
	void SetOrbitTarget(Math::Vector3 target, float zoom, float minDistance);

	void SlowMovement(bool enable) noexcept { m_fineMovement = enable; }
	void SlowRotation(bool enable) noexcept { m_fineRotation = enable; }

	void EnableMomentum(bool enable) noexcept { m_momentum = enable; }

	void SetSpeedScale(float scale) noexcept { m_speedScale = scale; }
	void SetPanScale(float scale) noexcept { m_panScale = scale; }

private:
	CameraController& operator=(const CameraController&) noexcept { return *this; }

	void ApplyMomentum(float& oldValue, float& newValue, float deltaTime);

	void UpdateWASD(InputSystem* inputSystem, float deltaTime, bool ignoreInput);
	void UpdateArcBall(InputSystem* inputSystem, float deltaTime, bool ignoreInput);

private:
	CameraMode m_mode{ CameraMode::WASD };

	Math::Vector3 m_worldUp{ Math::kZero };
	Math::Vector3 m_worldNorth{ Math::kZero };
	Math::Vector3 m_worldEast{ Math::kZero };
	Camera& m_targetCamera;
	float m_horizontalLookSensitivity{ 2.0f };
	float m_verticalLookSensitivity{ 2.0f };
	float m_moveSpeed{ 1000.0f };
	float m_strafeSpeed{ 1000.0f };
	float m_mouseSensitivityX{ 1.0f };
	float m_mouseSensitivityY{ 1.0f };

	float m_currentHeading{ 0.0f };
	float m_currentPitch{ 0.0f };

	float m_speedScale{ 1.0f };
	float m_panScale{ 1.0f };

	bool m_fineMovement{ false };
	bool m_fineRotation{ false };
	bool m_momentum{ true };

	float m_lastYaw{ 0.0f };
	float m_lastPitch{ 0.0f };
	float m_lastForward{ 0.0f };
	float m_lastStrafe{ 0.0f };
	float m_lastAscent{ 0.0f };

	Math::Vector3 m_orbitTarget{ 0.0f, 0.0f, 0.0f };
	float m_minDistance{ 0.0f };
	float m_zoom{ 1.0f };
};


} // namespace Luna