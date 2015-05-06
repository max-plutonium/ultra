#ifndef STACKED_DENOISING_AUTOENCODER_H
#define STACKED_DENOISING_AUTOENCODER_H

#include "denoising_autoencoder.h"
#include "back_prop.h"

namespace ultra {

class stacked_denoising_autoencoder
{
    std::vector<denoising_autoencoder> _encoders;
    back_prop _bp;

public:
    stacked_denoising_autoencoder(const std::vector<std::size_t> &layers);

    void train(const std::vector<std::vector<float>> &train_vectors,
               const std::vector<std::vector<float>> &train_targets,
               std::size_t da_training_epochs,
               std::size_t num_back_prop_iters,
               float da_learning_rate,
               float back_prop_learning_rate,
               float da_corruption_level,
               float alpha, float threshold);

    std::vector<float> predict(const std::vector<float> &input);
};

} // namespace ultra

#endif // STACKED_DENOISING_AUTOENCODER_H
