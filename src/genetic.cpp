#include <algorithm>

#include "genetic.h"

namespace ultra {

genome::genome(std::size_t genome_size)
    : _genome_size(genome_size), _genome(genome_size)
{
}

const gene_t &genome::select_best(const core::action<float (const gene_t &)> &fitness,
    std::size_t generation_count, selection_type st, crossing_type ct, float mutation_percent)
{
    std::size_t best_index = 0;

    do {
        auto estim = estimation(fitness);

        std::size_t max_count = 0;
        for(std::size_t i = 0; i < _genome_size; ++i) {
            std::size_t count = std::count(estim.first.cbegin(), estim.first.cend(), i);
            if(count > max_count) {
                max_count = count;
                best_index = i;
            }
        }

        auto indices = selection(st, estim);

        std::deque<std::pair<std::size_t, std::size_t>> pairs;
        while(!indices.empty()) {
            std::size_t cur_idx = indices.back();
            indices.pop_back();
            std::uniform_int_distribution<std::size_t> distr(0, indices.size() - 1);
            std::size_t pos = distr(_generator);
            std::size_t idx_for_pairing = indices.at(pos);
            indices.erase(indices.begin() + pos);
            pairs.emplace_back(cur_idx, idx_for_pairing);
        }

        crossing(ct, pairs);
        mutate(mutation_percent);

    } while(--generation_count);

    return _genome[best_index];
}

const std::deque<gene_t> &genome::chromosomes() const
{
    return _genome;
}

void genome::init_random()
{
    std::uniform_int_distribution<std::size_t> distr(0, 1 << 7);
    std::generate(_genome.begin(), _genome.end(), [&] {
        return distr(_generator);
    });
}

std::pair<std::deque<float>, float>
genome::estimation(const core::action<float (const gene_t &)> &fitness)
{
    std::deque<float> fitness_results(_genome_size);
    float summ = 0;
    for(std::size_t i = 0; i < fitness_results.size(); ++i) {
        fitness_results[i] = fitness(_genome[i]);
        summ += fitness_results[i];
    }
    return std::make_pair(fitness_results, summ);
}

std::deque<std::size_t> genome::selection(selection_type sel_type,
        const std::pair<std::deque<float>, float> &pair)
{
    auto &fitness_results = pair.first;
    auto summ = pair.second;
    std::deque<std::size_t> indices;

    if(sel_type == tourney) {
        std::uniform_int_distribution<std::size_t> distr(0, _genome_size - 1);
        for(std::size_t i = 0; i < _genome_size; ++i) {
            std::size_t idx1 = distr(_generator);
            std::size_t idx2 = distr(_generator);
            if(fitness_results[idx1] > fitness_results[idx2])
                indices.push_back(idx1);
            else
                indices.push_back(idx2);
        }
    }
    else if(sel_type == roulette_wheel) {
        std::vector<float> likelihoods(_genome_size);
        std::vector<float> wheel(_genome_size);
        for(std::size_t i = 0; i < fitness_results.size(); ++i) {
            likelihoods[i] = fitness_results[i] / summ;
            if(i == 0)
                wheel[i] = likelihoods[i];
            else
                wheel[i] = wheel[i - 1] + likelihoods[i];
        }

        std::vector<float> selectors(_genome_size);
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

void genome::crossing(crossing_type cross_type,
    const std::deque<std::pair<std::size_t, std::size_t> > &pairs)
{
    if(cross_type == one_point) {
        std::deque<gene_t> new_genome;
        std::uniform_int_distribution<std::size_t> distr(0, 7);
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

        _genome = std::move(new_genome);
    }
    else if(cross_type == two_point) {
        std::deque<gene_t> new_genome;
        std::uniform_int_distribution<std::size_t> distr(0, 7);
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

        _genome = std::move(new_genome);
    }
    else if(cross_type == elementwise) {
        std::uniform_int_distribution<std::size_t> distr(0, 1 << 7);
        for(std::pair<std::size_t, std::size_t> pair : pairs) {
            std::size_t mask = distr(_generator);
            std::size_t swap_mask = (_genome[pair.first] ^ _genome[pair.second]).to_ullong() & mask;
            _genome[pair.first] ^= swap_mask;
            _genome[pair.second] ^= swap_mask;
        }
    }
}

void genome::mutate(float mutation_percent)
{
    std::uniform_real_distribution<float> distr(0, 1);
    std::uniform_int_distribution<std::size_t> distr2(0, 7);
    for(gene_t &gene : _genome)
        if(distr(_generator) < mutation_percent)
            gene.flip(distr2(_generator));
}

} // namespace ultra


