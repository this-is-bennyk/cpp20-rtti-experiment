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

#include "Math.hpp"

#include "Meta.hpp"

META_TYPE_AS(Math::Vector1<Math::DefaultFloat>, Vector1, Meta::AddPOD<Type>());
META_TYPE_AS(Math::Vector2<Math::DefaultFloat>, Vector2, Meta::AddPOD<Type>());
META_TYPE_AS(Math::Vector3<Math::DefaultFloat>, Vector3, Meta::AddPOD<Type>());
META_TYPE_AS(Math::Vector4<Math::DefaultFloat>, Vector4, Meta::AddPOD<Type>());

META_TYPE_AS(Math::Vector1I<Math::DefaultInt>, Vector1I, Meta::AddPOD<Type>());
META_TYPE_AS(Math::Vector2I<Math::DefaultInt>, Vector2I, Meta::AddPOD<Type>());
META_TYPE_AS(Math::Vector3I<Math::DefaultInt>, Vector3I, Meta::AddPOD<Type>());
META_TYPE_AS(Math::Vector4I<Math::DefaultInt>, Vector4I, Meta::AddPOD<Type>());
