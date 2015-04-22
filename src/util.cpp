#include "util.h"

#include <fstream>
#include <iterator>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

namespace ultra {

std::vector<float> load_data_from_file(const std::string &file_name)
{
    std::vector<float> vec;
    std::ifstream ifs(file_name.c_str(), std::ios_base::in);

    if(!ifs.is_open())
        return vec;

    std::istream_iterator<float> ii(ifs);
    std::copy(ii, std::istream_iterator<float>(), std::back_inserter(vec));
    ifs.close();
    return vec;
}

ublas::matrix<float> load_matrix_from_file(const std::string &file_name, char line_delim)
{
    std::vector<float> data;
    std::ifstream ifs(file_name.c_str(), std::ios_base::in);

    if(!ifs.is_open())
        return ublas::matrix<float>();

    std::string line;
    std::size_t lines_count = 0;

    while(!ifs.eof()) {
        std::getline(ifs, line, line_delim);
        if(line.empty()) break;
        ++lines_count;
        std::istringstream iss(line);
        std::istream_iterator<float> ii(iss);
        std::copy(ii, std::istream_iterator<float>(), std::back_inserter(data));
    }

    ifs.close();

    const auto line_size = data.size() / lines_count;
    ublas::matrix<float> res(lines_count, line_size);

    for(std::size_t i = 0; i < lines_count; ++i)
        for(std::size_t j = 0; j < line_size; ++j)
            res(i, j) = data[i * line_size + j];

    return res;
}

void store_data_to_file(const std::string &file_name, const std::vector<float> &data,
                        const char *delim)
{
    std::ofstream ofs(file_name.c_str(), std::ios_base::out);
    std::copy(data.cbegin(), data.cend(), std::ostream_iterator<float>(ofs, delim));
    ofs.close();
}

void store_matrix_to_file(const std::string &file_name, const ublas::matrix<float> &data,
                          const char *unit_delim, const char *line_delim)
{
    std::ofstream ofs(file_name.c_str(), std::ios_base::out);
    std::ostream_iterator<float> unit_iter(ofs, unit_delim);

    const auto data_size1 = data.size1();
    const auto data_size2 = data.size2();

    for(std::size_t i = 0; i < data_size1; ++i) {
        for(std::size_t j = 0; j < data_size2; ++j)
            unit_iter = data(i, j);
        ofs << line_delim;
    }

    ofs.close();
}

std::vector<std::vector<float>> load_data_from_dir(const std::string &dir_name, const char *ext)
{
    namespace bfs = boost::filesystem;

    std::vector<bfs::directory_entry> files;
    bfs::directory_iterator rdib(dir_name), rdie;

    std::copy_if(rdib, rdie, std::back_inserter(files),
        [ext](const bfs::directory_entry &entry) {
            return entry.path().extension() == ext;
        });

    std::vector<std::vector<float>> res;
    std::for_each(files.cbegin(), files.cend(), [&res](const bfs::directory_entry &file) {
        res.push_back(load_data_from_file(file.path().string()));
    });

    return res;
}

std::vector<ublas::matrix<float>> load_matrices_from_dir(const std::string &dir_name, const char *ext)
{
    namespace bfs = boost::filesystem;

    std::vector<bfs::directory_entry> files;
    bfs::directory_iterator rdib(dir_name), rdie;

    std::copy_if(rdib, rdie, std::back_inserter(files),
        [ext](const bfs::directory_entry &entry) {
            return entry.path().extension() == ext;
        });

    std::vector<ublas::matrix<float>> res;
    std::for_each(files.cbegin(), files.cend(), [&res](const bfs::directory_entry &file) {
        res.push_back(load_matrix_from_file(file.path().string()));
    });

    return res;
}

std::vector<float> read_mnist_labels(const std::string &file_name, unsigned max_items)
{
    std::ifstream ifs(file_name.c_str(), std::ios_base::in | std::ios_base::binary);

    if(!ifs.is_open())
        return std::vector<float>();

    std::uint32_t magic, nr_items;
    ifs.readsome((char *) &magic, sizeof(magic));
    magic = htobe32(magic);
    ifs.readsome((char *) &nr_items, sizeof(nr_items));
    nr_items = std::min(htobe32(nr_items), max_items);

    std::vector<float> ret;
    ret.reserve(nr_items);

    while(ret.size() < nr_items)
        ret.push_back(ifs.get());

    ifs.close();
    return ret;
}

std::vector<std::vector<float>> read_mnist_images(const std::string &file_name, unsigned max_items)
{
    std::ifstream ifs(file_name.c_str(), std::ios_base::in | std::ios_base::binary);

    if(!ifs.is_open())
        return std::vector<std::vector<float>>();

    std::uint32_t magic, nr_items, rows, columns;
    ifs.readsome((char *) &magic, sizeof(magic));
    magic = htobe32(magic);
    ifs.readsome((char *) &nr_items, sizeof(nr_items));
    nr_items = std::min(htobe32(nr_items), max_items);
    ifs.readsome((char *) &rows, sizeof(rows));
    rows = htobe32(rows);
    ifs.readsome((char *) &columns, sizeof(columns));
    columns = htobe32(columns);

    std::vector<std::vector<float>> ret(nr_items);

    for(std::vector<float> &v : ret) {
        v.resize(rows * columns);

        for(std::size_t i = 0; i < rows * columns; ++i)
            v[i] = (ifs.get() > 0) ? 1 : 0;
    }

    ifs.close();
    return ret;
}

} // namespace ultra
