#ifndef GENETIC_H
#define GENETIC_H

#include <bitset>
#include <vector>
#include <random>
#include <deque>

#include "core/action.h"

namespace ultra {

template <std::size_t GeneSize>
float fitness(const std::bitset<GeneSize> &bs) {
    return bs.count();
}

class genetic_engine
{
protected:
    /// Кол-во генов в геноме
    constexpr static std::size_t genome_size = 8;

    /// Кол-во битов в гене
    constexpr static std::size_t gene_size = 8;

public:
    enum selection_type {
        tourney, roulette_wheel
    };

    enum crossing_type {
        one_point, two_point, elementwise
    };

    genetic_engine(ultra::core::action<float (const std::bitset<gene_size> &)> fitness,
                   std::size_t generation_count, selection_type st, crossing_type ct,
                   float mutation_percent = 0.1f);

    const std::bitset<gene_size> &run();

protected:
    // Инициализация
    void generate_init_population();

    // Оценка приспособленности
    std::pair<std::vector<float>, float> estimation();

    // Селекция
    std::deque<std::size_t> selection(const std::pair<std::vector<float>, float> &pair);

    // Скрещивание
    void crossing(std::deque<std::size_t> &indices);

    // Мутация
    void mutate();

private:
    ultra::core::action<float (const std::bitset<gene_size> &)> _fitness;
    std::vector<std::bitset<gene_size>> _genome;
    std::mt19937_64 _generator;
    std::size_t _generation_count;
    float _mut_percent = 0.1f;
    selection_type _sel_type = tourney;
    crossing_type _cross_type = elementwise;
};

} // namespace ultra

#endif // GENETIC_H
