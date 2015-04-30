#include "stacked_rbm.h"

namespace ultra {

stacked_rbm::stacked_rbm(const std::vector<std::size_t> &layers)
    : _bp(layers)
{
    if(layers.size() < 2)
        throw std::logic_error("layers must be greater than 1");

    // Последний слой - без RBM, только back_prop
    const std::size_t layers_size = layers.size() - 2;
    _rbms.reserve(layers_size);

    for(std::size_t i = 0; i < layers_size; ++i)
        _rbms.emplace_back(layers[i], layers[i + 1]);
}

void stacked_rbm::train(const std::vector<std::vector<float>> &train_vectors,
                        const std::vector<std::vector<float>> &train_targets,
                        std::size_t rbm_training_epochs,
                        std::size_t num_back_prop_iters,
                        float rbm_learning_rate,
                        float back_prop_learning_rate,
                        std::size_t rbm_sampling_iterations,
                        float alpha, float threshold)
{
    auto local_train_vectors = train_vectors;

    for(std::size_t i = 0; i < _rbms.size(); ++i) {
        rbm &m = _rbms[i];
        m.train(local_train_vectors, rbm_training_epochs,
                rbm_learning_rate, rbm_sampling_iterations);

        std::transform(local_train_vectors.cbegin(),
                       local_train_vectors.cend(),
                       local_train_vectors.begin(),
                       [&m](const std::vector<float> &entry) {
            return m.compute_hiddens(entry);
        });

        _bp.set_weights(i, m.weights());
    }

    _bp.train(train_vectors, train_targets, num_back_prop_iters,
              back_prop_learning_rate, alpha, threshold);
}

std::vector<float> stacked_rbm::predict(const std::vector<float> &input)
{
    std::vector<float> res(_bp.num_outputs());

    _bp.fprop(input);

    for(std::size_t i = 0; i < _bp.num_outputs(); ++i)
        res[i] = _bp.out(i);

    return res;
}

} // namespace ultra
