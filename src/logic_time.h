#ifndef LOGIC_TIME_H
#define LOGIC_TIME_H

#include <iostream>
#include <vector>

namespace ultra {

/*!
 * \brief Интерфейс логического времени
 */
struct logic_time
{
    /*!
     * \brief Продвигает логическое время вперед
     */
    virtual void advance() = 0;
};

/*!
 * \brief Класс скалярного времени
 */
class scalar_time : public logic_time
{
    std::size_t _time;

public:
    explicit scalar_time(std::size_t t = 0);
    virtual void advance() override;
    std::size_t time() const;
    void merge(const scalar_time &other);

    bool operator==(const scalar_time &o) const;
    bool operator!=(const scalar_time &o) const;
    bool operator<=(const scalar_time &o) const;
    bool operator<(const scalar_time &o) const;

    friend std::ostream &operator<<(std::ostream &o, const scalar_time &msg);
    friend std::istream &operator>>(std::istream &i, scalar_time &msg);
};

std::ostream &operator<<(std::ostream &o, const scalar_time &msg);
std::istream &operator>>(std::istream &i, scalar_time &msg);

/*!
 * \brief Класс векторного времени
 */
class vector_time : public logic_time
{
    std::vector<std::size_t> _time;
    std::size_t _idx;

public:
    explicit vector_time(std::size_t index, std::size_t count);
    virtual void advance() override;
    std::vector<std::size_t> time() const;
    void merge(const vector_time &other);

    bool operator==(const vector_time &o) const;
    bool operator!=(const vector_time &o) const;
    bool operator<=(const vector_time &o) const;
    bool operator<(const vector_time &o) const;

    friend std::ostream &operator<<(std::ostream &o, const vector_time &msg);
    friend std::istream &operator>>(std::istream &i, vector_time &msg);
};

std::ostream &operator<<(std::ostream &o, const vector_time &msg);
std::istream &operator>>(std::istream &i, vector_time &msg);

} // namespace ultra

#endif // LOGIC_TIME_H
