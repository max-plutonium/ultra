#include "denoising_autoencoder.h"

namespace ultra {

std::vector<float> denoising_autoencoder::get_corrupted_input(
        const std::vector<float> &input, float p) const
{
    std::vector<float> res(_nr_visible, 0);
    std::binomial_distribution<> distr(1, p);

    for(std::size_t i = 0; i < _nr_visible; i++)
        if(input[i] != 0)
            res[i] = distr(_engine);

    return res;
}

std::vector<float> denoising_autoencoder::get_hidden_values(
        const std::vector<float> &input) const
{
    std::vector<float> res(_nr_hidden, 0);

    for(std::size_t i = 0; i < _nr_hidden; i++)
    {
        for(std::size_t j = 0; j < _nr_visible; j++)
            res[i] += _weights(i, j) * input[j];

        res[i] = sigmoid(res[i] + _hbias[i]);
    }

    return res;
}

std::vector<float> denoising_autoencoder::get_reconstructed_input(
        const std::vector<float> &y) const
{
    std::vector<float> res(_nr_visible, 0);

    for(std::size_t i = 0; i < _nr_visible; i++)
    {
        for(std::size_t j = 0; j < _nr_hidden; j++)
            res[i] += _weights(j, i) * y[j];

        res[i] = sigmoid(res[i] + _vbias[i]);
    }

    return res;
}

void denoising_autoencoder::train_one(const std::vector<float> &input,
        float learning_rate, float corruption_level, std::size_t train_size)
{
    std::vector<float> l_vbias(_nr_visible);
    std::vector<float> l_hbias(_nr_hidden);

    const float p = 1 - corruption_level;

    std::vector<float> tilde_x = get_corrupted_input(input, p);
    std::vector<float> y = get_hidden_values(tilde_x);
    std::vector<float> z = get_reconstructed_input(y);

    // vbias
    for(std::size_t i = 0; i < _nr_visible; i++) {
        l_vbias[i] = input[i] - z[i];
        _vbias[i] += learning_rate * l_vbias[i] / train_size;
    }

    // hbias
    for(std::size_t i = 0; i < _nr_hidden; i++)
    {
        l_hbias[i] = 0;

        for(std::size_t j = 0; j < _nr_visible; j++)
            l_hbias[i] += _weights(i, j) * l_vbias[j];

        l_hbias[i] *= y[i] * (1 - y[i]);
        _hbias[i] += learning_rate * l_hbias[i] / train_size;
    }

    // weights
    for(std::size_t i = 0; i < _nr_hidden; i++)
        for(std::size_t j = 0; j < _nr_visible; j++)
            _weights(i, j) += learning_rate * (l_hbias[i] * tilde_x[j] + l_vbias[j] * y[i]) / train_size;
}

denoising_autoencoder::denoising_autoencoder(std::size_t visible_size, std::size_t hidden_size)
    : _nr_visible(visible_size), _nr_hidden(hidden_size)
    , _weights(_nr_hidden, _nr_visible), _hbias(_nr_hidden, 0), _vbias(_nr_visible, 0)
{
    const float a = 1.0f / _nr_visible;
    std::uniform_real_distribution<float> distr(-a, a);

    for(std::size_t i = 0; i < _nr_hidden; i++)
        for(std::size_t j = 0; j < _nr_visible; j++)
            _weights(i, j) = distr(_engine);
}

void denoising_autoencoder::train(const std::vector<std::vector<float>> &train_vectors,
        std::size_t training_epochs, float learning_rate, float corruption_level)
{
    const std::size_t train_vectors_size = train_vectors.size();

    for(std::size_t epoch = 0; epoch < training_epochs; ++epoch)
        for(std::size_t i = 0; i < train_vectors_size; ++i)
            train_one(train_vectors[i], learning_rate,
                      corruption_level, train_vectors_size);
}

std::vector<float> denoising_autoencoder::reconstruct(const std::vector<float> &input) const
{
    std::vector<float> y = get_hidden_values(input);
    return get_reconstructed_input(y);
}

} // namespace ultra
