#ifndef DENOISING_AUTOENCODER_H
#define DENOISING_AUTOENCODER_H

#include "util.h"

namespace ultra {

class denoising_autoencoder
{
    const std::size_t _nr_visible, _nr_hidden;
    ublas::matrix<float> _weights;
    std::vector<float> _hbias, _vbias;
    mutable std::mt19937 _engine;

    std::vector<float> get_corrupted_input(const std::vector<float> &input, float p) const;
    std::vector<float> get_hidden_values(const std::vector<float> &input) const;
    std::vector<float> get_reconstructed_input(const std::vector<float> &y) const;
    void train_one(const std::vector<float> &input, float learning_rate,
                   float corruption_level, std::size_t train_size);

public:
    denoising_autoencoder(std::size_t visible_size, std::size_t hidden_size);

    void train(const std::vector<std::vector<float>> &train_vectors,
               std::size_t training_epochs, float learning_rate, float corruption_level);

    std::vector<float> reconstruct(const std::vector<float> &input) const;
};

} // namespace ultra

#endif // DENOISING_AUTOENCODER_H
