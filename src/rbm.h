#ifndef RBM_H
#define RBM_H

#include <cmath>
#include <fstream>
#include <iterator>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <boost/numeric/ublas/matrix.hpp>

namespace ultra {

namespace ublas = boost::numeric::ublas;

template <typename Tp> constexpr Tp sigmoid(Tp x) {
    return 1.0 / (1.0 + std::exp(-x));
}

template <typename Tp> constexpr Tp sigma(Tp value, float beta = 1.0f) {
    return 1 / (1 + std::exp(-beta * value));
}

template <typename Tp>
std::vector<Tp> load_data_from_file(const std::string &file_name)
{
    std::vector<Tp> vec;
    std::ifstream ifs(file_name.c_str(), std::ios_base::in);
    std::istream_iterator<Tp> ii(ifs);

    std::copy(ii, std::istream_iterator<Tp>(), std::back_inserter(vec));
    ifs.close();
    return vec;
}

template <typename Tp>
ublas::matrix<Tp> load_matrix_from_file(const std::string &file_name, char line_delim = '\n')
{
    std::vector<Tp> data;
    std::ifstream ifs(file_name.c_str(), std::ios_base::in);
    std::string line;

    std::size_t lines_count = 0;
    while(!ifs.eof()) {
        std::getline(ifs, line, line_delim);
        if(line.empty()) break;
        ++lines_count;
        std::istringstream iss(line);
        std::istream_iterator<Tp> ii(iss);
        std::copy(ii, std::istream_iterator<Tp>(), std::back_inserter(data));
    }
    ifs.close();

    const auto line_size = data.size() / lines_count;
    ublas::matrix<Tp> res(lines_count, line_size);

    for(std::size_t i = 0; i < lines_count; ++i)
        for(std::size_t j = 0; j < line_size; ++j)
            res(i, j) = data[i * line_size + j];

    return res;
}

template <typename Tp>
void store_data_to_file(const std::string &file_name, const std::vector<Tp> &data, const char *delim = " ")
{
    std::ofstream ofs(file_name.c_str(), std::ios_base::out);
    std::copy(data.cbegin(), data.cend(), std::ostream_iterator<Tp>(ofs, delim));
    ofs.close();
}

template <typename Tp>
void store_matrix_to_file(const std::string &file_name, const ublas::matrix<Tp> &data,
                          const char *unit_delim = " ", const char *line_delim = "\n")
{
    std::ofstream ofs(file_name.c_str(), std::ios_base::out);
    std::ostream_iterator<Tp> unit_iter(ofs, unit_delim);

    const auto data_size1 = data.size1();
    const auto data_size2 = data.size2();

    for(std::size_t i = 0; i < data_size1; ++i) {
        for(std::size_t j = 0; j < data_size2; ++j)
            unit_iter = data(i, j);
        ofs << line_delim;
    }

    ofs.close();
}

template <typename Tp>
std::vector<std::vector<Tp>> load_data_from_dir(const std::string &dir_name = ".",
                                                const char *ext = ".txt")
{
    namespace bfs = boost::filesystem;

    std::vector<bfs::directory_entry> files;
    bfs::directory_iterator rdib(dir_name), rdie;

    std::copy_if(rdib, rdie, std::back_inserter(files),
        [ext](const bfs::directory_entry &entry) {
            return entry.path().extension() == ext;
        });

    std::vector<std::vector<Tp>> res;
    std::for_each(files.cbegin(), files.cend(), [&res](const bfs::directory_entry &file) {
        res.push_back(load_data_from_file<Tp>(file.path().string()));
    });

    return res;
}

template <typename Tp>
std::vector<ublas::matrix<Tp>> load_matrixes_from_dir(
        const std::string &dir_name = ".", const char *ext = ".txt")
{
    namespace bfs = boost::filesystem;

    std::vector<bfs::directory_entry> files;
    bfs::directory_iterator rdib(dir_name), rdie;

    std::copy_if(rdib, rdie, std::back_inserter(files),
        [ext](const bfs::directory_entry &entry) {
            return entry.path().extension() == ext;
        });

    std::vector<ublas::matrix<Tp>> res;
    std::for_each(files.cbegin(), files.cend(), [&res](const bfs::directory_entry &file) {
        res.push_back(load_matrix_from_file<Tp>(file.path().string()));
    });

    return res;
}

class rbm
{
    const std::size_t N, n_visible, n_hidden;
    ublas::matrix<float> W;
    std::vector<float> hbias, vbias;
    std::mt19937 engine;

    float propup(const std::vector<int> &v, std::size_t i, float b);
    float propdown(const std::vector<int> &h, std::size_t i, float b);

    /// Семплирование скрытого слоя при данном видимом
    void sample_h_given_v(const std::vector<int> &v0_sample,
                          std::vector<float> &mean, std::vector<int> &sample);

    /// Семплирование видимого слоя при данном скрытом
    void sample_v_given_h(const std::vector<int> &h0_sample,
                          std::vector<float> &mean, std::vector<int> &sample);

    void gibbs_hvh(const std::vector<int> &h0_sample,
                   std::vector<float> &nv_means,
                   std::vector<int> &nv_samples,
                   std::vector<float> &nh_means,
                   std::vector<int> &nh_samples);

public:
    rbm(std::size_t size, std::size_t n_v, std::size_t n_h);

    void contrastive_divergence(const std::vector<int> &input,
                                float lr, int sampling_iterations);

    std::vector<float> reconstruct(const std::vector<int> &v);
};

} // namespace ultra

#endif // RBM_H
