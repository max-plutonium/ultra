#ifndef ULTRA_H
#define ULTRA_H

#include "ultra_global.h"
#include "task.h"
#include "address.h"
#include <vector>

namespace ultra {

class task;

struct executor
{
    virtual ~executor() = default;
    virtual void execute(std::shared_ptr<task>) = 0;
};

using exec_ptr = std::shared_ptr<executor>;

} // namespace ultra

#endif // ULTRA_H
