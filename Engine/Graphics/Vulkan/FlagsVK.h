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

#include "Core\FlagStringMap.h"

namespace Luna::VK
{

inline static const FlagStringMap32 g_queueFlagsMap{
	{ VK_QUEUE_GRAPHICS_BIT,	"Graphics" },
	{ VK_QUEUE_COMPUTE_BIT,		"Compute" },
	{ VK_QUEUE_TRANSFER_BIT,	"Transfer" }
};


inline static const FlagStringMap32 g_resolveModeFlagsMap{
	{ VK_RESOLVE_MODE_NONE, "None" },
	{ VK_RESOLVE_MODE_SAMPLE_ZERO_BIT, "Sample Zero" },
	{ VK_RESOLVE_MODE_AVERAGE_BIT, "Average" },
	{ VK_RESOLVE_MODE_MIN_BIT, "Min" },
	{ VK_RESOLVE_MODE_MAX_BIT, "Max" }
};


inline static const FlagStringMap32 g_sampleCountFlagsMap{
	{ VK_SAMPLE_COUNT_1_BIT,	"1" },
	{ VK_SAMPLE_COUNT_2_BIT,	"2" },
	{ VK_SAMPLE_COUNT_4_BIT,	"4" },
	{ VK_SAMPLE_COUNT_8_BIT,	"8" },
	{ VK_SAMPLE_COUNT_16_BIT,	"16" },
	{ VK_SAMPLE_COUNT_32_BIT,	"32" },
	{ VK_SAMPLE_COUNT_64_BIT,	"64" }
};


inline static const FlagStringMap32 g_shaderStageFlagsMap{
	{ VK_SHADER_STAGE_VERTEX_BIT, "Vertex" },
	{ VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "Tessellation Control" },
	{ VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "Tessellation Evaluation" },
	{ VK_SHADER_STAGE_GEOMETRY_BIT, "Geometry" },
	{ VK_SHADER_STAGE_FRAGMENT_BIT, "Fragment" },
	{ VK_SHADER_STAGE_COMPUTE_BIT, "Compute" },
	{ VK_SHADER_STAGE_ALL_GRAPHICS, "All Graphics" },
	{ VK_SHADER_STAGE_RAYGEN_BIT_KHR, "RayGen" },
	{ VK_SHADER_STAGE_ANY_HIT_BIT_KHR, "Any Hit" },
	{ VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "Closest Hit" },
	{ VK_SHADER_STAGE_MISS_BIT_KHR, "Miss" },
	{ VK_SHADER_STAGE_INTERSECTION_BIT_KHR, "Intersection" },
	{ VK_SHADER_STAGE_CALLABLE_BIT_KHR, "Callable" },
	{ VK_SHADER_STAGE_TASK_BIT_EXT, "Task" },
	{ VK_SHADER_STAGE_MESH_BIT_EXT, "Mesh" }
};


inline static const FlagStringMap32 g_subgroupFeatureFlagsMap{
	{ VK_SUBGROUP_FEATURE_BASIC_BIT, "Basic" },
	{ VK_SUBGROUP_FEATURE_VOTE_BIT, "Vote" },
	{ VK_SUBGROUP_FEATURE_ARITHMETIC_BIT, "Arithmetic" },
	{ VK_SUBGROUP_FEATURE_BALLOT_BIT, "Ballot" },
	{ VK_SUBGROUP_FEATURE_SHUFFLE_BIT, "Shuffle" },
	{ VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT, "Shuffle Relative" },
	{ VK_SUBGROUP_FEATURE_CLUSTERED_BIT, "Clustered" },
	{ VK_SUBGROUP_FEATURE_QUAD_BIT, "Quad" },
	{ VK_SUBGROUP_FEATURE_PARTITIONED_BIT_NV, "Partitioned" }
};

} // namespace Luna::VK
