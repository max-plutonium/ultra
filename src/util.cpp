#include "util.h"

namespace ultra {

std::vector<float> read_mnist_labels(const std::string &file_name, unsigned max_items)
{
    std::ifstream ifs(file_name.c_str(), std::ios_base::in | std::ios_base::binary);
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
