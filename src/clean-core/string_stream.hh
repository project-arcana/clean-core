#pragma once

#include <cstring> // std::memcpy

#include <clean-core/string.hh>
#include <clean-core/string_view.hh>

namespace cc
{
class string_stream
{
public: // methods
    string_stream& operator<<(string_view sv)
    {
        reserve(sv.size());
        std::memcpy(m_curr, sv.data(), sv.size());
        m_curr += sv.size();
        return *this;
    }

    [[nodiscard]] string to_string() const
    {
        if (empty())
            return {};
        string s = string::uninitialized(size());
        std::memcpy(s.data(), m_data, size());
        return s;
    }

    /// reseve space for at least size more elements
    void reserve(size_t size)
    {
        size_t req_size = this->size() + size;
        if (req_size <= m_capacity)
            return;

        size_t new_cap = (m_capacity << 1) < req_size ? req_size : (m_capacity << 1);
        char* new_data = new char[new_cap];
        if (!empty())
            std::memcpy(new_data, m_data, this->size());
        delete[] m_data;
        m_curr = m_curr - m_data + new_data;
        m_data = new_data;
        m_capacity = new_cap;
    }

    void clear() { m_curr = m_data; }

public: // properties
    [[nodiscard]] size_t size() const noexcept { return m_curr - m_data; }
    [[nodiscard]] bool empty() const { return m_curr == m_data; }

public: // ctor
    string_stream() = default;

    string_stream(string_stream const& rhs)
    {
        if (!rhs.empty())
        {
            m_data = new char[rhs.size()];
            std::memcpy(m_data, rhs.m_data, rhs.size());
            m_curr = m_data + rhs.size();
            m_capacity = rhs.size();
        }
    };

    string_stream(string_stream&& rhs) noexcept
    {
        m_data = rhs.m_data;
        m_curr = rhs.m_curr;
        m_capacity = rhs.m_capacity;
        rhs.m_data = nullptr;
        rhs.m_curr = nullptr;
        rhs.m_capacity = 0;
    };

    ~string_stream() { delete[] m_data; }

public: // assignment
    string_stream& operator=(string_stream const& rhs)
    {
        if (this != &rhs)
        {
            if (m_capacity < rhs.size())
            {
                delete[] m_data;
                m_data = new char[rhs.size()];
                m_capacity = rhs.size();
            }
            std::memcpy(m_data, rhs.m_data, rhs.size());
            m_curr = m_data + rhs.size();
        }
        return *this;
    };

    string_stream& operator=(string_stream&& rhs) noexcept
    {
        delete[] m_data;
        m_data = rhs.m_data;
        m_curr = rhs.m_curr;
        m_capacity = rhs.m_capacity;
        rhs.m_data = nullptr;
        rhs.m_curr = nullptr;
        rhs.m_capacity = 0;
        return *this;
    };

private: // member
    char* m_data = nullptr;
    char* m_curr = nullptr;
    size_t m_capacity = 0;
};

inline string to_string(string_stream const& ss) { return ss.to_string(); }

}
