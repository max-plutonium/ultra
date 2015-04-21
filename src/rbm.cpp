#include "rbm.h"

float ultra::rbm::propup(const std::vector<int> &v, std::size_t i, float b)
{
    float pre_sigmoid_activation = 0.0;

    for(std::size_t j = 0; j < n_visible; j++)
        pre_sigmoid_activation += W(i, j) * v[j];

    pre_sigmoid_activation += b;
    return sigmoid(pre_sigmoid_activation);
}

float ultra::rbm::propdown(const std::vector<int> &h, std::size_t i, float b)
{
    float pre_sigmoid_activation = 0.0;

    for(std::size_t j = 0; j < n_hidden; j++)
        pre_sigmoid_activation += W(j, i) * h[j];

    pre_sigmoid_activation += b;
    return sigmoid(pre_sigmoid_activation);
}

void ultra::rbm::sample_h_given_v(const std::vector<int> &v0_sample,
                                  std::vector<float> &mean, std::vector<int> &sample)
{
    for(std::size_t i = 0; i < n_hidden; i++) {
        mean[i] = propup(v0_sample, i, hbias[i]);
        std::binomial_distribution<> distr(1, mean[i]);
        sample[i] = distr(engine);
    }
}

void ultra::rbm::sample_v_given_h(const std::vector<int> &h0_sample,
                                  std::vector<float> &mean, std::vector<int> &sample)
{
    for(std::size_t i = 0; i < n_visible; i++) {
        mean[i] = propdown(h0_sample, i, vbias[i]);
        std::binomial_distribution<> distr(1, mean[i]);
        sample[i] = distr(engine);
    }
}

void ultra::rbm::gibbs_hvh(const std::vector<int> &h0_sample,
                           std::vector<float> &nv_means,
                           std::vector<int> &nv_samples,
                           std::vector<float> &nh_means,
                           std::vector<int> &nh_samples)
{
    sample_v_given_h(h0_sample, nv_means, nv_samples);
    sample_h_given_v(nv_samples, nh_means, nh_samples);
}

ultra::rbm::rbm(std::size_t size, std::size_t n_v, std::size_t n_h)
    : N(size), n_visible(n_v), n_hidden(n_h), W(n_hidden, n_visible)
    , hbias(n_hidden, 0), vbias(n_visible, 0)
{
    const float a = 1.0f / n_visible;
    std::uniform_real_distribution<float> distr(-a, a);

    for(std::size_t i = 0; i < n_hidden; i++)
        for(std::size_t j = 0; j < n_visible; j++)
            W(i, j) = distr(engine);
}

void ultra::rbm::contrastive_divergence(const std::vector<int> &input,
                                        float lr, int sampling_iterations)
{
    std::vector<float>  ph_mean(n_hidden);
    std::vector<int>    ph_sample(n_hidden);
    std::vector<float>  nv_means(n_visible);
    std::vector<int>    nv_samples(n_visible);
    std::vector<float>  nh_means(n_hidden);
    std::vector<int>    nh_samples(n_hidden);

    /* CD-k */
    sample_h_given_v(input, ph_mean, ph_sample);

    for(int step = 0; step < sampling_iterations; step++)
        if(step == 0)
            gibbs_hvh(ph_sample, nv_means, nv_samples, nh_means, nh_samples);
        else
            gibbs_hvh(nh_samples, nv_means, nv_samples, nh_means, nh_samples);

    for(std::size_t i = 0; i < n_hidden; i++) {
        for(std::size_t j = 0; j < n_visible; j++)
            // W(i, j) += lr * (ph_sample[i] * input[j] - nh_means[i] * nv_samples[j]) / N;
            W(i, j) += lr * (ph_mean[i] * input[j] - nh_means[i] * nv_samples[j]) / N;

        hbias[i] += lr * (ph_sample[i] - nh_means[i]) / N;
    }

    for(std::size_t i = 0; i < n_visible; i++)
        vbias[i] += lr * (input[i] - nv_samples[i]) / N;
}

std::vector<float> ultra::rbm::reconstruct(const std::vector<int> &v)
{
    std::vector<float> h(n_hidden);
    std::vector<float> res;

    float pre_sigmoid_activation = 0.0;

    for(std::size_t i = 0; i < n_hidden; i++)
        h[i] = propup(v, i, hbias[i]);

    for(std::size_t i = 0; i < n_visible; i++) {
        pre_sigmoid_activation = 0.0;

        for(std::size_t j = 0; j < n_hidden; j++)
            pre_sigmoid_activation += W(j, i) * h[j];

        pre_sigmoid_activation += vbias[i];
        res.push_back(sigmoid(pre_sigmoid_activation));
    }

    return res;
}
