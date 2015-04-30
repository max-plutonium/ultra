#include "../../src/stacked_rbm.h"
#include <gmock/gmock.h>

using namespace ultra;

TEST(test_stacked_rbm, test)
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

    const std::size_t rbm_training_epochs = 5000;
    const std::size_t num_back_prop_iters = 500000;
    const float rbm_learning_rate = 0.1;
    const float back_prop_learning_rate = 0.1;
    const std::size_t rbm_sampling_iterations = 2;
    const float alpha = 0.1, thresh = 0.00001;

    stacked_rbm machine({100, 13, 7});

    machine.train(train_vectors, train_labels2, rbm_training_epochs,
                  num_back_prop_iters, rbm_learning_rate, back_prop_learning_rate,
                  rbm_sampling_iterations, alpha, thresh);

    for(std::size_t i = 0; i < test_vectors.size(); ++i)
    {
        auto res = machine.predict(test_vectors[i]);

        for(std::size_t i = 0; i < res.size(); ++i)
            std::printf("%.0f", res[i]);

        std::printf(" = %.0f", test_labels[i]);
        std::cout << std::endl;
    }
}
