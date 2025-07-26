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

class BaseCamera
{
public:
	// Call this function once per frame and after you've changed any state.  This
	// regenerates all matrices.  Calling it more or less than once per frame will break
	// temporal effects and cause unpredictable results.
	void Update(bool updateTemporalMatrices = true) noexcept;

	// Public functions for controlling where the camera is and its orientation
	void SetLookAt(Math::Vector3 cameraPos, Math::Vector3 targetPos, Math::Vector3 upAxis) noexcept;
	void SetLookAt(Math::Vector3 targetPos, Math::Vector3 upAxis) noexcept;
	void SetLookIn(Math::Vector3 cameraPos, Math::Vector3 lookDir, Math::Vector3 upAxis) noexcept;
	void SetLookIn(Math::Vector3 lookDir, Math::Vector3 upAxis) noexcept;

	void SetOrientation(Math::Quaternion orientation) noexcept;
	void SetPosition(Math::Vector3 worldPos) noexcept;

	void SetTransform(const Math::AffineTransform& xform) noexcept;

	const Math::Vector3 GetPosition() const noexcept;
	const Math::Quaternion GetOrientation() const noexcept;

	const Math::Vector3 GetRightVec() const noexcept { return m_basis.GetX(); }
	const Math::Vector3 GetUpVec() const noexcept { return m_basis.GetY(); }
	const Math::Vector3 GetForwardVec() const noexcept { return -m_basis.GetZ(); }

	const Math::Matrix4 GetViewToWorldMatrix() const noexcept { return m_viewToWorldMatrix; }
	const Math::Matrix4 GetViewMatrix() const noexcept { return m_viewMatrix; }
	const Math::Matrix4 GetProjectionMatrix() const noexcept { return m_projectionMatrix; }
	const Math::Matrix4 GetViewProjectionMatrix() const noexcept { return m_viewProjectionMatrix; }
	const Math::Matrix4& GetReprojectionMatrix() const noexcept { return m_reprojectMatrix; }
	const Math::Frustum& GetViewSpaceFrustum() const noexcept { return m_frustumVS; }
	const Math::Frustum& GetWorldSpaceFrustum() const noexcept { return m_frustumWS; }

protected:
	BaseCamera() = default;

	void SetProjectionMatrix(const Math::Matrix4& projectionMatrix) noexcept;

protected:
	// Cached for faster lookups
	Math::Matrix3 m_basis{ Math::kIdentity };

	Math::Matrix4 m_viewToWorldMatrix{ Math::kIdentity }; // View-to-World
	Math::Matrix4 m_viewMatrix{ Math::kIdentity }; // World-to-View

	Math::Matrix4 m_projectionMatrix{ Math::kIdentity }; // View-to-Projection

	Math::Matrix4 m_viewProjectionMatrix{ Math::kIdentity }; // World-to-View-to-Projection

	// The view-projection matrix from the previous frame
	Math::Matrix4 m_prevViewProjectionMatrix{ Math::kIdentity };

	// Projects a clip-space coordinate to the previous frame (useful for temporal effects).
	Math::Matrix4 m_reprojectMatrix{ Math::kIdentity };

	Math::Frustum m_frustumVS;		// View-space view frustum
	Math::Frustum m_frustumWS;		// World-space view frustum
};


class Camera : public BaseCamera
{
public:
	void SetPerspectiveMatrix(float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip) noexcept;

	float GetFOV() const noexcept { return m_verticalFOV; }
	float GetNearClip() const noexcept { return m_nearClip; }
	float GetFarClip() const noexcept { return m_farClip; }
	float GetClearDepth() const noexcept { return m_reverseZ ? 0.0f : 1.0f; }

protected:
	float m_verticalFOV;		// Field of view angle in radians
	float m_aspectRatio;
	float m_nearClip;
	float m_farClip;
	bool m_reverseZ{ false };	// Invert near and far clip distances so that Z=0 is the far plane
};


