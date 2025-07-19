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

#include "Profiling.h"

#include "Application.h"

#include "pix3.h"


namespace Luna
{

void BeginEvent(const std::string& event)
{
#if ENABLE_DEBUG_MARKERS
	PIXBeginEvent(0, event.c_str());
#endif
}


void EndEvent()
{
#if ENABLE_DEBUG_MARKERS
	PIXEndEvent();
#endif
}


void SetMarker(const std::string& marker)
{
#if ENABLE_DEBUG_MARKERS
	PIXSetMarker(0, marker.c_str());
#endif
}


ScopedEvent::ScopedEvent(const std::string& event)
{
#if FRAMEPRO_ENABLED
	if (IsFrameProRunning())
	{
		FRAMEPRO_GET_CLOCK_COUNT(m_startTime);
		m_stringId = FramePro::RegisterString(event.c_str());
		m_eventStarted = true;
	}
#endif

	BeginEvent(event);
}

ScopedEvent::~ScopedEvent()
{
	EndEvent();

#if FRAMEPRO_ENABLED
	if (m_eventStarted && IsFrameProRunning())
	{
		int64_t endTime = 0;
		FRAMEPRO_GET_CLOCK_COUNT(endTime);
		int64_t duration = endTime - m_startTime;
		if (duration < 0)
		{
			return;
		}
		else if (FramePro::IsConnected() && (duration > FramePro::GetConditionalScopeMinTime()))
		{
			FramePro::AddTimeSpan(m_stringId, "none", m_startTime, endTime);
		}
	}
#endif
}

} // namespace Luna