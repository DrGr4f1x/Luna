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

	void SlowMovement(bool enable) { m_fineMovement = enable; }
	void SlowRotation(bool enable) { m_fineRotation = enable; }

	void EnableMomentum(bool enable) { m_momentum = enable; }

	void SetSpeedScale(float scale) { m_speedScale = scale; }

private:
	CameraController& operator=(const CameraController&) { return *this; }

	void ApplyMomentum(float& oldValue, float& newValue, float deltaTime);

	void UpdateWASD(InputSystem* inputSystem, float deltaTime, bool ignoreInput);
	void UpdateArcBall(InputSystem* inputSystem, float deltaTime, bool ignoreInput);

private:
	CameraMode m_mode{ CameraMode::WASD };

	Math::Vector3 m_worldUp;
	Math::Vector3 m_worldNorth;
	Math::Vector3 m_worldEast;
	Camera& m_targetCamera;
	float m_horizontalLookSensitivity;
	float m_verticalLookSensitivity;
	float m_moveSpeed;
	float m_strafeSpeed;
	float m_mouseSensitivityX;
	float m_mouseSensitivityY;

	float m_currentHeading;
	float m_currentPitch;

	float m_speedScale;

	bool m_fineMovement;
	bool m_fineRotation;
	bool m_momentum;

	float m_lastYaw;
	float m_lastPitch;
	float m_lastForward;
	float m_lastStrafe;
	float m_lastAscent;

	Math::Vector3 m_orbitTarget{ 0.0f, 0.0f, 0.0f };
	float m_minDistance{ 0.0f };
	float m_zoom{ 1.0f };
};

} // namespace Luna