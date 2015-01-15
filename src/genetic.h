#ifndef GENETIC_H
#define GENETIC_H

#include <bitset>
#include <deque>
#include <random>

#include "core/action.h"

namespace ultra {

using gene_t = std::bitset<8>;

class genome
{
public:
    enum selection_type {
        tourney, roulette_wheel
    };

    enum crossing_type {
        one_point, two_point, elementwise
    };

    explicit genome(std::size_t genome_size);

    const gene_t &select_best(const core::action<float (const gene_t &)> &fitness,
        std::size_t generation_count, selection_type st,
        crossing_type ct, float mutation_percent = 0.1f);

    const std::deque<gene_t> &chromosomes() const;

    // Инициализация
    void init_random();

protected:
    // Оценка приспособленности
    std::pair<std::deque<float>, float>
        estimation(const core::action<float (const gene_t &)> &fitness);

    // Селекция
    std::deque<std::size_t> selection(selection_type sel_type,
        const std::pair<std::deque<float>, float> &pair);

    // Скрещивание
    void crossing(crossing_type cross_type,
            const std::deque<std::pair<std::size_t, std::size_t>> &pairs);

    // Мутация
    void mutate(float mutation_percent);

private:
    std::size_t _genome_size;
    std::deque<gene_t> _genome;
    std::mt19937_64 _generator;
};

} // namespace ultra

#endif // GENETIC_H
