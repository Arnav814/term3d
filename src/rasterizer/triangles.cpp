#include "triangles.hpp"
#include <boost/core/use_default.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/multi_array.hpp>
#include <limits>
#include <memory>
#include "common.hpp"

// converts from origin at center to origin at top left
template<typename T> inline void putBufPixel(boost::multi_array<T, 2>& buffer, const ivec2 coord, const T val) {
	ivec2 transformed = {buffer.shape()[0] / 2 - coord.y, buffer.shape()[1] / 2 + coord.x};
	if (0 <= transformed.y and transformed.y < (int) buffer.shape()[0] and
			0 <= transformed.x and transformed.x < (int) buffer.shape()[1])
		buffer[transformed.y][transformed.x] = val;
}

// converts from origin at center to origin at top left
template<typename T> inline T getBufPixel(const boost::multi_array<T, 2>& buffer, const ivec2 coord, const T fallback) {
	ivec2 transformed = {buffer.shape()[0] / 2 - coord.y, buffer.shape()[1] / 2 + coord.x};
	if (0 <= transformed.y and transformed.y < (int) buffer.shape()[0] and
			0 <= transformed.x and transformed.x < (int) buffer.shape()[1])
		return buffer[transformed.y][transformed.x];
	return fallback;
}

[[deprecated]] std::vector<double> interpolate(const int x0, const double y0, const int x1, const double y1) {
	// TODO: do these really need to be doubles?
	if (x0 == x1) // for only one point
		return {y0};

	assertGt(x1, x0, "Can't interpolate backwards");

	std::vector<double> out;
	out.reserve(x1 - x0 + 1);

	double slope = (double)(y1 - y0) / (x1 - x0);
	double y = y0;
	for (int x = x0; x <= x1; x++) {
		out.push_back(y);
		y += slope;
	}

	return out;
}

class Interpolate {
	private:
		int x0;
		double y0;
		int x1;
		double y1;
		double slope;

	public:
		Interpolate(const int x0, const double y0, const int x1, const double y1) :
				x0(x0), y0(y0), x1(x1), y1(y1), slope((y1-y0) / (x1-x0)) {
			assertGt(x1, x0, "Can't interpolate backwards");
		}
		Interpolate(const Interpolate& orig) :
				x0(orig.x0), y0(orig.y0), x1(orig.x1), y1(orig.y1), slope(orig.slope) {}
		
		class InterpIterator : public
				boost::iterator_facade<InterpIterator, double,
				boost::bidirectional_traversal_tag, double, int> {
			private:
				friend class boost::iterator_core_access;
				int x;
				double y;
				double slope;
				void increment() {this->y += this->slope; this->x++;}
				void decrement() {this->y -= this->slope; this->x--;}
				bool equal(const InterpIterator& other) const {return this->x == other.x;}
				double dereference() const {return this->y;}

			public:
				InterpIterator() : x(0), y(0), slope(0) {}
				InterpIterator(const int x, const double y, const double slope) :
					x(x), y(y), slope(slope) {}
		};

		InterpIterator begin() {return {this->x0, this->y0, this->slope};}
		InterpIterator end() {return {this->x1 + 1, this->y1 + this->slope, this->slope};}
		uint size() const {return x1 - x0 + 1;}
		void pop_back() {assertGt(this->x1, this->x0, "Can't decrement right point past left."); this->x1--;};

		double operator[](const uint x) {
			return (x - this->x0) * this->slope + this->y0;
		};
};
// static_assert(std::input_iterator<Interpolate::Iterator>); // TODO: make work
static_assert(std::ranges::range<Interpolate>);
static_assert(std::ranges::sized_range<Interpolate>);

template <uint num>
class BaseCollate {
	public:
		class BCIterator {
			public:
				virtual BCIterator& operator++() = 0;
				virtual BCIterator& operator--() = 0;
				virtual bool operator==(const std::shared_ptr<const BCIterator> other) const = 0;
				virtual std::array<double, num> operator*() const = 0;
				virtual ~BCIterator() = default;
		};
		virtual std::shared_ptr<BCIterator> beginCommon() = 0;
		virtual std::shared_ptr<BCIterator> endCommon() = 0;
		virtual uint size() const = 0;
		virtual std::array<double, num> operator[](const uint x) = 0;
		virtual ~BaseCollate() = default;
};

