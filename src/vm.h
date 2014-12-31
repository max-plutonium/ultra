#ifndef VM_H
#define VM_H

#include "core/action.h"
#include "message.h"

extern "C" {

void uvm_init(int argc, char **argv);
bool uvm_post_action(ultra::core::action<void ()>);

} // extern "C"

#endif // VM_H
