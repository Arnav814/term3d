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

	T length() const {
		return sqrt(pow(this->x, 2) + pow(this->y, 2) + pow(this->z, 2));
	}
};

template <typename T> inline double dotProduct(const Coord3d<T> a, const Coord3d<T> b) {
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

#endif /* COORD3D_HPP */
