#pragma once

#ifndef __LNI_fast_vector
#define __LNI_fast_vector

#include <cstddef>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <utility>

#define LNI_fast_vector_MAX_SZ 1000000000
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

namespace lni {

    template <typename T>
    class fast_vector {
    public:
        // types:
        typedef T value_type;
        typedef T &reference;
        typedef const T &const_reference;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef T *iterator;
        typedef const T *const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef ptrdiff_t difference_type;
        typedef unsigned int size_type;

        // 23.3.11.2, construct/copy/destroy:
        fast_vector() noexcept;
        explicit fast_vector(size_type n);
        fast_vector(size_type n, const T &val);
        fast_vector(typename fast_vector<T>::iterator first, typename fast_vector<T>::iterator last);
        fast_vector(std::initializer_list<T>);
        fast_vector(const fast_vector<T> &);
        fast_vector(fast_vector<T> &&) noexcept;
        ~fast_vector();
        fast_vector<T> &operator=(const fast_vector<T> &);
        fast_vector<T> &operator=(fast_vector<T> &&);
        fast_vector<T> &operator=(std::initializer_list<T>);
        void assign(size_type, const T &value);
        void assign(typename fast_vector<T>::iterator, typename fast_vector<T>::iterator);
        void assign(std::initializer_list<T>);

        // iterators:
        iterator begin() noexcept;
        const_iterator cbegin() const noexcept;
        iterator end() noexcept;
        const_iterator cend() const noexcept;
        reverse_iterator rbegin() noexcept;
        const_reverse_iterator crbegin() const noexcept;
        reverse_iterator rend() noexcept;
        const_reverse_iterator crend() const noexcept;

        // 23.3.11.3, capacity:
        bool empty() const noexcept;
        size_type size() const noexcept;
        size_type max_size() const noexcept;
        size_type capacity() const noexcept;
        size_type header_size() const noexcept;
        void resize(size_type);
        void resize(size_type, const T &);
        void reserve(size_type);
        void shrink_to_fit();

        // element access
        reference operator[](size_type);
        const_reference operator[](size_type) const;
        reference at(size_type);
        const_reference at(size_type) const;
        reference front();
        const_reference front() const;
        reference back();
        const_reference back() const;
        T *header() noexcept;
        T *header() const noexcept;

        // 23.3.11.4, data access:
        T *data() noexcept;
        const T *data() const noexcept;

        // 23.3.11.5, modifiers:
        template <class... Args>
        void emplace_back(Args &&...args);
        void push_back(const T &);
        void push_back(T &&);
        void pop_back();
        void apply_offset(int offset);

        template <class... Args>
        iterator emplace(const_iterator, Args &&...);
        iterator insert(const_iterator, const T &);
        iterator insert(const_iterator, T &&);
        iterator insert(const_iterator, size_type, const T &);
        template <class InputIt>
        iterator insert(const_iterator, InputIt, InputIt);
        iterator insert(const_iterator, std::initializer_list<T>);
        iterator erase(const_iterator);
        iterator erase(const_iterator, const_iterator);
        void swap(fast_vector<T> &);
        void clear() noexcept;

        bool operator==(const fast_vector<T> &) const;
        bool operator!=(const fast_vector<T> &) const;
        bool operator<(const fast_vector<T> &) const;
        bool operator<=(const fast_vector<T> &) const;
        bool operator>(const fast_vector<T> &) const;
        bool operator>=(const fast_vector<T> &) const;

    private:
        size_type rsrv_sz = 4;
        size_type vec_sz = 0;
        T *arr;
        int arr_offset = 0;

        inline void reallocate();
    };

    template <typename T>
    fast_vector<T>::fast_vector() noexcept
    {
        arr = new T[rsrv_sz];
    }

    template <typename T>
    fast_vector<T>::fast_vector(typename fast_vector<T>::size_type n)
    {
        size_type i;
        rsrv_sz = n << 2;
        arr = new T[rsrv_sz];
        for (i = 0; i < n; ++i)
            arr[i] = T();
        vec_sz = n;
    }

    template <typename T>
    fast_vector<T>::fast_vector(typename fast_vector<T>::size_type n, const T &value)
    {
        size_type i;
        rsrv_sz = n << 2;
        arr = new T[rsrv_sz];
        for (i = 0; i < n; ++i)
            arr[i] = value;
        vec_sz = n;
    }

