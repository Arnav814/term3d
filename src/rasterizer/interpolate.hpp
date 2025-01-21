#ifndef INTERPOLATE_HPP
#define INTERPOLATE_HPP
#include <boost/core/use_default.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/multi_array.hpp>
#include <memory>
#include "../extraAssertions.hpp"
#include "glm/ext/vector_int2.hpp"
#include "glm/ext/vector_double3.hpp"

using glm::dvec3, glm::ivec2;

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

[[deprecated]] inline std::vector<double> interpolate(const int x0, const double y0, const int x1, const double y1) {
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
			assertGtEq(x1, x0, "Can't interpolate backwards.");
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

		InterpIterator begin() const {return {this->x0, this->y0, this->slope};}
		InterpIterator end() const {return {this->x1 + 1, this->y1 + this->slope, this->slope};}
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
				virtual bool operator!=(const std::shared_ptr<const BCIterator> other) const = 0;
				virtual std::array<double, num> operator*() const = 0;
				virtual ~BCIterator() = default;
		};
		virtual std::shared_ptr<BCIterator> beginCommon() const = 0;
		virtual std::shared_ptr<BCIterator> endCommon() const = 0;
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
				bool operator!=(const std::shared_ptr<const typename BaseCollate<num>::BCIterator> other) const override {
					return not ((*this) == other);
				}
				bool operator==(const ColIterator& other) const {
					return this->iterators[0] == other->iterators[0];
				}
				bool operator!=(const ColIterator& other) const {
					return this->iterators[0] != other->iterators[0];
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
		ColIterator begin() const {
			std::array<Interpolate::InterpIterator, num> iterators{};
			for (uint i = 0; i < this->interps.size(); i++) {
				iterators[i] = this->interps[i].begin();
			}
			return {iterators};
		}
		ColIterator end() const {
			std::array<Interpolate::InterpIterator, num> iterators{};
			for (uint i = 0; i < this->interps.size(); i++) {
				iterators[i] = this->interps[i].end();
			}
			return {iterators};
		}
		std::shared_ptr<typename BaseCollate<num>::BCIterator> beginCommon() const override {
			return std::make_shared<Collate<num>::ColIterator>(this->begin());
		}
		std::shared_ptr<typename BaseCollate<num>::BCIterator> endCommon() const override {
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
				bool operator!=(const std::shared_ptr<const typename BaseCollate<num>::BCIterator> other) const override {
					return not ((*this) == other);
				}
				bool operator==(const ColJoinIterator& other) const {
					return this->index == other.index;
				}
				bool operator!=(const ColJoinIterator& other) const {
					return this->index != other.index;
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
		ColJoinIterator begin() const {
			return {0, {collates[0].begin(), collates[1].begin()}};
		}
		ColJoinIterator end() const {
			return {this->size(), {collates[0].end(), collates[1].end()}};
		}
		std::shared_ptr<typename BaseCollate<num>::BCIterator> beginCommon() const override {
			return std::make_shared<typename CollateJoin<num>::ColJoinIterator>(this->begin());
		}
		std::shared_ptr<typename BaseCollate<num>::BCIterator> endCommon() const override {
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
		}
		virtual ~CollateJoin() = default;
};

#endif /* INTERPOLATE_HPP */
