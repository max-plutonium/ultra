#include "../../src/rbm.h"
#include <gmock/gmock.h>

#include <boost/lexical_cast.hpp>

using namespace ultra;

TEST(test_rbm, create)
{
    auto train_vectors = load_data_from_dir<int>("train");
    auto test_vectors = load_data_from_dir<int>("data");

    const float learning_rate = 0.1;
    const std::size_t training_epochs = 1000;
    const std::size_t sampling_iterations = 2;

    const std::size_t train_N = train_vectors.size();
    const std::size_t test_N = test_vectors.size();
    const std::size_t n_visible = train_vectors.front().size();
    const std::size_t n_hidden = 3;

    // construct RBM
    rbm machine(train_N, n_visible, n_hidden);

    // train
    for(std::size_t epoch = 0; epoch < training_epochs; epoch++)
        for(std::size_t i = 0; i < train_N; i++)
            machine.contrastive_divergence(train_vectors[i], learning_rate, sampling_iterations);

    // test
    for(std::size_t i = 0; i < test_N; i++) {
        auto reconstructed = machine.reconstruct(test_vectors[i]);

        for(std::size_t j = 0; j < n_visible; j++) {
            if(j % 10 == 0)
                std::cout << std::endl;
            std::printf("%.0f ", reconstructed[j]);
        }
        std::cout << std::endl;

        store_data_to_file("out/" + boost::lexical_cast<std::string>(i) + ".txt", reconstructed);
    }

    store_matrix_to_file("out/weights.wgt", machine.weights());
}