    template <typename T>
    fast_vector<T>::fast_vector(typename fast_vector<T>::iterator first, typename fast_vector<T>::iterator last)
    {
        size_type i, count = last - first;
        rsrv_sz = count << 2;
        arr = new T[rsrv_sz];
        for (i = 0; i < count; ++i, ++first)
            arr[i] = *first;
        vec_sz = count;
    }

    template <typename T>
    fast_vector<T>::fast_vector(std::initializer_list<T> lst)
    {
        rsrv_sz = lst.size() << 2;
        arr = new T[rsrv_sz];
        for (auto &item : lst)
            arr[vec_sz++] = item;
    }

    template <typename T>
    fast_vector<T>::fast_vector(const fast_vector<T> &other)
    {
        size_type i;
        rsrv_sz = other.rsrv_sz;
        arr = new T[rsrv_sz];
        for (i = 0; i < other.vec_sz; ++i)
            arr[i] = other.arr[i];
        vec_sz = other.vec_sz;
    }

    template <typename T>
    fast_vector<T>::fast_vector(fast_vector<T> &&other) noexcept
    {
        size_type i;
        rsrv_sz = other.rsrv_sz;
        arr = new T[rsrv_sz];
        for (i = 0; i < other.vec_sz; ++i)
            arr[i] = std::move(other.arr[i]);
        vec_sz = other.vec_sz;
    }

    template <typename T>
    fast_vector<T>::~fast_vector()
    {
        arr -= arr_offset;
        delete[] arr;
    }

    template <typename T>
    fast_vector<T> &fast_vector<T>::operator=(const fast_vector<T> &other)
    {
        size_type i;
        if (rsrv_sz < other.vec_sz) {
            rsrv_sz = other.vec_sz << 2;
            reallocate();
        }
        for (i = 0; i < other.vec_sz; ++i)
            arr[i] = other.arr[i];
        vec_sz = other.vec_sz;
    }

    template <typename T>
    fast_vector<T> &fast_vector<T>::operator=(fast_vector<T> &&other)
    {
        size_type i;
        if (rsrv_sz < other.vec_sz) {
            rsrv_sz = other.vec_sz << 2;
            reallocate();
        }
        for (i = 0; i < other.vec_sz; ++i)
            arr[i] = std::move(other.arr[i]);
        vec_sz = other.vec_sz;
    }

    template <typename T>
    fast_vector<T> &fast_vector<T>::operator=(std::initializer_list<T> lst)
    {
        if (rsrv_sz < lst.size()) {
            rsrv_sz = lst.size() << 2;
            reallocate();
        }
        vec_sz = 0;
        for (auto &item : lst)
            arr[vec_sz++] = item;
    }

    template <typename T>
    void fast_vector<T>::assign(typename fast_vector<T>::size_type count, const T &value)
    {
        size_type i;
        if (count > rsrv_sz) {
            rsrv_sz = count << 2;
            reallocate();
        }
        for (i = 0; i < count; ++i)
            arr[i] = value;
        vec_sz = count;
    }

    template <typename T>
    void fast_vector<T>::assign(typename fast_vector<T>::iterator first, typename fast_vector<T>::iterator last)
    {
        size_type i, count = last - first;
        if (count > rsrv_sz) {
            rsrv_sz = count << 2;
            reallocate();
        }
        for (i = 0; i < count; ++i, ++first)
            arr[i] = *first;
        vec_sz = count;
    }

    template <typename T>
    void fast_vector<T>::assign(std::initializer_list<T> lst)
    {
        size_type i, count = lst.size();
        if (count > rsrv_sz) {
            rsrv_sz = count << 2;
            reallocate();
        }
        i = 0;
        for (auto &item : lst)
            arr[i++] = item;
    }

    template <typename T>
    typename fast_vector<T>::iterator fast_vector<T>::begin() noexcept
    {
        return arr;
    }

    template <typename T>
    typename fast_vector<T>::const_iterator fast_vector<T>::cbegin() const noexcept
    {
        return arr;
    }

    template <typename T>
    typename fast_vector<T>::iterator fast_vector<T>::end() noexcept
    {
        return arr + vec_sz;
    }

    template <typename T>
    typename fast_vector<T>::const_iterator fast_vector<T>::cend() const noexcept
    {
        return arr + vec_sz;
    }

