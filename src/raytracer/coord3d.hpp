#ifndef COORD3D_HPP
#define COORD3D_HPP

/*

#include "../extraAssertions.hpp"
#include <boost/multi_array.hpp>

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

    Coord3d operator-() const {
        return Coord3d(-this->x, -this->y, -this->z);
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

// TODO: make size compile-time, not run-time
struct Matrix {
    private:
        boost::multi_array<double, 2> matrix;
    public:

    Matrix(std::initializer_list<std::initializer_list<double>> init) {
        uint height = init.size();
        uint width;
        if (height == 0)
            width = 0;
        else
            width = init.begin()->size();

        this->matrix.resize(boost::extents[width][height]);

        int x = 0, y = 0;
        for (const auto& list: init) {
            assertEq(width, list.size(), "All sub-lists must have the same size");
            x = 0;
            for (const auto& elem: list) {
                this->matrix[x][y] = elem;
                x++;
            }
            y++;
        }
    }

    Matrix(const Coord3d<double>& coord) {
        this->matrix.resize(boost::extents[1][3]);
        this->matrix[0][0] = coord.x;
        this->matrix[0][1] = coord.y;
        this->matrix[0][2] = coord.z;
    }

    Matrix operator+(const Matrix& other) {
        if (not (this->matrix.size() == other.matrix.size() && this->matrix[0].size() ==
other.matrix[0].size())) throw std::logic_error("Cannot add matrices of different sizes");

        for (uint x = 0; x < this->matrix.size(); x++) {
            for (uint y = 0; y < this->matrix[0].size(); y++) {
                this->matrix[x][y] += other.matrix[x][y];
            }
        }

        return *this;
    }

    Matrix operator*(const double scale) {
        for (uint x = 0; x < this->matrix.size(); x++) {
            for (uint y = 0; y < this->matrix[0].size(); y++) {
                this->matrix[x][y] *= scale;
            }
        }
        return *this;
    }

    Matrix operator*(const Matrix& other) {

    }

    bool operator==(const Matrix& other) const = default;
    bool operator!=(const Matrix& other) const = default;
};

*/

#endif /* COORD3D_HPP */
