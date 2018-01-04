/*  Copyright (c) 2016 Michael Hansen

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE. */

#ifndef _ST_CHARBUFFER_H
#define _ST_CHARBUFFER_H

#include "st_assert.h"

#include <cstddef>
#include <cstring>
#include <cwchar>
#include <iterator>
#ifdef ST_HAVE_RVALUE_MOVE
#  include <utility>    // For std::move
#endif

#if !defined(ST_NO_STL_STRINGS)
#   include <string>
#   if defined(ST_HAVE_CXX17_STRING_VIEW)
#       include <string_view>
        namespace ST
        {
            template <typename char_T>
            using _std_basic_string_view = std::basic_string_view<char_T>;
            using _std_string_view = std::string_view;
            using _std_wstring_view = std::wstring_view;
            using _std_u16string_view = std::u16string_view;
            using _std_u32string_view = std::u32string_view;
        }
#   elif defined(ST_HAVE_EXPERIMENTAL_STRING_VIEW)
#       include <experimental/string_view>
        namespace ST
        {
            template <typename char_T>
            using _std_basic_string_view = std::experimental::basic_string_view<char_T>;
            using _std_string_view = std::experimental::string_view;
            using _std_wstring_view = std::experimental::wstring_view;
            using _std_u16string_view = std::experimental::u16string_view;
            using _std_u32string_view = std::experimental::u32string_view;
        }
#   endif
#endif

#define ST_AUTO_SIZE    (static_cast<size_t>(-1))

namespace ST
{
    // For optimized construction of empty objects
    struct ST_EXPORT null_t
    {
        null_t() { }
    };
    static const null_t null;

    template <typename char_T>
    class ST_EXPORT buffer
    {
    public:
        // STL-compatible typedefs
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef char_T value_type;
        typedef value_type *pointer;
        typedef const value_type *const_pointer;
        typedef value_type &reference;
        typedef const value_type &const_reference;

        // This should satisfy ContiguousIterator if std::array is any indication
        typedef value_type *iterator;
        typedef const value_type *const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    private:
        char_T *m_chars;
        size_t m_size;
        char_T m_data[ST_SHORT_STRING_LEN];

        inline bool is_reffed() const ST_NOEXCEPT
        {
            return m_size >= ST_SHORT_STRING_LEN;
        }

        struct _scope_deleter
        {
            // Useful for deleting the buffer at the *end* of a function,
            // even though we must capture it at the beginning, in case
            // the user does something silly like assign a buffer to itself.
            char_T *m_buffer;

            _scope_deleter(buffer<char_T> *self)
            {
                m_buffer = self->is_reffed() ? self->m_chars : ST_NULLPTR;
            }

            ~_scope_deleter()
            {
                delete[] m_buffer;
            }
        };

    public:
        buffer() ST_NOEXCEPT
            : m_chars(m_data), m_size()
        {
            memset(m_data, 0, sizeof(m_data));
        }

        buffer(const null_t &) ST_NOEXCEPT
            : m_chars(m_data), m_size()
        {
            memset(m_data, 0, sizeof(m_data));
        }

        buffer(const buffer<char_T> &copy)
            : m_size(copy.m_size)
        {
            if (is_reffed()) {
                m_chars = new char_T[m_size + 1];
                memcpy(m_chars, copy.m_chars, m_size * sizeof(char_T));
                m_chars[m_size] = 0;
            } else {
                memcpy(m_data, copy.m_data, sizeof(m_data));
                m_chars = m_data;
            }
        }

#ifdef ST_HAVE_RVALUE_MOVE
        buffer(buffer<char_T> &&move) ST_NOEXCEPT
            : m_size(move.m_size)
        {
            m_chars = is_reffed() ? move.m_chars : m_data;
            memcpy(m_data, move.m_data, sizeof(m_data));
            move.m_size = 0;
        }
#endif

        buffer(const char_T *data, size_t size)
            : m_size(size)
        {
            memset(m_data, 0, sizeof(m_data));
            m_chars = is_reffed() ? new char_T[m_size + 1] : m_data;
            memmove(m_chars, data, m_size * sizeof(char_T));
            m_chars[m_size] = 0;
        }

        ~buffer<char_T>() ST_NOEXCEPT
        {
            if (is_reffed())
                delete[] m_chars;
        }

        buffer<char_T> &operator=(const null_t &) ST_NOEXCEPT
        {
            _scope_deleter unref(this);
            m_chars = m_data;
            m_size = 0;
            memset(m_data, 0, sizeof(m_data));
            return *this;
        }

        buffer<char_T> &operator=(const buffer<char_T> &copy)
        {
            _scope_deleter unref(this);
            m_size = copy.m_size;
            if (is_reffed()) {
                m_chars = new char_T[m_size + 1];
                memcpy(m_chars, copy.m_chars, m_size * sizeof(char_T));
                m_chars[m_size] = 0;
            } else {
                memcpy(m_data, copy.m_data, sizeof(m_data));
                m_chars = m_data;
            }
            return *this;
        }

#ifdef ST_HAVE_RVALUE_MOVE
        buffer<char_T> &operator=(buffer<char_T> &&move) ST_NOEXCEPT
        {
            _scope_deleter unref(this);
            m_size = move.m_size;
            m_chars = is_reffed() ? move.m_chars : m_data;
            memcpy(m_data, move.m_data, sizeof(m_data));
            move.m_size = 0;
            return *this;
        }
#endif

