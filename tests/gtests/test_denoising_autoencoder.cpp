#include "../../src/denoising_autoencoder.h"
#include <gmock/gmock.h>

using namespace ultra;

TEST(test_denoising_autoencoder, test)
{
    auto train_vectors = load_data_from_dir("../data/train");
    auto test_labels = std::vector<float> {4};
    auto test_vectors = load_data_from_dir("../data/test");

    const std::size_t training_epochs = 1000;
    const float learning_rate = 0.1;
    const float corruption_level = 0.3;

    const std::size_t n_visible = train_vectors.front().size();
    const std::size_t n_hidden = 10;

    denoising_autoencoder encoder(n_visible, n_hidden);

    encoder.train(train_vectors, training_epochs, learning_rate, corruption_level);

    for(std::size_t i = 0; i < test_vectors.size(); i++)
    {
        auto reconstructed = encoder.reconstruct(test_vectors[i]);

        std::cout << std::endl << test_labels[i] << std::endl;

        for(std::size_t line = 0; line < 10; line++)
        {
            for(std::size_t j = 0; j < 10; j++)
                std::printf("%.0f", test_vectors[i][line * 10 + j]);

            std::cout << " ";

            for(std::size_t j = 0; j < 10; j++)
                std::printf("%.0f", reconstructed[line * 10 + j]);

            std::cout << std::endl;
        }

        std::cout << std::endl << std::endl;
    }
}
