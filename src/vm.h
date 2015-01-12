#ifndef VM_H
#define VM_H

#include <boost/graph/adjacency_list.hpp>
#include <vector>
#include <list>
#include <random>

#include "port.h"

namespace ultra {

class vm
{
    struct impl;
    impl *d = nullptr;

    struct vertex_property {
        std::size_t _idx;
    };

    using edge_property = boost::property<boost::edge_weight_t, float>;

    using graph_type = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
        vertex_property, edge_property>;

    struct interp {
        graph_type _g;
        std::size_t _out_index;
    };

public:
    std::vector<port_ptr> _inputs, _outputs;
    std::list<interp> _interps;
    std::uniform_int_distribution<int> _distr;
    std::mt19937_64 _generator;

    std::vector<float> load_from_file(const std::string &file_name);
    void store_to_file(const std::string &file_name, const std::vector<float> &vec);

public:
    vm(int argc, const char **argv);
    ~vm();

    static vm *instance();
    void wait_for_done();
    void loop();
    void post_message(port_message);

    void create_field(std::size_t input_size, std::size_t output_size);

    using node_iterator = std::pair<std::list<interp>::iterator, graph_type::vertex_descriptor>;

    node_iterator create_interp(std::size_t in_index, std::size_t out_indices);
    node_iterator create_child_vertex(node_iterator it, std::size_t in_index, float weight = 1.0f);
    void dump(node_iterator it);

private:
    float interp_recursive(node_iterator it);

public:
    void interpret();

    friend struct timed_task;
    friend struct scheduler;
};

} // namespace ultra

#endif // VM_H
