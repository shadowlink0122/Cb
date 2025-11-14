#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

extern "C" {

// 基本的な算術関数
int cpp_add(int a, int b) { return a + b; }

int cpp_multiply(int a, int b) { return a * b; }

// 幾何学計算
double circle_area(double radius) { return M_PI * radius * radius; }

double triangle_area(double base, double height) { return 0.5 * base * height; }

// C++ STL を使った関数
int find_max(const int *arr, int size) {
    if (!arr || size <= 0)
        return 0;
    std::vector<int> vec(arr, arr + size);
    return *std::max_element(vec.begin(), vec.end());
}

int find_min(const int *arr, int size) {
    if (!arr || size <= 0)
        return 0;
    std::vector<int> vec(arr, arr + size);
    return *std::min_element(vec.begin(), vec.end());
}

// ベクトル演算
double dot_product(const double *a, const double *b, int size) {
    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

// 距離計算
double euclidean_distance(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

} // extern "C"