template <uint num>
class Collate : public BaseCollate<num> {
	private:
		std::array<Interpolate, num> interps;

	public:
		class ColIterator : public BaseCollate<num>::BCIterator {
			private:
				std::array<Interpolate::InterpIterator, num> iterators;

			public:
				ColIterator& operator++() override {
					for (Interpolate::InterpIterator& i: iterators) {
						i++;
					}
					return *this;
				}
				ColIterator& operator--() override {
					for (Interpolate::InterpIterator& i: iterators) {
						i--;
					}
					return *this;
				}
				bool operator==(const std::shared_ptr<const typename BaseCollate<num>::BCIterator> other) const override {
					std::shared_ptr<const ColIterator> asIt = std::dynamic_pointer_cast<const ColIterator>(other);
					assertNotEq(asIt, NULL, "Can't compare iterators of different types (Collate).");
					// should only need to compare 1
					return this->iterators[0] == asIt->iterators[0];
				}
				std::array<double, num> operator*() const override {
					std::array<double, num> out{};
					for (uint i = 0; i < num; i++) {
						out[i] = *iterators[i];
					}
					return out;
				}

				ColIterator() : iterators{} {}
				ColIterator(const std::array<Interpolate::InterpIterator, num>& iterators) :
						iterators(iterators) {}
				virtual ~ColIterator() = default;

		};
		ColIterator begin() {
			std::array<Interpolate::InterpIterator, num> iterators{};
			for (uint i = 0; i < this->interps.size(); i++) {
				iterators[i] = this->interps[i].begin();
			}
			return {iterators};
		}
		ColIterator end() {
			std::array<Interpolate::InterpIterator, num> iterators{};
			for (uint i = 0; i < this->interps.size(); i++) {
				iterators[i] = this->interps[i].end();
			}
			return {iterators};
		}
		std::shared_ptr<typename BaseCollate<num>::BCIterator> beginCommon() override {
			return std::make_shared<Collate<num>::ColIterator>(this->begin());
		}
		std::shared_ptr<typename BaseCollate<num>::BCIterator> endCommon() override {
			return std::make_shared<Collate<num>::ColIterator>(this->end());
		}
		uint size() const override {
			return interps[0].size();
		}
		std::array<double, num> operator[](const uint x) override {
			std::array<double, num> out;
			for (uint i = 0; i < this->interps.size(); i++) {
				out[i] = interps[i][x];
			}
			return out;
		}
		void pop_back() {
			for (Interpolate& i : this->interps) {
				i.pop_back();
			};
		};

		Collate(std::array<Interpolate, num> interps) : interps(interps) {
			static_assert(num > 0);
			uint size = interps[0].size();
			for (Interpolate& interp : this->interps) {
				if (interp.size() != size) {
					throw std::logic_error(std::format("Interpolations have different sizes ({} and {})", size, interp.size()));
				}
			}
		}
		virtual ~Collate() = default;
};

template <uint num>
class CollateJoin : public BaseCollate<num> {
	private:
		std::array<Collate<num>, 2> collates;
		uint switchAt;

	public:
		class ColJoinIterator : public BaseCollate<num>::BCIterator {
			private:
				uint index;
				uint switchAt;
				std::array<typename Collate<num>::ColIterator, 2> iterators;

			public:
				ColJoinIterator& operator++() override {
					this->index++;
					// TODO: don't increment past switchAt
					++this->iterators[0];
					++this->iterators[1];
					return *this;
				}
				ColJoinIterator& operator--() override {
					this->index--;
					// TODO: don't decrement before switchAt
					--this->iterators[0];
					--this->iterators[1];
					return *this;
				}
				bool operator==(const std::shared_ptr<const typename BaseCollate<num>::BCIterator> other) const override {
					std::shared_ptr<const ColJoinIterator> asIt = std::dynamic_pointer_cast<const ColJoinIterator>(other);
					assertNotEq(asIt, NULL, "Can't compare iterators of different types (CollateJoin).");
					// should only need to compare 1
					return this->index == asIt->index;
				}
				std::array<double, num> operator*() const override {
					if (index < switchAt) {
						return *iterators[0];
					} else {
						return *iterators[1];
					}
				}

