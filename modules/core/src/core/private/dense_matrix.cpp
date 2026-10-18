#include "core/dense_matrix.h"

namespace era_engine
{
    VectorX::VectorX(uint32 size, float value)
        : values(size, value)
    {
    }

    void VectorX::resize(uint32 size, float value)
    {
        values.assign(size, value);
    }

    void VectorX::set_vec3(uint32 index, const vec3& value)
    {
        ASSERT(index + 3 <= size());

        values[index + 0] = value.x;
        values[index + 1] = value.y;
        values[index + 2] = value.z;
    }

    MatrixX::MatrixX(uint32 rows, uint32 cols, float value)
        : values((size_t)rows * cols, value), rows_count(rows), cols_count(cols)
    {
    }

    void MatrixX::resize(uint32 rows, uint32 cols, float value)
    {
        values.assign((size_t)rows * cols, value);
        rows_count = rows;
        cols_count = cols;
    }

    void MatrixX::set_column_vec3(uint32 row, uint32 col, const vec3& value)
    {
        ASSERT(row + 3 <= rows_count && col < cols_count);

        (*this)(row + 0, col) = value.x;
        (*this)(row + 1, col) = value.y;
        (*this)(row + 2, col) = value.z;
    }

    void MatrixX::add_to_diagonal(float value)
    {
        const uint32 count = std::min(rows_count, cols_count);
        for (uint32 i = 0; i < count; ++i)
        {
            (*this)(i, i) += value;
        }
    }

    void multiply_transposed(const MatrixX& a, const MatrixX& b, MatrixX& out)
    {
        ASSERT(a.cols() == b.cols() && &out != &a && &out != &b);

        const uint32 rows = a.rows();
        const uint32 cols = b.rows();
        const uint32 inner = a.cols();
        const bool symmetric = &a == &b;

        out.resize(rows, cols);
        for (uint32 i = 0; i < rows; ++i)
        {
            for (uint32 j = 0; j < (symmetric ? i + 1 : cols); ++j)
            {
                float sum = 0.0f;
                for (uint32 k = 0; k < inner; ++k)
                {
                    sum += a(i, k) * b(j, k);
                }
                out(i, j) = sum;
                if (symmetric)
                {
                    out(j, i) = sum;
                }
            }
        }
    }

    void multiply(const MatrixX& a, const VectorX& v, VectorX& out)
    {
        ASSERT(a.cols() == v.size() && &out != &v);

        out.resize(a.rows());
        for (uint32 i = 0; i < a.rows(); ++i)
        {
            const float* row = a.row(i);

            float sum = 0.0f;
            for (uint32 j = 0; j < a.cols(); ++j)
            {
                sum += row[j] * v[j];
            }
            out[i] = sum;
        }
    }

    void transpose_multiply(const MatrixX& a, const VectorX& v, VectorX& out)
    {
        ASSERT(a.rows() == v.size() && &out != &v);

        out.resize(a.cols());
        for (uint32 i = 0; i < a.rows(); ++i)
        {
            for (uint32 j = 0; j < a.cols(); ++j)
            {
                out[j] += a(i, j) * v[i];
            }
        }
    }

    bool solve_cholesky(MatrixX& a, VectorX& b)
    {
        ASSERT(a.rows() == a.cols() && a.rows() == b.size());

        const uint32 n = a.rows();

        // a = L * L^T, L in the lower triangle.
        for (uint32 i = 0; i < n; ++i)
        {
            for (uint32 j = 0; j <= i; ++j)
            {
                float sum = a(i, j);
                for (uint32 k = 0; k < j; ++k)
                {
                    sum -= a(i, k) * a(j, k);
                }

                if (i == j)
                {
                    if (sum <= 0.0f)
                    {
                        return false;
                    }
                    a(i, i) = std::sqrt(sum);
                }
                else
                {
                    a(i, j) = sum / a(j, j);
                }
            }
        }

        // L * y = b, then L^T * x = y.
        for (uint32 i = 0; i < n; ++i)
        {
            float sum = b[i];
            for (uint32 k = 0; k < i; ++k)
            {
                sum -= a(i, k) * b[k];
            }
            b[i] = sum / a(i, i);
        }

        for (uint32 i = n; i-- > 0;)
        {
            float sum = b[i];
            for (uint32 k = i + 1; k < n; ++k)
            {
                sum -= a(k, i) * b[k];
            }
            b[i] = sum / a(i, i);
        }
        return true;
    }
}