    template <typename T>
    typename fast_vector<T>::reverse_iterator fast_vector<T>::rbegin() noexcept
    {
        return reverse_iterator(arr + vec_sz);
    }

    template <typename T>
    typename fast_vector<T>::const_reverse_iterator fast_vector<T>::crbegin() const noexcept
    {
        return reverse_iterator(arr + vec_sz);
    }

    template <typename T>
    typename fast_vector<T>::reverse_iterator fast_vector<T>::rend() noexcept
    {
        return reverse_iterator(arr);
    }

    template <typename T>
    typename fast_vector<T>::const_reverse_iterator fast_vector<T>::crend() const noexcept
    {
        return reverse_iterator(arr);
    }

    template <typename T>
    inline void fast_vector<T>::reallocate()
    {
        T *tarr = new T[rsrv_sz];
        arr -= arr_offset;
        memcpy(tarr, arr, vec_sz * sizeof(T));
        delete[] arr;
        arr = tarr;
        arr += arr_offset;
    }

    template <typename T>
    bool fast_vector<T>::empty() const noexcept
    {
        return vec_sz == 0;
    }

    template <typename T>
    typename fast_vector<T>::size_type fast_vector<T>::size() const noexcept
    {
        return vec_sz;
    }

    template <typename T>
    typename fast_vector<T>::size_type fast_vector<T>::max_size() const noexcept
    {
        return LNI_fast_vector_MAX_SZ;
    }

    template <typename T>
    typename fast_vector<T>::size_type fast_vector<T>::capacity() const noexcept
    {
        return rsrv_sz;
    }

    template <typename T>
    typename fast_vector<T>::size_type fast_vector<T>::header_size() const noexcept
    {
        return arr_offset;
    }

    template <typename T>
    void fast_vector<T>::resize(typename fast_vector<T>::size_type sz)
    {
        if (sz > vec_sz) {
            if (sz > rsrv_sz) {
                rsrv_sz = sz;
                reallocate();
            }
        }
        else {
            size_type i;
            for (i = vec_sz; i < sz; ++i)
                arr[i].~T();
        }
        vec_sz = sz;
    }

    template <typename T>
    void fast_vector<T>::resize(typename fast_vector<T>::size_type sz, const T &c)
    {
        if (sz > vec_sz) {
            if (sz > rsrv_sz) {
                rsrv_sz = sz;
                reallocate();
            }
            size_type i;
            for (i = vec_sz; i < sz; ++i)
                arr[i] = c;
        }
        else {
            size_type i;
            for (i = vec_sz; i < sz; ++i)
                arr[i].~T();
        }
        vec_sz = sz;
    }

    template <typename T>
    void fast_vector<T>::reserve(typename fast_vector<T>::size_type _sz)
    {
        if (_sz > rsrv_sz) {
            rsrv_sz = _sz;
            reallocate();
        }
    }

    template <typename T>
    void fast_vector<T>::shrink_to_fit()
    {
        rsrv_sz = vec_sz;
        reallocate();
    }

    template <typename T>
    typename fast_vector<T>::reference fast_vector<T>::operator[](typename fast_vector<T>::size_type idx)
    {
        return arr[idx];
    }

    template <typename T>
    typename fast_vector<T>::const_reference fast_vector<T>::operator[](typename fast_vector<T>::size_type idx) const
    {
        return arr[idx];
    }

    template <typename T>
    typename fast_vector<T>::reference fast_vector<T>::at(size_type pos)
    {
        if (pos < vec_sz)
            return arr[pos];
        else
            throw std::out_of_range("accessed position is out of range");
    }

    template <typename T>
    typename fast_vector<T>::const_reference fast_vector<T>::at(size_type pos) const
    {
        if (pos < vec_sz)
            return arr[pos];
        else
            throw std::out_of_range("accessed position is out of range");
    }

    template <typename T>
    typename fast_vector<T>::reference fast_vector<T>::front()
    {
        return arr[0];
    }

    template <typename T>
    typename fast_vector<T>::const_reference fast_vector<T>::front() const
    {
        return arr[0];
    }

    template <typename T>
    typename fast_vector<T>::reference fast_vector<T>::back()
    {
        return arr[vec_sz - 1];
    }