				ColJoinIterator() : index(0), iterators{} {}
				ColJoinIterator(const uint index) :
						index(index), iterators{} {}
				ColJoinIterator(const uint index, std::array<typename Collate<num>::ColIterator, 2> iterators) :
						index(index), iterators(iterators) {}
				virtual ~ColJoinIterator() = default;
		};
		ColJoinIterator begin() {
			return {0, {collates[0].begin(), collates[1].begin()}};
		}
		ColJoinIterator end() {
			return {this->size(), {collates[0].end(), collates[1].end()}};
		}
		std::shared_ptr<typename BaseCollate<num>::BCIterator> beginCommon() override {
			return std::make_shared<typename CollateJoin<num>::ColJoinIterator>(this->begin());
		}
		std::shared_ptr<typename BaseCollate<num>::BCIterator> endCommon() override {
			return std::make_shared<typename CollateJoin<num>::ColJoinIterator>(this->end());
		}
		uint size() const override {
			return switchAt + collates[1].size();
		}
		std::array<double, num> operator[](const uint index) override {
			if (index < switchAt) {
				return collates[0][index];
			} else {
				return collates[1][index - switchAt];
			}
		}

		CollateJoin(const Collate<num>& a, const Collate<num>& b) :
				collates({a, b}), switchAt(a.size() - 1) {
			this->collates[0].pop_back();
		}
		virtual ~CollateJoin() = default;
};

// void drawLine(SextantDrawing& canvas, ivec2 p0, ivec2 p1, const Color color) {
// 	if (abs(p0.x - p1.x) > abs(p0.y - p1.y)) { // line is horizontalish
// 		if (p0.x > p1.x) // make sure p0 is left of p1
// 			std::swap(p0, p1);

// 		Interpolate yVals = {p0.x, static_cast<double>(p0.y), p1.x, static_cast<double>(p1.y)};
// 		for (int x = p0.x; x <= p1.x; x++) {
// 			putPixel(canvas, SextantCoord(yVals[x - p0.x], x), color);
// 		}
// 	} else { // line is verticalish
// 		if (p0.y > p1.y) // make sure p0 is under p1
// 			std::swap(p0, p1);

// 		Interpolate xVals = {p0.y, static_cast<double>(p0.x), p1.y, static_cast<double>(p1.x)};
// 		for (int y = p0.y; y <= p1.y; y++) {
// 			putPixel(canvas, SextantCoord(y, xVals[y - p0.y]), color);
// 		}
// 	}
// }

// void drawWireframeTriangle(SextantDrawing& canvas, const ivec2 p0, const ivec2 p1, const ivec2 p2, const Color color) {
//     drawLine(canvas, p0, p1, color);
//     drawLine(canvas, p1, p2, color);
//     drawLine(canvas, p2, p0, color);
// }

// void drawFilledTriangle(SextantDrawing& canvas, ivec2 p0, ivec2 p1, ivec2 p2, const Color color) {
// 	// sort top to bottom, so p0.y < p1.y < p2.y
// 	if (p1.y < p0.y) std::swap(p1, p0);
// 	if (p2.y < p0.y) std::swap(p2, p0);
// 	if (p2.y < p1.y) std::swap(p2, p1);
	
// 	// indexes represent y-values
// 	std::vector<double> shortSide1 = interpolate(p0.y, static_cast<double>(p0.x), p1.y, static_cast<double>(p1.x));
// 	std::vector<double> shortSide2 = interpolate(p1.y, static_cast<double>(p1.x), p2.y, static_cast<double>(p2.x));
// 	std::vector<double> longSide = interpolate(p0.y, static_cast<double>(p0.x), p2.y, static_cast<double>(p2.x));

