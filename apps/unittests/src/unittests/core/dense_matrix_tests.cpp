#include <gtest/gtest.h>

#include <core/dense_matrix.h>

namespace
{
	using namespace era_engine;

	MatrixX make_matrix(uint32 rows, uint32 cols, std::initializer_list<float> values)
	{
		MatrixX matrix(rows, cols);
		uint32 index = 0;
		for (float value : values)
		{
			matrix(index / cols, index % cols) = value;
			++index;
		}
		return matrix;
	}
}

TEST(Core_DenseMatrix, Construction)
{
	MatrixX matrix(2, 3, 1.5f);

	EXPECT_EQ(matrix.rows(), 2u);
	EXPECT_EQ(matrix.cols(), 3u);
	EXPECT_FLOAT_EQ(matrix(1, 2), 1.5f);

	matrix(1, 2) = 4.0f;
	EXPECT_FLOAT_EQ(matrix.row(1)[2], 4.0f);
	EXPECT_FLOAT_EQ(matrix.data()[5], 4.0f);

	matrix.resize(3, 3);
	EXPECT_EQ(matrix.rows(), 3u);
	EXPECT_EQ(matrix.cols(), 3u);

	VectorX vector(4, 2.0f);
	EXPECT_EQ(vector.size(), 4u);
	EXPECT_FLOAT_EQ(vector[3], 2.0f);
}

TEST(Core_DenseMatrix, AddToDiagonal)
{
	MatrixX matrix(3, 3, 1.0f);
	matrix.add_to_diagonal(2.0f);

	for (uint32 i = 0; i < 3; ++i)
	{
		for (uint32 j = 0; j < 3; ++j)
		{
			EXPECT_FLOAT_EQ(matrix(i, j), i == j ? 3.0f : 1.0f);
		}
	}
}

TEST(Core_DenseMatrix, MultiplyTransposed)
{
	const MatrixX a = make_matrix(2, 3, { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f });
	const MatrixX b = make_matrix(2, 3, { 1.0f, 0.0f, -1.0f, 2.0f, 1.0f, 0.0f });

	MatrixX result;
	multiply_transposed(a, b, result);

	ASSERT_EQ(result.rows(), 2u);
	ASSERT_EQ(result.cols(), 2u);
	EXPECT_FLOAT_EQ(result(0, 0), -2.0f);
	EXPECT_FLOAT_EQ(result(0, 1), 4.0f);
	EXPECT_FLOAT_EQ(result(1, 0), -2.0f);
	EXPECT_FLOAT_EQ(result(1, 1), 13.0f);
}

TEST(Core_DenseMatrix, MatrixVectorProducts)
{
	const MatrixX a = make_matrix(2, 3, { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f });

	VectorX v(3);
	v[0] = 1.0f;
	v[1] = -1.0f;
	v[2] = 2.0f;

	VectorX result;
	multiply(a, v, result);

	ASSERT_EQ(result.size(), 2u);
	EXPECT_FLOAT_EQ(result[0], 5.0f);
	EXPECT_FLOAT_EQ(result[1], 11.0f);

	VectorX w(2);
	w[0] = 1.0f;
	w[1] = 2.0f;

	VectorX transposed_result;
	transpose_multiply(a, w, transposed_result);

	ASSERT_EQ(transposed_result.size(), 3u);
	EXPECT_FLOAT_EQ(transposed_result[0], 9.0f);
	EXPECT_FLOAT_EQ(transposed_result[1], 12.0f);
	EXPECT_FLOAT_EQ(transposed_result[2], 15.0f);
}

TEST(Core_DenseMatrix, SolveCholesky)
{
	const MatrixX a = make_matrix(3, 3, {
		4.0f, 1.0f, 2.0f,
		1.0f, 5.0f, 3.0f,
		2.0f, 3.0f, 6.0f });

	VectorX expected(3);
	expected[0] = 1.0f;
	expected[1] = -2.0f;
	expected[2] = 0.5f;

	VectorX b;
	multiply(a, expected, b);

	MatrixX factorized = a;
	ASSERT_TRUE(solve_cholesky(factorized, b));

	for (uint32 i = 0; i < 3; ++i)
	{
		EXPECT_NEAR(b[i], expected[i], 1e-4f);
	}
}

TEST(Core_DenseMatrix, SolveCholeskyRejectsIndefiniteMatrix)
{
	MatrixX a = make_matrix(2, 2, { 1.0f, 2.0f, 2.0f, 1.0f });
	VectorX b(2, 1.0f);

	EXPECT_FALSE(solve_cholesky(a, b));
}
