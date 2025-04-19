#include "structures.hpp"

#include "../util/floatComparisons.hpp"
#include "../util/formatters.hpp"

#include <glm/matrix.hpp>

#include <csignal>

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

	return translateMatrix * rotMatrix * scaleMatrix;
}

/**
 * Copied from https://callumhay.blogspot.com/2010/10/decomposing-affine-transforms.html
 * @brief Determine whether this matrix represents an affine transform or not.
 * @return true if this matrix is an affine transform, false if not.
 */
bool isAffine(const dmat4& mat) {
	// First make sure the right row meets the condition that it is (0, 0, 0, 1)
	if (mat[0][3] != 0 or mat[1][3] != 0 or mat[2][3] != 0 or mat[3][3] != 1) return false;

	// Get the inverse of this matrix:
	// Make sure the matrix is invertible to begin with...
	if (std::abs(glm::determinant(mat)) <= SMALL) return false;

	// extract the translation
	dvec3 translation4x4{mat[3][0], mat[3][1], mat[3][2]};

	// Calculate the inverse and seperate the inverse translation component
	// and the top 3x3 part of the inverse matrix
	dmat4 inv4x4Matrix = glm::inverse(mat);
	dvec3 inv4x4Translation{inv4x4Matrix[3][0], inv4x4Matrix[3][1], inv4x4Matrix[3][2]};
	glm::dmat3 inv4x4Top3x3 = glm::dmat3(inv4x4Matrix);

	// Grab just the top 3x3 matrix
	glm::dmat3 top3x3Matrix = glm::dmat3(mat);
	glm::dmat3 invTop3x3Matrix = glm::inverse(top3x3Matrix);
	dvec3 inv3x3Translation = -(invTop3x3Matrix * translation4x4);

	// Make sure we adhere to the conditions of a 4x4 invertible affine transform matrix
	return matCmp(inv4x4Top3x3, invTop3x3Matrix) and vecCmp(inv4x4Translation, inv3x3Translation);
}

/**
 * copied from https://callumhay.blogspot.com/2010/10/decomposing-affine-transforms.html
 * @brief Decomposes the given matrix 'mat' into its translation, rotation and scale components.
 * @param mat The matrix to decompose.
 */
Transform decompose(dmat4 mat) {
	assertMsg(isAffine(mat), "Can't decompose a non-affine matrix.");

	// Start by extracting the translation (and/or any projection) from the given matrix
	dvec3 translation = {mat[3][0], mat[3][1], mat[3][2]};
	for (int i = 0; i < 3; i++) {
		mat[i][3] = 0.0;
		mat[3][i] = 0.0;
	}
	mat[3][3] = 1.0;

	// Extract the rotation component - this is done using polar decompostion, where
	// we successively average the matrix with its inverse transpose until there is
	// no/a very small difference between successive averages
	double norm;
	int count = 0;
	dmat4 rotation = mat;
	do {
		dmat4 nextRotation;
		dmat4 currInvTranspose = glm::inverse(glm::transpose(rotation));

		// Go through every component in the matrices and find the next matrix
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				nextRotation[i][j] = 0.5 * (rotation[i][j] + currInvTranspose[i][j]);
			}
		}

		norm = 0.0;
		for (int i = 0; i < 3; i++) {
			double n = std::abs(rotation[i][0] - nextRotation[i][0])
			           + std::abs(rotation[i][1] - nextRotation[i][1])
			           + std::abs(rotation[i][2] - nextRotation[i][2]);
			norm = std::max(norm, n);
		}
		rotation = nextRotation;
	} while (count < 100 && norm > SMALL);

	// The scale is simply the removal of the rotation from the non-translated matrix
	dmat4 scaleMatrix = glm::inverse(rotation) * mat;
	dvec3 scale = dvec3{scaleMatrix[0][0], scaleMatrix[1][1], scaleMatrix[2][2]};

	// Calculate the normalized rotation matrix and take its determinant to determine whether
	// it had a negative scale or not...
	dvec3 row1{mat[0][0], mat[0][1], mat[0][2]};
	dvec3 row2{mat[1][0], mat[1][1], mat[1][2]};
	dvec3 row3{mat[2][0], mat[2][1], mat[2][2]};
	row1 = glm::normalize(row1);
	row2 = glm::normalize(row2);
	row3 = glm::normalize(row3);
	glm::dmat3 nRotation{row1, row2, row3};

	// Special consideration: if there's a single negative scale
	// (all other combinations of negative scales will
	// be part of the rotation matrix), the determinant of the
	// normalized rotation matrix will be < 0.
	// If this is the case we apply an arbitrary negative to one
	// of the component of the scale.
	double determinant = glm::determinant(nRotation);
	if (determinant < 0) scale.x *= -1;

	// std::println(std::cerr, "rot:{}", rotation);
	return Transform{translation, rotation, scale};
}

Transform invertTransform(const Transform& transform) {
	return {
	    -transform.translation,
	    glm::inverse(transform.rotation),
	    {1.0 / transform.scale.x, 1.0 / transform.scale.y, 1.0 / transform.scale.z}
    };
}

std::vector<Plane> Camera::getClippingPlanes() const {
	return {
	    // four planes that define the "cone" of clipping
	    // no, the width/height and distance are not swapped, they are supposed to be this way
	    {glm::normalize(dvec3{this->viewportDistance, 0, this->viewportWidth}),   0                             },
	    {glm::normalize(dvec3{-this->viewportDistance, 0, this->viewportWidth}),  0                             },
	    {glm::normalize(dvec3{0, this->viewportDistance, this->viewportHeight}),  0                             },
	    {glm::normalize(dvec3{0, -this->viewportDistance, this->viewportHeight}), 0                             },
	    // for the viewport
	    {{0, 0, 1},	                                                           this->viewportDistance + SMALL}
    };
}