// 	// combine vectors
// 	shortSide1.pop_back();
// 	// from here, shortSide1 is actually both short sides combined
// 	shortSide1.reserve(shortSide1.size() + shortSide2.size());
// 	// TODO: don't actually copy here, use boost::join() or something
// 	shortSide1.insert(shortSide1.end(), shortSide2.begin(), shortSide2.end());

// 	std::unique_ptr<std::vector<double>> xLeft;
// 	std::unique_ptr<std::vector<double>> xRight;
// 	int middleIndex = longSide.size() / 2; // some arbitrary index
// 	if (longSide.at(middleIndex) < shortSide1.at(middleIndex)) {
// 		xLeft = std::make_unique<std::vector<double>>(longSide);
// 		xRight = std::make_unique<std::vector<double>>(shortSide1);
// 	} else {
// 		xLeft = std::make_unique<std::vector<double>>(shortSide1);
// 		xRight = std::make_unique<std::vector<double>>(longSide);
// 	}

// 	for (int y = p0.y; y <= p2.y; y++) {
// 		for (int x = xLeft->at(y - p0.y); x <= xRight->at(y - p0.y); x++) {
// 			putPixel(canvas, SextantCoord(y, x), color);
// 		}
// 	}
// }

// this function is a mess
void drawFilledTriangle(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer,
		Triangle<ivec2> points, Triangle<float> depth, const Color color) {
	// sort top to bottom, so p0.y < p1.y < p2.y
	if (points[1].y < points[0].y) {
		std::swap(depth[1], depth[0]);
		std::swap(points[1], points[0]);
	}
	if (points[2].y < points[0].y) {
		std::swap(depth[2], depth[0]);
		std::swap(points[2], points[0]);
	}
	if (points[2].y < points[1].y) {
		std::swap(depth[2], depth[1]);
		std::swap(points[2], points[1]);
	}

	// indexes represent y-values
	// note: uses interpolated _reciprocal_ of depth
	enum class PointField {X = 0, Depth = 1};

	Collate<2> shortSide1 = {{
		Interpolate{points[0].y, (double) points[0].x, points[1].y, (double) points[1].x},
		Interpolate{points[0].y, 1.0/depth[0], points[1].y, 1.0/depth[1]}
	}};
	Collate<2> shortSide2 = {{
		Interpolate{points[1].y, (double) points[1].x, points[2].y, (double) points[2].x},
		Interpolate{points[1].y, 1.0/depth[1], points[2].y, 1.0/depth[2]}
	}};
	Collate<2> longSide = {{
		Interpolate{points[0].y, (double) points[0].x, points[2].y, (double) points[2].x},
		Interpolate{points[0].y, 1.0/depth[0], points[2].y, 1.0/depth[2]}
	}};

	CollateJoin shortSides = {shortSide1, shortSide2};

	std::unique_ptr<BaseCollate<2>> left;
	std::unique_ptr<BaseCollate<2>> right;
	int middleIndex = longSide.size() / 2; // some arbitrary index
	if (longSide[middleIndex] == shortSide1[middleIndex] and middleIndex != 0) middleIndex--; // fix problems if length==2

	if (longSide[middleIndex] < shortSide1[middleIndex]) {
		left = std::make_unique<Collate<2>>(longSide);
		right = std::make_unique<CollateJoin<2>>(shortSides);
	} else {
		left = std::make_unique<CollateJoin<2>>(shortSides);
		right = std::make_unique<Collate<2>>(longSide);
	}

	for (int y = points[0].y; y <= points[2].y; y++) {
		auto rowLeft = (*left)[y - points[0].y];
		auto rowRight = (*right)[y - points[0].y];

		int rowLeftX = round(rowLeft[(uint) PointField::X]);
		int rowRightX = round(rowRight[(uint) PointField::X]);
		assertGtEq(rowRightX, rowLeftX, "right is left of left");

		double rowLeftDepth = rowLeft[(uint) PointField::Depth];
		double rowRightDepth = rowRight[(uint) PointField::Depth];
		Interpolate rowDepth = {rowLeftX, rowLeftDepth, rowRightX, rowRightDepth};
		auto rowDepthIt = rowDepth.begin();

		for (int x = rowLeftX; x <= rowRightX; x++) {
			if (getBufPixel(depthBuffer, {x, y}, std::numeric_limits<float>::infinity()) < *rowDepthIt) {
				putBufPixel(depthBuffer, {x, y}, (float) *rowDepthIt);
				putPixel(canvas, SextantCoord(y, x), color);
			}
			++rowDepthIt;
		}
	}
}

