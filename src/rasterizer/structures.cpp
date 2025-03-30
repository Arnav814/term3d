#include "structures.hpp"

dmat4 parseTransform(const Transform& transform) {
	dmat4 scaleMatrix{1}; // identity matrix
	scaleMatrix[0][0] = transform.scale.x;
	scaleMatrix[1][1] = transform.scale.y;
	scaleMatrix[2][2] = transform.scale.z;

	dmat4 rotMatrix{1};
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 3; y++) {
			rotMatrix[x][y] = transform.rotation[y][x];
			// flipping x and y makes it work
			// I have no idea why
		}
	}

	dmat4 translateMatrix{1};
	translateMatrix[3][0] = transform.translation.x;
	translateMatrix[3][1] = transform.translation.y;
	translateMatrix[3][2] = transform.translation.z;

	return translateMatrix * scaleMatrix * rotMatrix;
}

Transform invertTransform(const Transform& transform) {
	return {
	    -transform.translation,
	    glm::inverse(transform.rotation),
	    {1.0 / transform.scale.x, 1.0 / transform.scale.y, 1.0 / transform.scale.z}
    };
}

std::vector<Plane> Camera::getClippingPlanes() const { // TODO: allow changing FOV
	return {
	    // four planes that define the "cone" of clipping
	    {{glm::inversesqrt(2.0), 0, glm::inversesqrt(2.0)},  0},
	    {{-glm::inversesqrt(2.0), 0, glm::inversesqrt(2.0)}, 0},
	    {{0, glm::inversesqrt(2.0), glm::inversesqrt(2.0)},  0},
	    {{0, -glm::inversesqrt(2.0), glm::inversesqrt(2.0)}, 0},
	    // for the viewport
	    {{0, 0, 1},	                                      1}  // negate till it works
	};
}
