#ifndef UTIL_H
#define UTIL_H

#include <cmath>

#include <boost/numeric/ublas/matrix.hpp>

namespace ultra {

namespace ublas = boost::numeric::ublas;

template <typename Tp>
constexpr Tp sigmoid(Tp value, float beta = 1.0f) {
    return 1.0 / (1.0 + std::exp(-beta * value));
}

std::vector<float> load_data_from_file(const std::string &file_name);

ublas::matrix<float> load_matrix_from_file(const std::string &file_name, char line_delim = '\n');

void store_data_to_file(const std::string &file_name, const std::vector<float> &data,
                        const char *delim = " ");

void store_matrix_to_file(const std::string &file_name, const ublas::matrix<float> &data,
                          const char *unit_delim = " ", const char *line_delim = "\n");

std::vector<std::vector<float>> load_data_from_dir(
        const std::string &dir_name = ".", const char *ext = ".txt");

std::vector<ublas::matrix<float>> load_matrices_from_dir(
        const std::string &dir_name = ".", const char *ext = ".txt");

std::vector<float> read_mnist_labels(const std::string &file_name, unsigned max_items = -1);

std::vector<std::vector<float>> read_mnist_images(const std::string &file_name, unsigned max_items = -1);

} // namespace ultra

#endif // UTIL_H
