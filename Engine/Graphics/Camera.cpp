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

#include "Camera.h"

using namespace Math;


namespace Luna
{

void BaseCamera::Update(bool updateTemporalMatrices) noexcept
{
	if (updateTemporalMatrices)
	{
		m_prevViewProjectionMatrix = m_viewProjectionMatrix;
	}

	m_viewToWorldMatrix = Invert(m_viewMatrix);
	m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;

	if (updateTemporalMatrices)
	{
		m_reprojectMatrix = m_prevViewProjectionMatrix * Invert(m_viewProjectionMatrix);
	}

	Math::Quaternion rotation{ XMQuaternionRotationMatrix(m_viewToWorldMatrix) };
	m_basis = Math::Matrix3(rotation);

	m_frustumVS = Frustum(m_projectionMatrix);
	m_frustumWS = m_viewToWorldMatrix * m_frustumVS;
}

} // namespace Luna