# A Modern C++20 RTTI / Reflection Experiment

## What?

This experiment creates a runtime type information (RTTI) system that allows for using and examining datatypes baked into the program.

## Why?

- C++ type reflection is notoriously difficult. I dislike most implementations because they seem overly complicated. I wanted to see if it could be done in a simpler manner, using as few macros as possible and as much useful templatization as possible.
- I was bored one day and wanted to do it.

## How...?

...did I make it? With some C++20 chicanery.

...do you use it? Great question! Like this, in your implementation (.cpp/.cxx/etc.) file:

### Basic registration
```c++
META(MyType)
```

### Adding inheritance
```c++
META(MyType, Meta::AddInheritance<Type, Base1, Base2, ...>())
```

## May I?

Yeah, see license.
