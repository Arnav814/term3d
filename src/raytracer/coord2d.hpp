#ifndef COORD2D_HPP
#define COORD2D_HPP

// another utility coordinate struct
// all inline

struct Coord3d {
	int x; int y; int z;

	Coord3d() : Coord3d(0, 0, 0) { }

	Coord3d(const int x, const int y, const int z) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	Coord3d operator+(const Coord3d& other) const {
		return Coord3d(this->x + other.x, this->y + other.y, this->z + other.z);
	}

	Coord3d operator-(const Coord3d& other) const {
		return Coord3d(this->x - other.x, this->y - other.y, this->z - other.z);
	}

	Coord3d operator*(const int other) const {
		return Coord3d(this->x * other, this->y * other, this->z * other);
	}

	Coord3d operator/(const int other) const {
		return Coord3d(this->x / other, this->y / other, this->z / other);
	}

	bool operator==(const Coord3d& other) const = default;

	bool operator!=(const Coord3d& other) const = default;
};

#endif /* COORD2D_HPP */