inline void BaseCamera::SetLookAt(Math::Vector3 cameraPos, Math::Vector3 targetPos, Math::Vector3 upAxis) noexcept
{
	m_viewMatrix = Math::Matrix4(XMMatrixLookAtRH(cameraPos, targetPos, upAxis));
	Update(false);
}


inline void BaseCamera::SetLookAt(Math::Vector3 targetPos, Math::Vector3 upAxis) noexcept
{
	const Math::Vector3 position = GetPosition();
	m_viewMatrix = Math::Matrix4(XMMatrixLookAtRH(position, targetPos, upAxis));
	Update(false);
}


inline void BaseCamera::SetLookIn(Math::Vector3 cameraPos, Math::Vector3 targetDir, Math::Vector3 upAxis) noexcept
{
	m_viewMatrix = Math::Matrix4(XMMatrixLookToRH(cameraPos, targetDir, upAxis));
	Update(false);
}


inline void BaseCamera::SetLookIn(Math::Vector3 targetDir, Math::Vector3 upAxis) noexcept
{
	const Math::Vector3 position = GetPosition();
	m_viewMatrix = Math::Matrix4(XMMatrixLookToRH(position, targetDir, upAxis));
	Update(false);
}


inline void BaseCamera::SetOrientation(Math::Quaternion orientation) noexcept
{
	XMVECTOR xmScale{};
	XMVECTOR xmRotQuat{};
	XMVECTOR xmTrans{};
	XMMatrixDecompose(&xmScale, &xmRotQuat, &xmTrans, m_viewToWorldMatrix);

	m_viewMatrix = Math::Matrix4(XMMatrixAffineTransformation(xmScale, XMVectorZero(), orientation, xmTrans));
	Update(false);
}


inline void BaseCamera::SetPosition(Math::Vector3 worldPos) noexcept
{
	XMVECTOR xmScale{};
	XMVECTOR xmRotQuat{};
	XMVECTOR xmTrans{};
	XMMatrixDecompose(&xmScale, &xmRotQuat, &xmTrans, m_viewToWorldMatrix);

	m_viewMatrix = Math::Matrix4(XMMatrixAffineTransformation(xmScale, XMVectorZero(), xmRotQuat, worldPos));
	Update(false);
}


inline void BaseCamera::SetTransform(const Math::AffineTransform& xform) noexcept
{
	SetLookIn(xform.GetTranslation(), -xform.GetZ(), xform.GetY());
	Update(false);
}


inline const Math::Vector3 BaseCamera::GetPosition() const noexcept
{
	XMVECTOR xmScale{};
	XMVECTOR xmRotQuat{};
	XMVECTOR xmTrans{};
	XMMatrixDecompose(&xmScale, &xmRotQuat, &xmTrans, m_viewToWorldMatrix);

	return Math::Vector3(xmTrans);
}


inline const Math::Quaternion BaseCamera::GetOrientation() const noexcept
{
	XMVECTOR xmScale{};
	XMVECTOR xmRotQuat{};
	XMVECTOR xmTrans{};
	XMMatrixDecompose(&xmScale, &xmRotQuat, &xmTrans, m_viewToWorldMatrix);

	return Math::Quaternion(xmRotQuat);
}


inline void BaseCamera::SetProjectionMatrix(const Math::Matrix4& projectionMatrix) noexcept
{
	m_projectionMatrix = projectionMatrix;
	Update(false);
}


inline void Camera::SetPerspectiveMatrix(float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip) noexcept
{
	m_verticalFOV = verticalFovRadians;
	m_aspectRatio = aspectHeightOverWidth;
	m_nearClip = nearZClip;
	m_farClip = farZClip;

	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(verticalFovRadians, 1.0f / aspectHeightOverWidth, nearZClip, farZClip);
	SetProjectionMatrix(Math::Matrix4(perspectiveMatrix));
	Update(false);
}

} // namespace Luna