    template <typename T>
    typename fast_vector<T>::const_reference fast_vector<T>::back() const
    {
        return arr[vec_sz - 1];
    }

    template <typename T>
    T *fast_vector<T>::header() noexcept
    {
        return (arr - arr_offset);
    }

    template <typename T>
    T *fast_vector<T>::header() const noexcept
    {
        return (arr - arr_offset);
    }

    template <typename T>
    T *fast_vector<T>::data() noexcept
    {
        return arr;
    }

    template <typename T>
    const T *fast_vector<T>::data() const noexcept
    {
        return arr;
    }

    template <typename T>
    template <class... Args>
    void fast_vector<T>::emplace_back(Args &&...args)
    {
        if (unlikely(vec_sz == rsrv_sz)) {
            rsrv_sz <<= 2;
            reallocate();
        }
        arr[vec_sz] = std::move(T(std::forward<Args>(args)...));
        ++vec_sz;
    }

    template <typename T>
    void fast_vector<T>::push_back(const T &val)
    {
        if (unlikely(vec_sz == rsrv_sz)) {
            rsrv_sz <<= 2;
            reallocate();
        }
        arr[vec_sz] = val;
        ++vec_sz;
    }

    template <typename T>
    void fast_vector<T>::push_back(T &&val)
    {
        if (unlikely(vec_sz == rsrv_sz)) {
            rsrv_sz <<= 2;
            reallocate();
        }
        arr[vec_sz] = std::move(val);
        ++vec_sz;
    }

    template <typename T>
    void fast_vector<T>::pop_back()
    {
        if (vec_sz)
            --vec_sz;
        arr[vec_sz].~T();
    }

    template <typename T>
    void fast_vector<T>::apply_offset(int offset)
    {
        if (unlikely(offset >= rsrv_sz)) {
            rsrv_sz = offset + 1;
            reallocate();
        }

        arr -= arr_offset;
        arr_offset = offset;
        arr += arr_offset;
    }

    template <typename T>
    template <class... Args>
    typename fast_vector<T>::iterator fast_vector<T>::emplace(typename fast_vector<T>::const_iterator it, Args &&...args)
    {
        size_t copy_length = it - arr;
        iterator iit = &arr[copy_length];
        if (unlikely(vec_sz == rsrv_sz)) {
            rsrv_sz <<= 2;
            reallocate();
            iit = &arr[copy_length];
        }
        memmove(iit + 1, iit, (vec_sz - copy_length) * sizeof(T));
        (*iit) = std::move(T(std::forward<Args>(args)...));
        ++vec_sz;
        return iit;
    }

    template <typename T>
    typename fast_vector<T>::iterator fast_vector<T>::insert(typename fast_vector<T>::const_iterator it, const T &val)
    {
        size_t copy_length = it - arr;
        iterator iit = &arr[copy_length];
        if (unlikely(vec_sz == rsrv_sz)) {
            rsrv_sz <<= 2;
            reallocate();
            iit = &arr[copy_length];
        }
        memmove(iit + 1, iit, (vec_sz - copy_length) * sizeof(T));
        (*iit) = val;
        ++vec_sz;
        return iit;
    }

    template <typename T>
    typename fast_vector<T>::iterator fast_vector<T>::insert(typename fast_vector<T>::const_iterator it, T &&val)
    {
        size_t copy_length = it - arr;
        iterator iit = &arr[copy_length];
        if (unlikely(vec_sz == rsrv_sz)) {
            rsrv_sz <<= 2;
            reallocate();
            iit = &arr[copy_length];
        }
        memmove(iit + 1, iit, (vec_sz - copy_length) * sizeof(T));
        (*iit) = std::move(val);
        ++vec_sz;
        return iit;
    }

    template <typename T>
    typename fast_vector<T>::iterator fast_vector<T>::insert(typename fast_vector<T>::const_iterator it, typename fast_vector<T>::size_type cnt, const T &val)
    {
        size_t copy_length = it - arr;
        iterator f = &arr[copy_length];
        if (unlikely(!cnt))
            return f;
        if (unlikely(vec_sz + cnt > rsrv_sz)) {
            rsrv_sz = (vec_sz + cnt) << 2;
            reallocate();
            f = &arr[copy_length];
        }
        memmove(f + cnt, f, (vec_sz - copy_length) * sizeof(T));
        vec_sz += cnt;
        for (iterator it = f; cnt--; ++it)
            (*it) = val;
        return f;
    }