// void drawShadedTriangle(SextantDrawing& canvas, ivec2 p0, ivec2 p1, ivec2 p2,
// 		Triangle<float> intensities, const Color color) {
// 	// p0, p1, and p2 = intensities a, b, and c
// 	// sort top to bottom, so p0.y < p1.y < p2.y
// 	if (p1.y < p0.y) {std::swap(p1, p0); std::swap(intensities[1], intensities[0]);}
// 	if (p2.y < p0.y) {std::swap(p2, p0); std::swap(intensities[2], intensities[0]);}
// 	if (p2.y < p1.y) {std::swap(p2, p1); std::swap(intensities[2], intensities[1]);}

// 	// Compute the x coordinates and h values of the triangle edges
// 	auto x01 = interpolate(p0.y, p0.x, p1.y, p1.x);
// 	auto h01 = interpolate(p0.y, intensities[0], p1.y, intensities[1]);

// 	auto x12 = interpolate(p1.y, p1.x, p2.y, p2.x);
// 	auto h12 = interpolate(p1.y, intensities[1], p2.y, intensities[2]);

// 	auto x02 = interpolate(p0.y, p0.x, p2.y, p2.x);
// 	auto h02 = interpolate(p0.y, intensities[0], p2.y, intensities[2]);

// 	// Concatenate the short sides
// 	x01.pop_back();
// 	x01.insert(x01.end(), x12.begin(), x12.end());
// 	#define x012 x01 // from here, x01 is reused for both x01 and x12 joined together
	
// 	// h means intensity
// 	h01.pop_back();
// 	h01.insert(h01.end(), h12.begin(), h12.end());
// 	#define h012 h01 // same for h

// 	// Determine which is left and which is right
// 	uint middleIndex = x012.size() / 2; // some arbitrary index
// 	std::unique_ptr<std::vector<double>> xLeft;
// 	std::unique_ptr<std::vector<double>> xRight;
// 	std::unique_ptr<std::vector<double>> hLeft;
// 	std::unique_ptr<std::vector<double>> hRight;
// 	if (x02.at(middleIndex) < x012.at(middleIndex)) {
// 		xLeft = std::make_unique<std::vector<double>>(x02);
// 		hLeft = std::make_unique<std::vector<double>>(h02);

// 		xRight = std::make_unique<std::vector<double>>(x012);
// 		hRight = std::make_unique<std::vector<double>>(h012);
// 	} else {
// 		xLeft = std::make_unique<std::vector<double>>(x012);
// 		hLeft = std::make_unique<std::vector<double>>(h012);

// 		xRight = std::make_unique<std::vector<double>>(x02);
// 		hRight = std::make_unique<std::vector<double>>(h02);
// 	}

// 	// Draw the horizontal segments
// 	for (int y = p0.y; y <= p2.y; y++) {
// 		double rowLeftX = xLeft->at(y - p0.y);
// 		double rowRightX = xRight->at(y - p0.y);

// 		auto rowHVals = interpolate(rowLeftX, hLeft->at(y - p0.y), rowRightX, hRight->at(y - p0.y));
// 		for (int x = rowLeftX; x <= rowRightX; x++) {
// 			RGBA shadedColor = color.color * rowHVals.at(x - rowLeftX);
// 			putPixel(canvas, SextantCoord(y, x), Color(color.category, shadedColor));
// 		}
// 	}	

// 	#undef x012
// 	#undef h012
// }

void renderTriangle(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer, const Triangle<ivec2>& triangle, const Triangle<float>& depth, Color color) {
	// std::println(std::cerr, "p0: {}, p1: {}, p2: {} ", glm::to_string(triangle.a),
	// 	glm::to_string(triangle.b), glm::to_string(triangle.c));
	drawFilledTriangle(canvas, depthBuffer, triangle, depth, color);
}

