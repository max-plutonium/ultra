#include "../../src/back_prop.h"
#include <gmock/gmock.h>

#include <boost/lexical_cast.hpp>

using namespace ultra;

TEST(test_back_prop, test)
{
    auto train_labels = read_mnist_labels("../data/mnist/train-labels-idx1-ubyte");
    auto train_vectors = read_mnist_images("../data/mnist/train-images-idx3-ubyte");
    auto test_labels = read_mnist_labels("../data/mnist/t10k-labels-idx1-ubyte", 100);
    auto test_vectors = read_mnist_images("../data/mnist/t10k-images-idx3-ubyte", 100);

    std::vector<std::vector<float>> train_labels2(train_labels.size());

    for(std::size_t i = 0; i < train_labels.size(); ++i) {
        train_labels2[i].resize(10, 0);
        train_labels2[i][train_labels[i]] = 1;
    }

    const float learning_rate = 0.1, alpha = 0.1, thresh = 0.00001;
    const std::size_t num_iter = 500000;

    back_prop bp({28 * 28, 47, 23, 10}, learning_rate, alpha);

    EXPECT_EQ(28 * 28, bp.num_inputs());
    EXPECT_EQ(10, bp.num_outputs());

    bool b = true;
    for(std::size_t i = 0; i < bp.num_layers(); ++i)
        b &= bp.set_weights(i, load_matrix_from_file("../data/bp-weights" +
                                    boost::lexical_cast<std::string>(i) + ".wgt"));

    if(!b)
        for (std::size_t i = 0; i < num_iter; i++)
        {
            bp.bprop(train_vectors[i % train_vectors.size()],
                     train_labels2[i % train_labels2.size()]);

            if(bp.mse(train_labels2[i % train_labels2.size()]) < thresh) {
                std::cout << std::endl << "Network Trained. Threshold value achieved in "
                          << i << " iterations." << std::endl;
                std::cout << "MSE: " << bp.mse(train_labels2[i % train_labels2.size()])
                          << std::endl << std::endl;
                break;
            }
            if (i % (num_iter / 10) == 0)
                std::cout << std::endl << "MSE: "
                          << bp.mse(train_labels2[i % train_labels2.size()])
                          << "... Training..." << std::endl;
        }

    std::cout << "Now using the trained network to make predctions on test data...."
              << std::endl << std::endl;

    for(std::size_t i = 0; i < test_vectors.size(); ++i) {
        bp.fprop(test_vectors[i]);

        for(std::size_t i = 0; i < bp.num_outputs(); ++i)
            std::printf("%.0f", bp.out(i));

        std::printf(" = %.0f", test_labels[i]);
        std::cout << std::endl;
    }

    auto mtx = bp.weights(0);
    EXPECT_TRUE(bp.set_weights(0, mtx));
    EXPECT_TRUE(std::equal(mtx.data().begin(), mtx.data().end(),
                           bp.weights(0).data().begin()));

    for(std::size_t i = 0; i < bp.num_layers(); ++i)
        store_matrix_to_file("../data/bp-weights" +
                             boost::lexical_cast<std::string>(i) +
                             ".wgt", bp.weights(i));
}