    template <typename T>
    template <class InputIt>
    typename fast_vector<T>::iterator fast_vector<T>::insert(typename fast_vector<T>::const_iterator it, InputIt first, InputIt last)
    {
        size_t copy_length = it - arr;
        iterator f = &arr[copy_length];
        size_type cnt = last - first;
        if (unlikely(!cnt))
            return f;
        if (unlikely(vec_sz + cnt > rsrv_sz)) {
            rsrv_sz = (vec_sz + cnt) << 2;
            reallocate();
            f = &arr[copy_length];
        }
        memmove(f + cnt, f, (vec_sz - copy_length) * sizeof(T));
        for (iterator it = f; first != last; ++it, ++first)
            (*it) = *first;
        vec_sz += cnt;
        return f;
    }

    template <typename T>
    typename fast_vector<T>::iterator fast_vector<T>::insert(typename fast_vector<T>::const_iterator it, std::initializer_list<T> lst)
    {
        size_type cnt = lst.size();
        size_t copy_length = it - arr;
        iterator f = &arr[copy_length];
        if (!cnt)
            return f;
        if (vec_sz + cnt > rsrv_sz) {
            rsrv_sz = (vec_sz + cnt) << 2;
            reallocate();
            f = &arr[copy_length];
        }
        memmove(f + cnt, f, (vec_sz - copy_length) * sizeof(T));
        iterator iit = f;
        for (auto &item : lst) {
            (*iit) = item;
            ++iit;
        }
        vec_sz += cnt;
        return f;
    }

    template <typename T>
    inline typename fast_vector<T>::iterator fast_vector<T>::erase(typename fast_vector<T>::const_iterator it)
    {
        iterator iit = &arr[it - arr];
        (*iit).~T();
        memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(T));
        --vec_sz;
        return iit;
    }

    template <typename T>
    inline typename fast_vector<T>::iterator fast_vector<T>::erase(typename fast_vector<T>::const_iterator first, typename fast_vector<T>::const_iterator last)
    {
        iterator f = &arr[first - arr];
        if (first == last)
            return f;
        for (; first != last; ++first)
            (*first).~T();
        memmove(f, last, (vec_sz - (last - arr)) * sizeof(T));
        vec_sz -= last - first;
        return f;
    }

    template <typename T>
    void fast_vector<T>::swap(fast_vector<T> &rhs)
    {
        size_t tvec_sz = vec_sz,
               trsrv_sz = rsrv_sz;
        T *tarr = arr;

        vec_sz = rhs.vec_sz;
        rsrv_sz = rhs.rsrv_sz;
        arr = rhs.arr;

        rhs.vec_sz = tvec_sz;
        rhs.rsrv_sz = trsrv_sz;
        rhs.arr = tarr;
    }

    template <typename T>
    void fast_vector<T>::clear() noexcept
    {
        size_type i;
        for (i = 0; i < vec_sz; ++i)
            arr[i].~T();
        vec_sz = 0;
    }

    template <typename T>
    bool fast_vector<T>::operator==(const fast_vector<T> &rhs) const
    {
        if (vec_sz != rhs.vec_sz)
            return false;
        size_type i;
        for (i = 0; i < vec_sz; ++i)
            if (arr[i] != rhs.arr[i])
                return false;
        return true;
    }

    template <typename T>
    bool fast_vector<T>::operator!=(const fast_vector<T> &rhs) const
    {
        if (vec_sz != rhs.vec_sz)
            return true;
        size_type i;
        for (i = 0; i < vec_sz; ++i)
            if (arr[i] != rhs.arr[i])
                return true;
        return false;
    }

    template <typename T>
    bool fast_vector<T>::operator<(const fast_vector<T> &rhs) const
    {
        size_type i, ub = vec_sz < rhs.vec_sz ? vec_sz : rhs.vec_sz;
        for (i = 0; i < ub; ++i)
            if (arr[i] != rhs.arr[i])
                return arr[i] < rhs.arr[i];
        return vec_sz < rhs.vec_sz;
    }

    template <typename T>
    bool fast_vector<T>::operator<=(const fast_vector<T> &rhs) const
    {
        size_type i, ub = vec_sz < rhs.vec_sz ? vec_sz : rhs.vec_sz;
        for (i = 0; i < ub; ++i)
            if (arr[i] != rhs.arr[i])
                return arr[i] < rhs.arr[i];
        return vec_sz <= rhs.vec_sz;
    }

    template <typename T>
    bool fast_vector<T>::operator>(const fast_vector<T> &rhs) const
    {
        size_type i, ub = vec_sz < rhs.vec_sz ? vec_sz : rhs.vec_sz;
        for (i = 0; i < ub; ++i)
            if (arr[i] != rhs.arr[i])
                return arr[i] > rhs.arr[i];
        return vec_sz > rhs.vec_sz;
    }

    template <typename T>
    bool fast_vector<T>::operator>=(const fast_vector<T> &rhs) const
    {
        size_type i, ub = vec_sz < rhs.vec_sz ? vec_sz : rhs.vec_sz;
        for (i = 0; i < ub; ++i)
            if (arr[i] != rhs.arr[i])
                return arr[i] > rhs.arr[i];
        return vec_sz >= rhs.vec_sz;
    }

