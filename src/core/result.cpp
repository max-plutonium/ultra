#include "result.h"

using namespace ultra::core;

void result_base::deleter::operator()(result_base *r) const
{
    r->_destroyer(r);
}

result_base::result_base(result_base::destroyer d)
    : _destroyer(d)
{
}
