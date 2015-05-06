#include "stacked_denoising_autoencoder.h"

namespace ultra {

stacked_denoising_autoencoder::stacked_denoising_autoencoder(
        const std::vector<std::size_t> &layers)
    : _bp(layers)
{
    if(layers.size() < 2)
        throw std::logic_error("layers must be greater than 1");

    // Последний слой - без DA, только back_prop
    const std::size_t layers_size = layers.size() - 2;
    _encoders.reserve(layers_size);

    for(std::size_t i = 0; i < layers_size; ++i)
        _encoders.emplace_back(layers[i], layers[i + 1]);
}

void stacked_denoising_autoencoder::train(
        const std::vector<std::vector<float>> &train_vectors,
        const std::vector<std::vector<float>> &train_targets,
        std::size_t da_training_epochs,
        std::size_t num_back_prop_iters,
        float da_learning_rate,
        float back_prop_learning_rate,
        float da_corruption_level,
        float alpha, float threshold)
{
    auto local_train_vectors = train_vectors;

    for(std::size_t i = 0; i < _encoders.size(); ++i) {
        denoising_autoencoder &enc = _encoders[i];
        enc.train(local_train_vectors, da_training_epochs,
                  da_learning_rate, da_corruption_level);

        std::transform(local_train_vectors.cbegin(),
                       local_train_vectors.cend(),
                       local_train_vectors.begin(),
                       [&enc](const std::vector<float> &entry) {
            return enc.compute_hiddens(entry);
        });

        _bp.set_weights(i, enc.weights());
    }

    _bp.train(train_vectors, train_targets, num_back_prop_iters,
              back_prop_learning_rate, alpha, threshold);
}

std::vector<float> stacked_denoising_autoencoder::predict(const std::vector<float> &input)
{
    std::vector<float> res(_bp.num_outputs());

    _bp.fprop(input);

    for(std::size_t i = 0; i < _bp.num_outputs(); ++i)
        res[i] = _bp.out(i);

    return res;
}

} // namespace ultra