#define LIST_ALL(FUNCTION)           \
    FUNCTION(bool)                   \
    FUNCTION(signed char)            \
    FUNCTION(unsigned char)          \
    FUNCTION(char)                   \
    FUNCTION(short int)              \
    FUNCTION(unsigned short int)     \
    FUNCTION(int)                    \
    FUNCTION(unsigned int)           \
    FUNCTION(long int)               \
    FUNCTION(unsigned long int)      \
    FUNCTION(long long int)          \
    FUNCTION(unsigned long long int) \
    FUNCTION(float)                  \
    FUNCTION(double)                 \
    FUNCTION(long double)

#define RESIZE(TYPE)                                                                \
    template <>                                                                     \
    inline void fast_vector<TYPE>::resize(typename fast_vector<TYPE>::size_type sz) \
    {                                                                               \
        if (sz > rsrv_sz) {                                                         \
            rsrv_sz = sz;                                                           \
            reallocate();                                                           \
        }                                                                           \
        vec_sz = sz;                                                                \
    }

#define RESIZE_WITH_VALUE(TYPE)                                                     \
    template <>                                                                     \
    inline void fast_vector<TYPE>::resize(typename fast_vector<TYPE>::size_type sz, \
                                          const TYPE &c)                            \
    {                                                                               \
        if (sz > vec_sz) {                                                          \
            if (sz > rsrv_sz) {                                                     \
                rsrv_sz = sz;                                                       \
                reallocate();                                                       \
            }                                                                       \
            size_type i;                                                            \
            for (i = vec_sz; i < sz; ++i)                                           \
                arr[i] = c;                                                         \
        }                                                                           \
        vec_sz = sz;                                                                \
    }

#define POP_BACK(TYPE) \
    template <>        \
    inline void fast_vector<TYPE>::pop_back() { --vec_sz; }

#define ERASE(TYPE)                                                       \
    template <>                                                           \
    inline typename fast_vector<TYPE>::iterator fast_vector<TYPE>::erase( \
        typename fast_vector<TYPE>::const_iterator it)                    \
    {                                                                     \
        iterator iit = &arr[it - arr];                                    \
        memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(TYPE));  \
        --vec_sz;                                                         \
        return iit;                                                       \
    }

#define ERASE_ITERATOR(TYPE)                                              \
    template <>                                                           \
    inline typename fast_vector<TYPE>::iterator fast_vector<TYPE>::erase( \
        typename fast_vector<TYPE>::const_iterator first,                 \
        typename fast_vector<TYPE>::const_iterator last)                  \
    {                                                                     \
        iterator f = &arr[first - arr];                                   \
        if (first == last)                                                \
            return f;                                                     \
        memmove(f, last, (vec_sz - (last - arr)) * sizeof(TYPE));         \
        vec_sz -= last - first;                                           \
        return f;                                                         \
    }

#define CLEAR(TYPE) \
    template <>     \
    inline void fast_vector<TYPE>::clear() noexcept { vec_sz = 0; }

    LIST_ALL(RESIZE)
    LIST_ALL(RESIZE_WITH_VALUE)
    LIST_ALL(POP_BACK)
    LIST_ALL(ERASE)
    LIST_ALL(ERASE_ITERATOR)
}

#endif