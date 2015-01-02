#ifndef LOGIC_TIME_H
#define LOGIC_TIME_H

#include <iostream>

namespace ultra {

struct logic_time
{
    virtual bool advance() = 0;
};

class scalar_time : public logic_time
{
    std::size_t _time;

    // logic_time interface
public:
    explicit scalar_time(std::size_t t = 0);
    virtual bool advance() override;
    std::size_t time() const;
    void merge(const scalar_time &other);

    bool operator==(const scalar_time &o) const;
    bool operator!=(const scalar_time &o) const;

    friend std::ostream &operator<<(std::ostream &o, const scalar_time &msg);
    friend std::istream &operator>>(std::istream &i, scalar_time &msg);
};

std::ostream &operator<<(std::ostream &o, const scalar_time &msg);
std::istream &operator>>(std::istream &i, scalar_time &msg);

} // namespace ultra

#endif // LOGIC_TIME_H
