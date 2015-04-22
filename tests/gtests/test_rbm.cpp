#include "../../src/rbm.h"
#include <gmock/gmock.h>

using namespace ultra;

TEST(test_rbm, test)
{
    //auto train_labels = read_mnist_labels("../data/mnist/train-labels-idx1-ubyte", 10);
    auto train_vectors = read_mnist_images("../data/mnist/train-images-idx3-ubyte", 100);
    auto test_labels = read_mnist_labels("../data/mnist/t10k-labels-idx1-ubyte", 100);
    auto test_vectors = read_mnist_images("../data/mnist/t10k-images-idx3-ubyte", 100);

    const float learning_rate = 0.1;
    const std::size_t training_epochs = 5000;
    const std::size_t sampling_iterations = 2;

    const std::size_t train_N = train_vectors.size();
    const std::size_t test_N = test_vectors.size();
    const std::size_t n_visible = train_vectors.front().size();
    const std::size_t n_hidden = 19;

    // construct RBM
    rbm machine(train_N, n_visible, n_hidden);

    auto weights = load_matrix_from_file("../data/rbm-weights.wgt");
    if(weights.size1() == n_hidden && weights.size2() == n_visible)
        machine.set_weights(weights);
    else
        // train
        for(std::size_t epoch = 0; epoch < training_epochs; epoch++)
            for(std::size_t i = 0; i < train_N; i++)
                machine.contrastive_divergence(train_vectors[i], learning_rate, sampling_iterations);

    // test
    for(std::size_t i = 0; i < test_N; i++) {
        auto reconstructed = machine.reconstruct(test_vectors[i]);

        std::cout << std::endl << test_labels[i] << std::endl;

        for(std::size_t line = 0; line < 28; line++) {

            for(std::size_t j = 0; j < 28; j++)
                std::printf("%.0f", test_vectors[i][line * 28 + j]);

            std::cout << " \t ";

            for(std::size_t j = 0; j < 28; j++)
                std::printf("%.0f", reconstructed[line * 28 + j]);

            std::cout << std::endl;
        }

        std::cout << std::endl << std::endl;
    }

    store_matrix_to_file("../data/rbm-weights.wgt", machine.weights());
}
