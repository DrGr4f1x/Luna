//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "CameraController.h"

#include "InputSystem.h"
#include "Graphics\Camera.h"

using namespace Math;


namespace Luna
{

CameraController::CameraController(Camera& camera, Vector3 worldUp)
	: m_targetCamera(camera)
{
	m_worldUp = Normalize(worldUp);
	m_worldNorth = Normalize(Cross(m_worldUp, Vector3(kXUnitVector)));
	m_worldEast = Cross(m_worldNorth, m_worldUp);

	RefreshFromCamera();
}


void CameraController::Update(InputSystem* inputSystem, float deltaTime, bool ignoreInput)
{
	if (!ignoreInput)
	{
		if (inputSystem->IsFirstPressed(DigitalInput::kLThumbClick) || inputSystem->IsFirstPressed(DigitalInput::kKey_lshift))
		{
			m_fineMovement = !m_fineMovement;
		}

		if (inputSystem->IsFirstPressed(DigitalInput::kRThumbClick))
		{
			m_fineRotation = !m_fineRotation;
		}
	}

	switch (m_mode)
	{
	case CameraMode::WASD:
		UpdateWASD(inputSystem, deltaTime, ignoreInput);
		break;
	case CameraMode::ArcBall:
		UpdateArcBall(inputSystem, deltaTime, ignoreInput);
		break;
	}

	m_targetCamera.Update();
}


void CameraController::RefreshFromCamera()
{
	Vector3 cameraForward = m_targetCamera.GetForwardVec();
	m_currentPitch = Sin(Dot(cameraForward, m_worldUp));

	Vector3 cameraRight = m_targetCamera.GetRightVec();
	Vector3 forward = Normalize(Cross(m_worldUp, cameraRight));
	m_currentHeading = ATan2(-Dot(forward, m_worldEast), Dot(forward, m_worldNorth));

	m_lastYaw = 0.0f;
	m_lastPitch = 0.0f;
	m_lastForward = 0.0f;
	m_lastStrafe = 0.0f;
	m_lastAscent = 0.0f;
}


void CameraController::SetCameraMode(CameraMode mode)
{
	m_mode = mode;
}


void CameraController::SetOrbitTarget(Math::Vector3 target, float zoom, float minDistance)
{
	m_zoom = zoom;
	m_orbitTarget = target;
	m_minDistance = fabsf(minDistance);
	Vector3 cameraPosition = m_targetCamera.GetPosition();
	m_targetCamera.SetLookIn(target - cameraPosition, m_worldUp);
	m_targetCamera.Update();
	RefreshFromCamera();
}


void CameraController::ApplyMomentum(float& oldValue, float& newValue, float deltaTime)
{
	float blendedValue;
	if (Abs(newValue) > Abs(oldValue))
	{
		blendedValue = Lerp(newValue, oldValue, Pow(0.6f, deltaTime * 60.0f));
	}
	else
	{
		blendedValue = Lerp(newValue, oldValue, Pow(0.8f, deltaTime * 60.0f));
	}
	oldValue = blendedValue;
	newValue = blendedValue;
}


void CameraController::UpdateWASD(InputSystem* inputSystem, float deltaTime, bool ignoreInput)
{
	float speedScale = (m_fineMovement ? 0.1f : 1.0f) * m_speedScale;
	float panScale = (m_fineRotation ? 0.5f : 1.0f) * m_panScale;

	float yaw = 0.0f;
	float pitch = 0.0f;
	float forward = 0.0f;
	float strafe = 0.0f;
	float ascent = 0.0f;

	if (!ignoreInput)
	{
		if (m_allowRotation)
		{
			yaw = inputSystem->GetTimeCorrectedAnalogInput(AnalogInput::kAnalogRightStickX) * m_horizontalLookSensitivity * panScale;
			pitch = inputSystem->GetTimeCorrectedAnalogInput(AnalogInput::kAnalogRightStickY) * m_verticalLookSensitivity * panScale;
		}

		forward = m_moveSpeed * speedScale * (
			inputSystem->GetTimeCorrectedAnalogInput(AnalogInput::kAnalogLeftStickY) +
			(inputSystem->IsPressed(DigitalInput::kKey_w) ? deltaTime : 0.0f) +
			(inputSystem->IsPressed(DigitalInput::kKey_s) ? -deltaTime : 0.0f));

		strafe = m_strafeSpeed * speedScale * (
			inputSystem->GetTimeCorrectedAnalogInput(AnalogInput::kAnalogLeftStickX) +
			(inputSystem->IsPressed(DigitalInput::kKey_d) ? deltaTime : 0.0f) +
			(inputSystem->IsPressed(DigitalInput::kKey_a) ? -deltaTime : 0.0f));

		ascent = m_strafeSpeed * speedScale * (
			inputSystem->GetTimeCorrectedAnalogInput(AnalogInput::kAnalogRightTrigger) -
			inputSystem->GetTimeCorrectedAnalogInput(AnalogInput::kAnalogLeftTrigger) +
			(inputSystem->IsPressed(DigitalInput::kKey_e) ? deltaTime : 0.0f) +
			(inputSystem->IsPressed(DigitalInput::kKey_q) ? -deltaTime : 0.0f));

		if (m_momentum)
		{
			ApplyMomentum(m_lastYaw, yaw, deltaTime);
			ApplyMomentum(m_lastPitch, pitch, deltaTime);
			ApplyMomentum(m_lastForward, forward, deltaTime);
			ApplyMomentum(m_lastStrafe, strafe, deltaTime);
			ApplyMomentum(m_lastAscent, ascent, deltaTime);
		}

		// don't apply momentum to mouse inputs
		if (m_allowRotation && inputSystem->IsPressed(DigitalInput::kMouse0))
		{
			yaw += inputSystem->GetAnalogInput(AnalogInput::kAnalogMouseX) * m_mouseSensitivityX;
			pitch += inputSystem->GetAnalogInput(AnalogInput::kAnalogMouseY) * m_mouseSensitivityY;
		}
	}

	m_currentPitch += pitch;
	m_currentPitch = DirectX::XMMin(XM_PIDIV2, m_currentPitch);
	m_currentPitch = DirectX::XMMax(-XM_PIDIV2, m_currentPitch);

	m_currentHeading -= yaw;
	if (m_currentHeading > XM_PI)
	{
		m_currentHeading -= XM_2PI;
	}
	else if (m_currentHeading <= -XM_PI)
	{
		m_currentHeading += XM_2PI;
	}

	Vector3 cameraPosition = m_targetCamera.GetPosition();
	Matrix3 orientation = Matrix3(m_worldEast, m_worldUp, -m_worldNorth) * Matrix3::MakeYRotation(m_currentHeading) * Matrix3::MakeXRotation(m_currentPitch);
	Vector3 position = orientation * Vector3(strafe, ascent, -forward) + cameraPosition;
	m_targetCamera.SetTransform(AffineTransform(orientation, position));
}


void CameraController::UpdateArcBall(InputSystem* inputSystem, float deltaTime, bool ignoreInput)
{
	float panScale = (m_fineRotation ? 0.5f : 1.0f) * m_panScale;

	float yaw = 0.0f;
	float pitch = 0.0f;

	if (!ignoreInput)
	{
		yaw = inputSystem->GetTimeCorrectedAnalogInput(AnalogInput::kAnalogRightStickX) * m_horizontalLookSensitivity * panScale;
		pitch = inputSystem->GetTimeCorrectedAnalogInput(AnalogInput::kAnalogRightStickY) * m_verticalLookSensitivity * panScale;

		if (m_momentum)
		{
			ApplyMomentum(m_lastYaw, yaw, deltaTime);
			ApplyMomentum(m_lastPitch, pitch, deltaTime);
		}

		// don't apply momentum to mouse inputs
		if (inputSystem->GetCaptureMouse() || inputSystem->IsPressed(DigitalInput::kMouse0))
		{
			yaw += inputSystem->GetAnalogInput(AnalogInput::kAnalogMouseX) * m_mouseSensitivityX;
			pitch += inputSystem->GetAnalogInput(AnalogInput::kAnalogMouseY) * m_mouseSensitivityY;
		}
	}

	const float epsilon = 0.05f;
	m_currentPitch -= pitch;
	m_currentPitch = DirectX::XMMin(XM_PIDIV2 - epsilon, m_currentPitch);
	m_currentPitch = DirectX::XMMax(-XM_PIDIV2 + epsilon, m_currentPitch);

	m_currentHeading -= yaw;
	if (m_currentHeading > XM_PI)
	{
		m_currentHeading -= XM_2PI;
	}
	else if (m_currentHeading <= -XM_PI)
	{
		m_currentHeading += XM_2PI;
	}

	float zoom = ignoreInput ? 0.0f : inputSystem->GetAnalogInput(AnalogInput::kAnalogMouseScroll);
	m_zoom += zoom;
	m_zoom = DirectX::XMMax(m_minDistance, m_zoom);

	// ArcBall logic
	Vector3 position = Quaternion(m_currentPitch, m_currentHeading, 0.0f) * (-1.0f * Vector3(kZUnitVector));
	position = m_zoom * position;
	position += m_orbitTarget;
	m_targetCamera.SetLookAt(position, m_orbitTarget, Vector3(kYUnitVector));
}

} // namespace Luna