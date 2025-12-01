#pragma once

#include <Eigen/Core>
#include <Eigen/Dense>

using vec6 = Eigen::Vector<double, 6>;
using mat6x6 = Eigen::Matrix<double, 6, 6>;

//#include <array>
//
//// ALL GLM HAD TO DO was support larger vectors/matrices. but NO. it had to be STUPID. so fine. i'll do it myself.
//
//struct vec6 {
//	std::array<double, 6> values;
//
//	inline vec6 operator-(const vec6& other) {
//		return vec6(values[0] - other[0], values[1] - other[1], values[2] - other[2], values[3] - other[3], values[4] - other[4], values[5] - other[5]);
//	}
//
//	inline vec6 operator*(const double other) {
//		return vec6(values[0] * other, values[1] * other, values[2] * other, values[3] * other, values[4] * other, values[5] * other);
//	}
//
//	inline vec6() {
//		for (unsigned i = 0; i < 6; i++) values[i] = 0;
//	}
//
//	inline vec6(double a, double b, double c, double d, double e, double f) {
//		values[0] = a;
//		values[1] = b;
//		values[2] = c;
//		values[3] = d;
//		values[4] = e;
//		values[5] = f;
//	}
//
//	inline double& operator[](const unsigned i) {
//		return values[i];
//	}
//
//	inline const double& operator[](const unsigned i) const {
//		return values[i];
//	}
//};
//
//struct mat6x6 {
//	std::array<double, 36> values;
//
//	inline double& operator[](const unsigned i) {
//		return values[i];
//	}
//
//	inline const double& operator[](const unsigned i) const {
//		return values[i];
//	}
//
//	inline mat6x6() {}
//
//	inline mat6x6(vec6 a, vec6 b) {
//		for (unsigned i = 0; i < 6; i++) {
//			for (unsigned j = 0; j < 6; j++) {
//				values[i + j * 6] = a[i] * b[j];
//			}
//		}
//	}
//
//	inline mat6x6 operator*(const double other) {
//		mat6x6 result = *this;
//		for (auto& v : result.values) v *= other;
//		return result;
//	} 
//
//	inline mat6x6 operator+(const mat6x6& other) {
//		mat6x6 result;
//		for (unsigned i = 0; i < 36; i++) result[i] = values[i] + other[i];
//		return result;
//	}
//
//};
//
//vec6 MultiplyVectorByInverseOfMatrix(const vec6& a, const mat6x6& b) {
//	// we use LDL decomposition
//
//
//}
//
//inline vec6 operator*(const mat6x6& a, const vec6& b) {
//	vec6 result = vec6();
//	for (unsigned i = 0; i < 6; i++)
//		for (unsigned j = 0; j < 6; j++)
//			result[i] += a[i + j * 6] * b[i]; // TODO: unclear if this is the right column major thing
//	return result;
//}