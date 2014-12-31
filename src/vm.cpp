#include "vm.h"
#include "core/thread_pool.h"
#include "core/grid.h"

#include <shared_mutex>
#include <unordered_map>
#include <boost/program_options.hpp>
#include <iostream>

namespace {

using namespace ultra;

core::thread_pool<core::prio_scheduler> s_pool;

std::shared_timed_mutex s_grid_lock;
//core::grid<

} // namespace

namespace po = boost::program_options;

void uvm_init(int argc, char **argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("compression", po::value<int>(), "set compression level")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
    }

    if (vm.count("compression")) {
        std::cout << "Compression level was set to "
                  << vm["compression"].as<int>() << ".\n";
    } else {
        std::cout << "Compression level was not set.\n";
    }
}

bool uvm_post_action(ultra::core::action<void ()> action)
{
    s_pool.execute_callable(std::move(action));
    return true;
}
