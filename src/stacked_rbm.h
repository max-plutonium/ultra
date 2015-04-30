#ifndef STACKED_RBM_H
#define STACKED_RBM_H

#include "rbm.h"
#include "back_prop.h"

namespace ultra {

class stacked_rbm
{
    std::vector<rbm> _rbms;
    back_prop _bp;

public:
    stacked_rbm(const std::vector<std::size_t> &layers);

    void train(const std::vector<std::vector<float>> &train_vectors,
               const std::vector<std::vector<float>> &train_targets,
               std::size_t rbm_training_epochs,
               std::size_t num_back_prop_iters,
               float rbm_learning_rate,
               float back_prop_learning_rate,
               std::size_t rbm_sampling_iterations,
               float alpha, float threshold);

    std::vector<float> predict(const std::vector<float> &input);
};

} // namespace ultra

#endif // STACKED_RBM_H
