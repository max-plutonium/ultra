#include "rbm.h"

namespace ultra {

float rbm::propup(const std::vector<int> &v, std::size_t i, float b) const
{
    float pre_sigmoid_activation = 0.0;

    for(std::size_t j = 0; j < _nr_visible; j++)
        pre_sigmoid_activation += _weights(i, j) * v[j];

    pre_sigmoid_activation += b;
    return sigmoid(pre_sigmoid_activation);
}

float rbm::propdown(const std::vector<int> &h, std::size_t i, float b) const
{
    float pre_sigmoid_activation = 0.0;

    for(std::size_t j = 0; j < _nr_hidden; j++)
        pre_sigmoid_activation += _weights(j, i) * h[j];

    pre_sigmoid_activation += b;
    return sigmoid(pre_sigmoid_activation);
}

void rbm::sample_h_given_v(const std::vector<int> &v0_sample,
                           std::vector<float> &mean, std::vector<int> &sample)
{
    for(std::size_t i = 0; i < _nr_hidden; i++) {
        mean[i] = propup(v0_sample, i, _hbias[i]);
        std::binomial_distribution<> distr(1, mean[i]);
        sample[i] = distr(_engine);
    }
}

void rbm::sample_v_given_h(const std::vector<int> &h0_sample,
                           std::vector<float> &mean, std::vector<int> &sample)
{
    for(std::size_t i = 0; i < _nr_visible; i++) {
        mean[i] = propdown(h0_sample, i, _vbias[i]);
        std::binomial_distribution<> distr(1, mean[i]);
        sample[i] = distr(_engine);
    }
}

void rbm::gibbs_hvh(const std::vector<int> &h0_sample,
                    std::vector<float> &nv_means,
                    std::vector<int> &nv_samples,
                    std::vector<float> &nh_means,
                    std::vector<int> &nh_samples)
{
    sample_v_given_h(h0_sample, nv_means, nv_samples);
    sample_h_given_v(nv_samples, nh_means, nh_samples);
}

rbm::rbm(std::size_t size, std::size_t n_v, std::size_t n_h)
    : _size(size), _nr_visible(n_v), _nr_hidden(n_h), _weights(_nr_hidden, _nr_visible)
    , _hbias(_nr_hidden, 0), _vbias(_nr_visible, 0)
{
    const float a = 1.0f / _nr_visible;
    std::uniform_real_distribution<float> distr(-a, a);

    for(std::size_t i = 0; i < _nr_hidden; i++)
        for(std::size_t j = 0; j < _nr_visible; j++)
            _weights(i, j) = distr(_engine);
}

void rbm::contrastive_divergence(const std::vector<int> &input,
                                 float learning_rate, std::size_t sampling_iterations)
{
    std::vector<float>  ph_mean(_nr_hidden);
    std::vector<int>    ph_sample(_nr_hidden);
    std::vector<float>  nv_means(_nr_visible);
    std::vector<int>    nv_samples(_nr_visible);
    std::vector<float>  nh_means(_nr_hidden);
    std::vector<int>    nh_samples(_nr_hidden);

    /* CD-k */
    sample_h_given_v(input, ph_mean, ph_sample);

    for(std::size_t step = 0; step < sampling_iterations; step++)
        if(step == 0)
            gibbs_hvh(ph_sample, nv_means, nv_samples, nh_means, nh_samples);
        else
            gibbs_hvh(nh_samples, nv_means, nv_samples, nh_means, nh_samples);

    for(std::size_t i = 0; i < _nr_hidden; i++) {
        for(std::size_t j = 0; j < _nr_visible; j++)
            // _weights(i, j) += lr * (ph_sample[i] * input[j] - nh_means[i] * nv_samples[j]) / _size;
            _weights(i, j) += learning_rate * (ph_mean[i] * input[j] - nh_means[i] * nv_samples[j]) / _size;

        _hbias[i] += learning_rate * (ph_sample[i] - nh_means[i]) / _size;
    }

    for(std::size_t i = 0; i < _nr_visible; i++)
        _vbias[i] += learning_rate * (input[i] - nv_samples[i]) / _size;
}

std::vector<float> rbm::reconstruct(const std::vector<int> &v) const
{
    std::vector<float> h(_nr_hidden);
    std::vector<float> res;

    for(std::size_t i = 0; i < _nr_hidden; i++)
        h[i] = propup(v, i, _hbias[i]);

    for(std::size_t i = 0; i < _nr_visible; i++) {

        float pre_sigmoid_activation = 0.0;

        for(std::size_t j = 0; j < _nr_hidden; j++)
            pre_sigmoid_activation += _weights(j, i) * h[j];

        pre_sigmoid_activation += _vbias[i];
        res.push_back(sigmoid(pre_sigmoid_activation));
    }

    return res;
}

ublas::matrix<float> rbm::weights() const
{
    return _weights;
}

} // namespace ultra
