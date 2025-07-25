#include <iostream>
#include "Meta.h"
// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.

class Foo { public: int x = 0; };

class Bar : public Foo { public: int y = 0; };

META(Foo)
META(Bar, Meta::AddInheritance<Type, Foo>())

int main()
{
	Meta::DumpInfo();

	Meta::Spandle span(3);

	Meta::Handle b = true;
	Meta::Handle b2 = b;

	span[0] = true;
	span[1] = 34;
	span[2] = 3.14;

	const auto result = span.expand<bool, i32&, const f64&>();

	std::cout << std::get<0>(result) << std::endl;
	std::cout << std::get<1>(result) << std::endl;
	std::cout << std::get<2>(result) << std::endl;

	const Meta::Method method = Meta::FromMethod(&Meta::Handle::valid);

	std::cout << std::boolalpha << method(b, Meta::Spandle()).as<bool>() << std::endl;

	Foo foo;
	foo.x = 1;

	const Meta::Member member = Meta::FromMember(&Foo::x);

	std::cout << member(foo).as<int>() << std::endl;

	u8 bar_memory[sizeof(Bar)] = { 0 };
	const auto bar_view = Meta::View(&bar_memory[0], Meta::Info<Bar>());

	const Meta::Constructor constructor = Meta::FromCtor<Bar>();

	constructor(bar_view, Meta::Spandle());

	bar_view.as<Bar>().x = 2;
	bar_view.as<Bar>().y = 3;

	std::cout << bar_view.as<const Bar>().x << ", " << bar_view.as<const Bar>().y << std::endl;
	std::cout << bar_view.as<const Foo>().x << std::endl;

	const Meta::Destructor destructor = Meta::FromDtor<Bar>();

	destructor(bar_view);
}