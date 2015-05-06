#include "../../src/stacked_denoising_autoencoder.h"
#include <gmock/gmock.h>

using namespace ultra;

TEST(test_stacked_denoising_autoencoder, test)
{
    auto train_labels = std::vector<float> {0, 1, 2, 3, 4, 5, 6};
    auto train_vectors = load_data_from_dir("../data/train");
    auto test_labels = std::vector<float> {4};
    auto test_vectors = load_data_from_dir("../data/test");

    std::vector<std::vector<float>> train_labels2(train_labels.size());

    for(std::size_t i = 0; i < train_labels.size(); ++i) {
        train_labels2[i].resize(10, 0);
        train_labels2[i][train_labels[i]] = 1;
    }

    const std::size_t da_training_epochs = 1000;
    const std::size_t num_back_prop_iters = 500000;
    const float da_learning_rate = 0.1;
    const float back_prop_learning_rate = 0.1;
    const float da_corruption_level = 0.3;
    const float alpha = 0.1, thresh = 0.00001;

    stacked_denoising_autoencoder encoder({100, 13, 7});

    encoder.train(train_vectors, train_labels2, da_training_epochs,
                  num_back_prop_iters, da_learning_rate, back_prop_learning_rate,
                  da_corruption_level, alpha, thresh);

    for(std::size_t i = 0; i < test_vectors.size(); ++i)
    {
        auto res = encoder.predict(test_vectors[i]);

        for(std::size_t i = 0; i < res.size(); ++i)
            std::printf("%.0f", res[i]);

        std::printf(" = %.0f", test_labels[i]);
        std::cout << std::endl;
    }
}
