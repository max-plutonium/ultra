#ifndef BACK_PROP_H
#define BACK_PROP_H

#include "util.h"

namespace ultra {

class back_prop
{
    struct layer
    {
        std::size_t _nr_neurons;

        // output of each neuron
        std::vector<float> _outputs;

        // delta error value for each neuron
        std::vector<float> _deltas;

        // 2-D array to store weights for each neuron
        std::vector<std::vector<float>> _weights;

        // storage for weight-change made in previous epoch
        std::vector<std::vector<float>> _prev_dwt;
    };

    std::vector<layer> _layers;

public:
    // initializes and allocates memory
    explicit back_prop(const std::vector<std::size_t> &layers);

    // feed forwards activations for one set of inputs
    void fprop(const std::vector<float> &input);

    // backpropogates error for one set of input
    void bprop(const std::vector<float> &input, const std::vector<float> &target,
               float learning_rate, float alpha);

    // returns mean square error of the net
    float mse(const std::vector<float> &target);

    void train(const std::vector<std::vector<float>> &train_vectors,
               const std::vector<std::vector<float>> &train_labels, std::size_t num_iter,
               float learning_rate, float alpha, float threshold);

    // returns i'th output of the net
    float out(std::size_t i) const;

    std::size_t num_inputs() const;
    std::size_t num_outputs() const;
    std::size_t num_layers() const;

    ublas::matrix<float> weights(std::size_t layer) const;
    bool set_weights(std::size_t layer, const ublas::matrix<float> &matrix);
};

} // namespace ultra

#endif // BACK_PROP_H