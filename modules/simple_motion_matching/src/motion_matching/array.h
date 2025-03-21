#pragma once

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "motion_matching_api.h"

namespace era_engine
{

    // Basic type representing a pointer to some
    // data and the size of the data.
    template<typename T>
    struct ERA_MOTION_MATCHING_API slice1d
    {
        int size = 0;
        T* data = nullptr;

        slice1d(int _size, T* _data) : size(_size), data(_data) {}

        void zero() { memset((char*)data, 0, sizeof(T) * size); }
        void set(const T& x) { for (int i = 0; i < size; i++) { data[i] = x; } }

        inline T& operator()(int i) const { assert(i >= 0 && i < size); return data[i]; }
    };

    // Same but for a 2d array of data.
    template<typename T>
    struct ERA_MOTION_MATCHING_API slice2d
    {
        int rows = 0, cols = 0;
        T* data = nullptr;

        slice2d(int _rows, int _cols, T* _data) : rows(_rows), cols(_cols), data(_data) {}

        void zero() { memset((char*)data, 0, sizeof(T) * rows * cols); }
        void set(const T& x) { for (int i = 0; i < rows * cols; i++) { data[i] = x; } }

        inline slice1d<T> operator()(int i) const { assert(i >= 0 && i < rows); return slice1d<T>(cols, &data[i * cols]); }
        inline T& operator()(int i, int j) const { assert(i >= 0 && i < rows && j >= 0 && j < cols); return data[i * cols + j]; }
    };

    //--------------------------------------

    // These types are used for the storage of arrays of data.
    // They implicitly cast to slices so can be given directly 
    // as inputs to functions requiring them.
    template<typename T>
    struct ERA_MOTION_MATCHING_API array1d
    {
        int size = 0;
        T* data = nullptr;

        array1d() : size(0), data(NULL) {}
        array1d(int _size) : array1d() { resize(_size); }
        array1d(const slice1d<T>& rhs) : array1d() { resize(rhs.size); memcpy(data, rhs.data, rhs.size * sizeof(T)); }
        array1d(const array1d<T>& rhs) : array1d() { resize(rhs.size); memcpy(data, rhs.data, rhs.size * sizeof(T)); }
        ~array1d() { resize(0); }

        array1d& operator=(const slice1d<T>& rhs) { resize(rhs.size); memcpy(data, rhs.data, rhs.size * sizeof(T)); return *this; };
        array1d& operator=(const array1d<T>& rhs) { resize(rhs.size); memcpy(data, rhs.data, rhs.size * sizeof(T)); return *this; };

        inline T& operator()(int i) const { assert(i >= 0 && i < size); return data[i]; }
        operator slice1d<T>() const { return slice1d<T>(size, data); }

        void zero() { memset(data, 0, sizeof(T) * size); }
        void set(const T& x) { for (int i = 0; i < size; i++) { data[i] = x; } }

        void resize(int _size)
        {
            if (_size == 0 && size != 0)
            {
                free(data);
                data = NULL;
                size = 0;
                return;
            }
            else if (_size > 0 && size == 0)
            {
                data = (T*)malloc(_size * sizeof(T));
                size = _size;
                assert(data != NULL);
            }
            else if (_size > 0 && size > 0 && _size != size)
            {
                data = (T*)realloc(data, _size * sizeof(T));
                size = _size;
                assert(data != NULL);
            }
            zero();
        }
    };

    template<typename T>
    void array1d_write(const array1d<T>& arr, FILE* f)
    {
        fwrite(&arr.size, sizeof(int), 1, f);
        size_t num = fwrite(arr.data, sizeof(T), arr.size, f);
        assert((int)num == arr.size);
    }

    template<typename T>
    void array1d_read(array1d<T>& arr, FILE* f)
    {
        int size;
        fread(&size, sizeof(int), 1, f);
        arr.resize(size);
        size_t num = fread(arr.data, sizeof(T), size, f);
        assert((int)num == size);
    }

    // Similar type but for 2d data
    template<typename T>
    struct ERA_MOTION_MATCHING_API array2d
    {
        int rows = 0, cols = 0;
        T* data = nullptr;

        array2d() : rows(0), cols(0), data(NULL) {}
        array2d(int _rows, int _cols) : array2d() { resize(_rows, _cols); }
        ~array2d() { resize(0, 0); }

        array2d& operator=(const array2d<T>& rhs) { resize(rhs.rows, rhs.cols); memcpy(data, rhs.data, rhs.rows * rhs.cols * sizeof(T)); return *this; };
        array2d& operator=(const slice2d<T>& rhs) { resize(rhs.rows, rhs.cols); memcpy(data, rhs.data, rhs.rows * rhs.cols * sizeof(T)); return *this; };

        inline slice1d<T> operator()(int i) const { assert(i >= 0 && i < rows); return slice1d<T>(cols, &data[i * cols]); }
        inline T& operator()(int i, int j) const { assert(i >= 0 && i < rows && j >= 0 && j < cols); return data[i * cols + j]; }
        operator slice2d<T>() const { return slice2d<T>(rows, cols, data); }

        void zero() { memset(data, 0, sizeof(T) * rows * cols); }
        void set(const T& x) { for (int i = 0; i < rows * cols; i++) { data[i] = x; } }

        void resize(int _rows, int _cols)
        {
            int _size = _rows * _cols;
            int size = rows * cols;

            if (_size == 0 && size != 0)
            {
                free(data);
                data = NULL;
                rows = 0;
                cols = 0;
                return;
            }
            else if (_size > 0 && size == 0)
            {
                data = (T*)malloc(_size * sizeof(T));
                rows = _rows;
                cols = _cols;
                assert(data != NULL);
            }
            else if (_size > 0 && size > 0 && _size != size)
            {
                data = (T*)realloc(data, _size * sizeof(T));
                rows = _rows;
                cols = _cols;
                assert(data != NULL);
            }
            zero();
        }
    };

    template<typename T>
    void array2d_write(const array2d<T>& arr, FILE* f)
    {
        fwrite(&arr.rows, sizeof(int), 1, f);
        fwrite(&arr.cols, sizeof(int), 1, f);
        size_t num = fwrite(arr.data, sizeof(T), arr.rows * arr.cols, f);
        assert((int)num == arr.rows * arr.cols);
    }

    template<typename T>
    void array2d_read(array2d<T>& arr, FILE* f)
    {
        int rows, cols;
        fread(&rows, sizeof(int), 1, f);
        fread(&cols, sizeof(int), 1, f);
        arr.resize(rows, cols);
        size_t num = fread(arr.data, sizeof(T), rows * cols, f);
        assert((int)num == rows * cols);
    }

}