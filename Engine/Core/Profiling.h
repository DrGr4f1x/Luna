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

void BeginEvent(const std::string& event);
void EndEvent();

void SetMarker(const std::string& marker);


class ScopedEvent
{
public:
	explicit ScopedEvent(const std::string& event);
	~ScopedEvent();

#if FRAMEPRO_ENABLED
	int64_t m_startTime{ 0 };
	FramePro::StringId m_stringId{ 0 };
#endif
};

} // namespace Luna
