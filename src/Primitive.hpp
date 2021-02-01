#pragma once

#include <glm/glm.hpp>

struct Line {
	union {
		struct {
			glm::vec2 v1;
			glm::vec2 v2;
		};

		glm::vec2 vertices[2];
	};
};

struct Triangle {
	union {
		struct {
			glm::vec2 v1;
			glm::vec2 v2;
			glm::vec2 v3;
		};

		glm::vec2 vertices[3];
	};
};