        bool operator==(const null_t &) const ST_NOEXCEPT
        {
            return empty();
        }

        bool operator==(const buffer<char_T> &other) const ST_NOEXCEPT
        {
            if (other.size() != size())
                return false;
            return memcmp(data(), other.data(), size()) == 0;
        }

        bool operator!=(const null_t &) const ST_NOEXCEPT
        {
            return !empty();
        }

        bool operator!=(const buffer<char_T> &other) const ST_NOEXCEPT
        {
            return !operator==(other);
        }

        char_T *data() ST_NOEXCEPT { return m_chars; }
        const char_T *data() const ST_NOEXCEPT { return m_chars; }

        size_t size() const ST_NOEXCEPT { return m_size; }
        bool empty() const ST_NOEXCEPT { return m_size == 0; }

        ST_DEPRECATED_IN_2_0("replaced with empty() in string_theory 2.0")
        bool is_empty() const ST_NOEXCEPT { return empty(); }

        char_T &at(size_t index)
        {
            if (index >= size())
                throw std::out_of_range("Character index out of range");
            return m_chars[index];
        }

        char_T at(size_t index) const
        {
            if (index >= size())
                throw std::out_of_range("Character index out of range");
            return m_chars[index];
        }

        char_T &operator[](size_t index) ST_NOEXCEPT
        {
            return m_chars[index];
        }

        char_T operator[](size_t index) const ST_NOEXCEPT
        {
            return m_chars[index];
        }

        char_T &front() ST_NOEXCEPT
        {
            return m_chars[0];
        }

        char_T front() const ST_NOEXCEPT
        {
            return m_chars[0];
        }

        char_T &back() ST_NOEXCEPT
        {
            return empty() ? m_chars[0] : m_chars[m_size - 1];
        }

        char_T back() const ST_NOEXCEPT
        {
            return empty() ? m_chars[0] : m_chars[m_size - 1];
        }

        iterator begin() ST_NOEXCEPT { return m_chars; }
        const_iterator begin() const ST_NOEXCEPT { return m_chars; }
        const_iterator cbegin() const ST_NOEXCEPT { return m_chars; }

        iterator end() ST_NOEXCEPT { return m_chars + m_size; }
        const_iterator end() const ST_NOEXCEPT { return m_chars + m_size; }
        const_iterator cend() const ST_NOEXCEPT { return m_chars + m_size; }

        reverse_iterator rbegin() ST_NOEXCEPT
        {
            return reverse_iterator(end());
        }
        const_reverse_iterator rbegin() const ST_NOEXCEPT
        {
            return const_reverse_iterator(end());
        }
        const_reverse_iterator crbegin() const ST_NOEXCEPT
        {
            return const_reverse_iterator(cend());
        }

        reverse_iterator rend() ST_NOEXCEPT
        {
            return reverse_iterator(begin());
        }
        const_reverse_iterator rend() const ST_NOEXCEPT
        {
            return const_reverse_iterator(begin());
        }
        const_reverse_iterator crend() const ST_NOEXCEPT
        {
            return const_reverse_iterator(cbegin());
        }

        void allocate(size_t size)
        {
            if (is_reffed())
                delete[] m_chars;
            else
                memset(m_data, 0, sizeof(m_data));

            m_size = size;
            m_chars = is_reffed() ? new char_T[m_size + 1] : m_data;
            m_chars[m_size] = 0;
        }

        void allocate(size_t size, int fill)
        {
            allocate(size);
            memset(m_chars, fill, size);
        }

        ST_DEPRECATED_IN_2_0("Use allocate() and mutable data()/operator[] accessors")
        char_T *create_writable_buffer(size_t size)
        {
            allocate(size);
            return data();
        }

        static inline size_t strlen(const char_T *buffer)
        {
            ST_ASSERT(buffer, "buffer<char_T>::strlen passed null buffer");

            size_t length = 0;
            for ( ; *buffer++; ++length)
                ;
            return length;
        }

#ifndef ST_NO_STL_STRINGS
        std::basic_string<char_T> to_std_string() const
        {
            return std::basic_string<char_T>(data(), size());
        }

#ifdef ST_HAVE_STD_STRING_VIEW
        ST::_std_basic_string_view<char_T> view(size_t start = 0,
                                                size_t length = ST_AUTO_SIZE)
        {
            if (length == ST_AUTO_SIZE)
                length = size() - start;
            return ST::_std_basic_string_view<char_T>(data() + start, length);
        }

        operator ST::_std_basic_string_view<char_T>() const
        {
            return ST::_std_basic_string_view<char_T>(data(), size());
        }
#endif

#endif  /* ST_NO_STL_STRINGS */
    };

    typedef buffer<char>        char_buffer;
    typedef buffer<wchar_t>     wchar_buffer;
    typedef buffer<char16_t>    utf16_buffer;
    typedef buffer<char32_t>    utf32_buffer;

    template <typename char_T>
    bool operator==(const null_t &, const buffer<char_T> &right) ST_NOEXCEPT
    {
        return right.empty();
    }

    template <typename char_T>
    bool operator!=(const null_t &, const buffer<char_T> &right) ST_NOEXCEPT
    {
        return !right.empty();
    }
}

#endif // _ST_CHARBUFFER_H
