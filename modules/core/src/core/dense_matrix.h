// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/math.h"

#include <vector>

namespace era_engine
{
    class ERA_CORE_API VectorX
    {
    public:
        VectorX() = default;
        explicit VectorX(uint32 size, float value = 0.0f);

        // All elements are set to the value.
        void resize(uint32 size, float value = 0.0f);

        uint32 size() const { return (uint32)values.size(); }

        float& operator[](uint32 index) { return values[index]; }
        float operator[](uint32 index) const { return values[index]; }

        float* data() { return values.data(); }
        const float* data() const { return values.data(); }

        // Elements index, index + 1 and index + 2.
        void set_vec3(uint32 index, const vec3& value);

    private:
        std::vector<float> values;
    };

    class ERA_CORE_API MatrixX
    {
    public:
        MatrixX() = default;
        MatrixX(uint32 rows, uint32 cols, float value = 0.0f);

        void resize(uint32 rows, uint32 cols, float value = 0.0f);

        uint32 rows() const { return rows_count; }
        uint32 cols() const { return cols_count; }

        float& operator()(uint32 row, uint32 col) { return values[(size_t)row * cols_count + col]; }
        float operator()(uint32 row, uint32 col) const { return values[(size_t)row * cols_count + col]; }

        float* row(uint32 index) { return &values[(size_t)index * cols_count]; }
        const float* row(uint32 index) const { return &values[(size_t)index * cols_count]; }

        float* data() { return values.data(); }
        const float* data() const { return values.data(); }

        // Elements (row, col), (row + 1, col) and (row + 2, col).
        void set_column_vec3(uint32 row, uint32 col, const vec3& value);

        void add_to_diagonal(float value);

    private:
        std::vector<float> values;
        uint32 rows_count = 0;
        uint32 cols_count = 0;
    };

    ERA_CORE_API void multiply_transposed(const MatrixX& a, const MatrixX& b, MatrixX& out);

    ERA_CORE_API void multiply(const MatrixX& a, const VectorX& v, VectorX& out);

    ERA_CORE_API void transpose_multiply(const MatrixX& a, const VectorX& v, VectorX& out);

    // Solves a * x = b for a symmetric positive definite a by the Cholesky factorization.
    ERA_CORE_API bool solve_cholesky(MatrixX& a, VectorX& b);
}
