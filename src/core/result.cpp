#include "result.h"

using namespace ultra::core;

void details::result_base::Deleter::operator()(details::result_base *r) const
{
    r->_destroyer(r);
}

details::result_base::result_base(details::result_base::Destroyer d)
    : _destroyer(d)
{
}
