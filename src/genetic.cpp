#include <algorithm>

#include "genetic.h"

namespace ultra {

genetic_engine::genetic_engine(core::action<float (const std::bitset<gene_size> &)> fitness,
    std::size_t generation_count, selection_type st, crossing_type ct, float mutation_percent)
    : _fitness(std::move(fitness)), _genome(genome_size)
    , _generation_count(generation_count), _mut_percent(mutation_percent)
    , _sel_type(st), _cross_type(ct)
{
}

const std::bitset<genetic_engine::gene_size> &genetic_engine::run()
{
    generate_init_population();
    std::size_t best_index = 0;

    do {
        auto estim = estimation();

        std::size_t max_count = 0;
        for(std::size_t i = 0; i < genome_size; ++i) {
            std::size_t count = std::count(estim.first.cbegin(), estim.first.cend(), i);
            if(count > max_count) {
                max_count = count;
                best_index = i;
            }
        }

        auto indices = selection(estim);
        crossing(indices);
        mutate();

    } while(--_generation_count);

    return _genome[best_index];
}

void genetic_engine::generate_init_population()
{
    std::uniform_int_distribution<std::size_t> distr(0, 1 << gene_size);
    std::generate(_genome.begin(), _genome.end(), [&] {
        return distr(_generator);
    });
}

std::pair<std::vector<float>, float> genetic_engine::estimation()
{
    std::vector<float> fitness_results(genome_size);
    float summ = 0;
    for(std::size_t i = 0; i < fitness_results.size(); ++i) {
        fitness_results[i] = _fitness(_genome[i]);
        summ += fitness_results[i];
    }
    return std::make_pair(fitness_results, summ);
}

std::deque<std::size_t> genetic_engine::selection(
        const std::pair<std::vector<float>, float> &pair)
{
    auto &fitness_results = pair.first;
    auto summ = pair.second;
    std::deque<std::size_t> indices;

    if(_sel_type == tourney) {
        std::uniform_int_distribution<std::size_t> distr(0, genome_size - 1);
        for(std::size_t i = 0; i < genome_size; ++i) {
            std::size_t idx1 = distr(_generator);
            std::size_t idx2 = distr(_generator);
            if(fitness_results[idx1] > fitness_results[idx2])
                indices.push_back(idx1);
            else
                indices.push_back(idx2);
        }
    }
    else if(_sel_type == roulette_wheel) {
        std::vector<float> likelihoods(genome_size);
        std::vector<float> wheel(genome_size);
        for(std::size_t i = 0; i < fitness_results.size(); ++i) {
            likelihoods[i] = fitness_results[i] / summ;
            if(i == 0)
                wheel[i] = likelihoods[i];
            else
                wheel[i] = wheel[i - 1] + likelihoods[i];
        }

        std::vector<float> selectors(genome_size);
        std::uniform_real_distribution<float> distr(0, 1);
        std::generate(selectors.begin(), selectors.end(), [&] {
            return distr(_generator);
        });

        for(float selector : selectors) {
            for(std::size_t i = 0; i < wheel.size(); ++i) {
                if(i == 0 && selector <= wheel[1]) {
                    indices.push_back(i);
                    break;
                } else if((selector > wheel[i - 1]) &&
                          (selector <= wheel[i])) {
                    indices.push_back(i);
                    break;
                }
            }
        }
    }

    return indices;
}

void genetic_engine::crossing(std::deque<std::size_t> &indices)
{
    std::vector<std::pair<std::size_t, std::size_t>> pairs;
    while(!indices.empty()) {
        std::size_t cur_idx = indices.back();
        indices.pop_back();
        std::uniform_int_distribution<std::size_t> distr(0, indices.size() - 1);
        std::size_t pos = distr(_generator);
        std::size_t idx_for_pairing = indices.at(pos);
        indices.erase(indices.begin() + pos);
        pairs.emplace_back(cur_idx, idx_for_pairing);
    }

    if(_cross_type == one_point) {
        std::vector<std::bitset<gene_size>> new_genome;
        std::uniform_int_distribution<std::size_t> distr(0, gene_size - 1);
        for(std::pair<std::size_t, std::size_t> pair : pairs) {
            std::string str1 = _genome.at(pair.first).to_string();
            std::string str2 = _genome.at(pair.second).to_string();
            std::size_t locus = distr(_generator);

            auto str3 = str1.substr(0, locus);
            auto str4 = str1.substr(locus);
            auto str5 = str2.substr(0, locus);
            auto str6 = str2.substr(locus);

            new_genome.emplace_back(str3 + str6);
            new_genome.emplace_back(str5 + str4);
        }

        _genome = new_genome;
    }
    else if(_cross_type == two_point) {
        std::vector<std::bitset<gene_size>> new_genome;
        std::uniform_int_distribution<std::size_t> distr(0, gene_size - 1);
        for(std::pair<std::size_t, std::size_t> pair : pairs) {
            std::string str1 = _genome.at(pair.first).to_string();
            std::string str2 = _genome.at(pair.second).to_string();
            std::size_t locus1 = distr(_generator);
            std::size_t locus2 = distr(_generator);

            auto str3 = str1.substr(0, std::min(locus1, locus2));
            auto str4 = str1.substr(std::min(locus1, locus2), std::max(locus1, locus2));
            auto str5 = str1.substr(std::max(locus1, locus2));

            auto str6 = str2.substr(0, std::min(locus1, locus2));
            auto str7 = str2.substr(std::min(locus1, locus2), std::max(locus1, locus2));
            auto str8 = str2.substr(std::max(locus1, locus2));

            new_genome.emplace_back(str3 + str7 + str5);
            new_genome.emplace_back(str6 + str4 + str8);
        }

        _genome = new_genome;
    }
    else if(_cross_type == elementwise) {
        std::uniform_int_distribution<std::size_t> distr(0, 1 << gene_size);
        for(std::pair<std::size_t, std::size_t> pair : pairs) {
            std::size_t mask = distr(_generator);
            std::size_t swap_mask = (_genome[pair.first] ^ _genome[pair.second]).to_ulong() & mask;
            _genome[pair.first] ^= swap_mask;
            _genome[pair.second] ^= swap_mask;
        }
    }
}

void genetic_engine::mutate()
{
    std::uniform_real_distribution<float> distr(0, 1);
    std::uniform_int_distribution<std::size_t> distr2(0, gene_size - 1);
    for(std::bitset<gene_size> &gene : _genome)
        if(distr(_generator) < _mut_percent)
            gene.flip(distr2(_generator));
}

} // namespace ultra


