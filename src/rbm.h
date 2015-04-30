#ifndef RBM_H
#define RBM_H

#include "util.h"

namespace ultra {

class rbm
{
    const std::size_t _nr_visible, _nr_hidden;
    ublas::matrix<float> _weights;
    std::vector<float> _hbias, _vbias;
    std::mt19937 _engine;

    float propup(const std::vector<float> &v, std::size_t i, float b) const;
    float propdown(const std::vector<int> &h, std::size_t i, float b) const;

    /// Семплирование скрытого слоя при данном видимом
    void sample_h_given_v(const std::vector<float> &v0_sample,
                          std::vector<float> &mean, std::vector<int> &sample);

    /// Семплирование видимого слоя при данном скрытом
    void sample_v_given_h(const std::vector<int> &h0_sample,
                          std::vector<float> &mean, std::vector<float> &sample);

    void gibbs_hvh(const std::vector<int> &h0_sample,
                   std::vector<float> &nv_means,
                   std::vector<float> &nv_samples,
                   std::vector<float> &nh_means,
                   std::vector<int> &nh_samples);

    void contrastive_divergence(const std::vector<float> &input, float learning_rate,
                                std::size_t sampling_iterations, std::size_t train_size);

public:
    rbm(std::size_t visible_size, std::size_t hidden_size);

    void train(const std::vector<std::vector<float>> &train_vectors, std::size_t training_epochs,
               float learning_rate, std::size_t sampling_iterations);

    std::vector<float> reconstruct(const std::vector<float> &input) const;
    std::vector<float> compute_hiddens(const std::vector<float> &input) const;

    ublas::matrix<float> weights() const;
    void set_weights(const ublas::matrix<float> &matrix);
};

} // namespace ultra

#endif // RBM_H
