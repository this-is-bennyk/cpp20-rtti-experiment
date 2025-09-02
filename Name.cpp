// MIT License
//
// Copyright (c) 2025 Entropy Embracers LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "Name.hpp"

#include <algorithm>
#include <iterator>
#include <vector>
#include "OS.hpp"
#include "Program.hpp"

namespace
{
    struct NameKeymaker
    {
        template <typename U>
        struct rebind
        {
            using other = sk::patricia_key_maker<U>;
        };

        auto operator()(const Program::Name& name) const -> sk::patricia_key
        {
            return sk::patricia_key(std::as_bytes(std::span(name.data(), name.size())));
        };
    };

    OS::Vector<wchar_t> name_pool;
    OS::TrieSet<Program::Name, NameKeymaker> name_set;
}

Program::Name Program::LiteralName(const wchar_t* literal)
{
    const auto name = Name(literal);
    auto search = name_set.find(name);

    if (search == name_set.end())
    {
        auto [iterator, success] = name_set.insert(name);
        Assert(success, "Could not store name!");
        return *iterator;
    }

    return *search;
}

Program::Name Program::StringName(const wchar_t* string, const std::size_t size)
{
    auto name = Name(string, size);
    auto search = name_set.find(name);

    if (search == name_set.end())
    {
        const size_t old_list_size = name_pool.size();

        name_pool.resize(old_list_size + size);
        std::copy_n(string, size, std::next(name_pool.begin(), old_list_size));

        name = Name(&(*std::next(name_pool.begin(), old_list_size)), size);

        auto [iterator, success] = name_set.insert(name);
        Assert(success, "Could not store name!");
        return *iterator;
    }

    return *search;
}
