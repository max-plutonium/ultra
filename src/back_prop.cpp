#include "back_prop.h"

namespace ultra {

// Note that the following are unused,
//
// delta[0]
// weight[0]
// prevDwt[0]

//  I did this intentionally to maintain
//  consistency in numbering the layers.
//  Since for a net having n layers,
//  input layer is referred to as 0th layer,
//  first hidden layer as 1st layer
//  and the nth layer as output layer. And
//  first (0th) layer just stores the inputs
//  hence there is no delta or weight
//  values associated to it.

back_prop::back_prop(const std::vector<std::size_t> &layers, float beta, float alpha)
    : _layers(layers.size()), _learning_rate(beta), _alpha(alpha)
{
    std::uniform_real_distribution<float> distr(-1, 1);
    std::mt19937 engine;

    for(std::size_t i = 0; i < _layers.size(); i++) {
        layer &lay = _layers[i];
        lay._nr_neurons = layers[i];
        lay._outputs.resize(layers[i]);
        lay._deltas.resize(layers[i]);
        lay._weights.resize(layers[i]);
        lay._prev_dwt.resize(layers[i]);

        for(std::size_t j = 0; j < _layers[i]._nr_neurons; j++) {
            lay._weights[j].resize(layers[i - 1] + 1, distr(engine)); // +1 for bias
            lay._prev_dwt[j].resize(layers[i - 1] + 1, 0.0f); // +1 for bias
        }
    }
}

void back_prop::fprop(const std::vector<float> &input)
{
    // assign content to input layer
    for(std::size_t i = 0; i < _layers[0]._nr_neurons; i++)
        _layers[0]._outputs[i] = input[i];

    // assign output(activation) value
    // to each neuron usng sigmoid func

    // For each layer
    for(std::size_t i = 1; i < _layers.size(); i++)
    {
        // For each neuron in current layer
        for(std::size_t j = 0; j < _layers[i]._nr_neurons; j++)
        {
            float sum = 0.0;

            // For input from each neuron in preceding layer
            for(std::size_t k = 0; k < _layers[i - 1]._nr_neurons; k++)
                // Apply weight to inputs and add to sum
                sum += _layers[i - 1]._outputs[k] * _layers[i]._weights[j][k];

            // Apply bias
            sum += _layers[i]._weights[j][_layers[i - 1]._nr_neurons];

            // Apply sigmoid function
            _layers[i]._outputs[j] = sigmoid(sum);
        }
    }
}

void back_prop::bprop(const std::vector<float> &input, const std::vector<float> &target)
{
    fprop(input);

    layer &last_layer = _layers[_layers.size() - 1];

    // The next step is to find the delta for the output layer
    for(std::size_t i = 0; i < last_layer._nr_neurons; i++)
        last_layer._deltas[i] = last_layer._outputs[i]
            * (1 - last_layer._outputs[i]) * (target[i] - last_layer._outputs[i]);

    // then find the delta for the hidden layers
    for(std::size_t i = _layers.size() - 2; i > 0; i--)
    {
        layer &cur_layer = _layers[i];
        layer &next_layer = _layers[i + 1];

        for(std::size_t j = 0; j < cur_layer._nr_neurons; j++)
        {
            float sum = 0.0;

            for(std::size_t k = 0; k < _layers[i + 1]._nr_neurons; k++)
                sum += next_layer._deltas[k] * next_layer._weights[k][j];

            cur_layer._deltas[j] = cur_layer._outputs[j]
                * (1 - cur_layer._outputs[j]) * sum;
        }
    }

    // Apply momentum (does nothing if alpha = 0)
    for(std::size_t i = 1; i < _layers.size(); i++)
    {
        layer &cur_layer = _layers[i];
        layer &prev_layer = _layers[i - 1];

        for(std::size_t j = 0; j < cur_layer._nr_neurons; j++)
        {
            for(std::size_t k = 0; k < prev_layer._nr_neurons; k++)
                cur_layer._weights[j][k] += _alpha * cur_layer._prev_dwt[j][k];

            cur_layer._weights[j][prev_layer._nr_neurons] +=
                    _alpha * cur_layer._prev_dwt[j][prev_layer._nr_neurons];
        }
    }

    // Finally, adjust the weights by finding the correction to the weight.
    // And then apply the correction
    for(std::size_t i = 1; i < _layers.size(); i++)
    {
        layer &cur_layer = _layers[i];
        layer &prev_layer = _layers[i - 1];

        for(std::size_t j = 0; j < cur_layer._nr_neurons; j++)
        {
            for(std::size_t k = 0; k < prev_layer._nr_neurons; k++) {
                cur_layer._prev_dwt[j][k] = _learning_rate
                    * cur_layer._deltas[j] * prev_layer._outputs[k];
                cur_layer._weights[j][k] += cur_layer._prev_dwt[j][k];
            }

            cur_layer._prev_dwt[j][prev_layer._nr_neurons] =
                    _learning_rate * cur_layer._deltas[j];
            cur_layer._weights[j][prev_layer._nr_neurons]
                    += cur_layer._prev_dwt[j][prev_layer._nr_neurons];
        }
    }
}

float back_prop::mse(const std::vector<float> &target)
{
    float mse = 0;

    layer &last_layer = _layers[_layers.size() - 1];

    for(std::size_t i = 0; i < last_layer._nr_neurons; i++)
        mse += std::pow(target[i] - last_layer._outputs[i], 2);

    return mse / 2;
}

float back_prop::out(std::size_t i) const
{
    return _layers[_layers.size() - 1]._outputs[i];
}

std::size_t back_prop::num_inputs() const
{
    return _layers.front()._nr_neurons;
}

std::size_t back_prop::num_outputs() const
{
    return _layers.back()._nr_neurons;
}

ublas::matrix<float> back_prop::weights(std::size_t layer) const
{
    auto vec = _layers[layer]._weights;
    ublas::matrix<float> res(_layers[layer]._nr_neurons, vec.front().size());
    for(std::size_t i = 0; i < vec.size(); i++)
        for(std::size_t j = 0; j < vec[i].size(); j++)
            res(i, j) = vec[i][j];
    return res;
}

bool back_prop::set_weights(std::size_t layer, const ublas::matrix<float> &matrix)
{
    auto vec = _layers[layer]._weights;
    if(matrix.size1() != vec.size() || matrix.size2() != vec.front().size())
        return false;

    for(std::size_t i = 0; i < vec.size(); i++)
        for(std::size_t j = 0; j < vec[i].size(); j++)
            vec[i][j] = matrix(i, j);

    return true;
}

} // namespace ultra
