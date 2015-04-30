#include "../../src/back_prop.h"
#include <gmock/gmock.h>

using namespace ultra;

TEST(test_back_prop, test)
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

    const std::size_t num_iter = 500000;
    const float learning_rate = 0.1, alpha = 0.1, thresh = 0.00001;

    back_prop bp({10 * 10, 7});

    EXPECT_EQ(10 * 10, bp.num_inputs());
    EXPECT_EQ(7, bp.num_outputs());

    bp.train(train_vectors, train_labels2, num_iter, learning_rate, alpha, thresh);

    for(std::size_t i = 0; i < test_vectors.size(); ++i)
    {
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
}
