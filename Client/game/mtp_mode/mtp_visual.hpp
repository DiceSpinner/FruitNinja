#ifndef MTP_VISUAL_H
#define MTP_VISUAL_H
#include <glm/glm.hpp>

namespace MTP_Visual {
	constexpr glm::vec4 localSlicableColorTint = { 1.0f, 1.0f, 1.0f, 1.0f };
	constexpr glm::vec4 localSlicableColorOutline = { 0.271f, 0.482f, 0.616f, 1 };
	constexpr glm::vec4 remoteSlicableColorTint = { 1.0f, 1.0f, 1.0f, 0.1f };
	constexpr glm::vec4 remoteSlicableColorOutline = { 0.902f, 0.224f, 0.275f, 1 };
}

#endif