#ifndef COORD3D_HPP
#define COORD3D_HPP

// another utility coordinate struct
// all inline

template <typename T> struct Coord3d {
	T x; T y; T z;

	Coord3d() : Coord3d(0, 0, 0) { }

	Coord3d(const T x, const T y, const T z) {
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

	Coord3d operator*(const double other) const {
		return Coord3d(this->x * other, this->y * other, this->z * other);
	}

	Coord3d operator/(const double other) const {
		return Coord3d(this->x / other, this->y / other, this->z / other);
	}

	bool operator==(const Coord3d& other) const = default;

	bool operator!=(const Coord3d& other) const = default;
};

template <typename T> inline double dotProduct(Coord3d<T> a, Coord3d<T> b) {
	
}

#endif /* COORD3D_HPP */
