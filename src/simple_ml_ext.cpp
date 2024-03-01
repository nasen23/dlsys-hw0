#include <algorithm>
#include <numeric>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <cmath>
#include <iostream>
#include <vector>

namespace py = pybind11;


void matmul(const float *A, const float *B, float *dest, size_t i, size_t j, size_t k, bool transpose = false) {
    for (size_t ii = 0; ii < i; ii++) {
        for (size_t jj = 0; jj < j; jj++) {
            for (size_t kk = 0; kk < k; kk++) {
                size_t idx = transpose ? jj * i + ii : ii * j + jj;
                dest[ii * k + kk] += A[idx] * B[jj * k + kk];
            }
        }
    }
}


void softmax_regression_epoch_cpp(const float *X, const unsigned char *y,
								  float *theta, size_t m, size_t n, size_t k,
								  float lr, size_t batch)
{
    /**
     * A C++ version of the softmax regression epoch code.  This should run a
     * single epoch over the data defined by X and y (and sizes m,n,k), and
     * modify theta in place.  Your function will probably want to allocate
     * (and then delete) some helper arrays to store the logits and gradients.
     *
     * Args:
     *     X (const float *): pointer to X data, of size m*n, stored in row
     *          major (C) format
     *     y (const unsigned char *): pointer to y data, of size m
     *     theta (float *): pointer to theta data, of size n*k, stored in row
     *          major (C) format
     *     m (size_t): number of examples
     *     n (size_t): input dimension
     *     k (size_t): number of classes
     *     lr (float): learning rate / SGD step size
     *     batch (int): SGD minibatch size
     *
     * Returns:
     *     (None)
     */

    /// BEGIN YOUR CODE
    for (size_t i = 0; i < m; i += batch) {
        auto size = std::min(m - i, batch);
        auto X_batch = X + i * n;
        auto y_batch = y + i;
        std::vector<float> Y(size * k, 0);
        matmul(X_batch, theta, Y.data(), size, n, k);
        std::transform(Y.begin(), Y.end(), Y.begin(), expf);
        for (size_t j = 0; j < size; j++) {
            auto sum = std::accumulate(Y.begin() + j * k, Y.begin() + (j + 1) * k, 0.0f);
            std::transform(Y.begin() + j * k, Y.begin() + (j + 1) * k, Y.begin() + j * k, [sum](float x) {return x / sum;});
        }
        auto Z = std::move(Y); // size * k
        for (size_t j = 0; j < size; j++) {
            Z[j * k + y_batch[j]] -= 1;
        }
        std::vector<float> g(n * k, 0);
        matmul(X_batch, Z.data(), g.data(), n, size, k, true);
        std::transform(g.begin(), g.end(), g.begin(), [size](float x) {return x / (float) size;});
        std::transform(g.begin(), g.end(), theta, theta, [lr](float gradient, float weight) {return weight - lr * gradient;});
    }
    /// END YOUR CODE
}


/**
 * This is the pybind11 code that wraps the function above.  It's only role is
 * wrap the function above in a Python module, and you do not need to make any
 * edits to the code
 */
PYBIND11_MODULE(simple_ml_ext, m) {
    m.def("softmax_regression_epoch_cpp",
    	[](py::array_t<float, py::array::c_style> X,
           py::array_t<unsigned char, py::array::c_style> y,
           py::array_t<float, py::array::c_style> theta,
           float lr,
           int batch) {
        softmax_regression_epoch_cpp(
        	static_cast<const float*>(X.request().ptr),
            static_cast<const unsigned char*>(y.request().ptr),
            static_cast<float*>(theta.request().ptr),
            X.request().shape[0],
            X.request().shape[1],
            theta.request().shape[1],
            lr,
            batch
           );
    },
    py::arg("X"), py::arg("y"), py::arg("theta"),
    py::arg("lr"), py::arg("batch"));
}
