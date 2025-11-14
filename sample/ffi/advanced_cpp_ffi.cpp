// Advanced C++ library demonstrating complex features
// This shows how to integrate sophisticated C++ code with Cb via FFI

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Complex C++ class using modern features
class DataAnalyzer {
  private:
    std::vector<double> data_;
    std::map<std::string, double> statistics_;

  public:
    void add_data(double value) { data_.push_back(value); }

    void compute_statistics() {
        if (data_.empty())
            return;

        // Use STL algorithms
        double sum = 0.0;
        for (auto v : data_)
            sum += v;
        double mean = sum / data_.size();

        auto min_it = std::min_element(data_.begin(), data_.end());
        auto max_it = std::max_element(data_.begin(), data_.end());

        statistics_["mean"] = mean;
        statistics_["min"] = *min_it;
        statistics_["max"] = *max_it;
        statistics_["count"] = static_cast<double>(data_.size());

        // Compute standard deviation
        double variance = 0.0;
        for (auto v : data_) {
            double diff = v - mean;
            variance += diff * diff;
        }
        statistics_["stddev"] = std::sqrt(variance / data_.size());
    }

    double get_statistic(const std::string &name) {
        auto it = statistics_.find(name);
        return (it != statistics_.end()) ? it->second : 0.0;
    }

    void clear() {
        data_.clear();
        statistics_.clear();
    }
};

// Global instance (or use opaque pointer pattern for better memory management)
static std::unique_ptr<DataAnalyzer> g_analyzer;

// Utility functions demonstrating templates (must be outside extern "C")
template <typename T> T generic_max(T a, T b) { return (a > b) ? a : b; }

// C API wrapper for FFI
extern "C" {

// Initialize the analyzer
void analyzer_init() { g_analyzer = std::make_unique<DataAnalyzer>(); }

// Add a data point
void analyzer_add(double value) {
    if (g_analyzer) {
        g_analyzer->add_data(value);
    }
}

// Compute statistics
void analyzer_compute() {
    if (g_analyzer) {
        g_analyzer->compute_statistics();
    }
}

// Get a specific statistic
double analyzer_get_stat(int stat_type) {
    if (!g_analyzer)
        return 0.0;

    switch (stat_type) {
    case 0:
        return g_analyzer->get_statistic("mean");
    case 1:
        return g_analyzer->get_statistic("min");
    case 2:
        return g_analyzer->get_statistic("max");
    case 3:
        return g_analyzer->get_statistic("count");
    case 4:
        return g_analyzer->get_statistic("stddev");
    default:
        return 0.0;
    }
}

// Clear all data
void analyzer_clear() {
    if (g_analyzer) {
        g_analyzer->clear();
    }
}

// Cleanup
void analyzer_destroy() { g_analyzer.reset(); }

// Wrapper functions using templates
int max_int(int a, int b) { return generic_max(a, b); }

double max_double(double a, double b) { return generic_max(a, b); }

// Function using lambda - simplified to supported signature
double add_values(double x, double y) {
    auto add = [](double a, double b) { return a + b; };
    return add(x, y);
}

double multiply_values(double x, double y) {
    auto multiply = [](double a, double b) { return a * b; };
    return multiply(x, y);
}

// Vector operations using STL
double vector_sum(double *values, int size) {
    std::vector<double> vec(values, values + size);
    double sum = 0.0;
    for (auto v : vec)
        sum += v;
    return sum;
}

double vector_average(double *values, int size) {
    if (size == 0)
        return 0.0;
    return vector_sum(values, size) / size;
}

// Sorting demonstration
void sort_array(double *values, int size, int ascending) {
    std::vector<double> vec(values, values + size);

    if (ascending) {
        std::sort(vec.begin(), vec.end());
    } else {
        std::sort(vec.begin(), vec.end(), std::greater<double>());
    }

    // Copy back
    std::copy(vec.begin(), vec.end(), values);
}

// Exception handling (safe for C boundary)
int safe_divide(int a, int b) {
    try {
        if (b == 0) {
            throw std::runtime_error("Division by zero");
        }
        return a / b;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1; // Error indicator
    }
}

// Memory management demonstration
void *allocate_buffer(int size) { return new char[size]; }

void free_buffer(void *buffer) { delete[] static_cast<char *>(buffer); }
}
