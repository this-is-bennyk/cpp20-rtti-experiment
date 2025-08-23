#include <iostream>
#include "Meta.h"
// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.

class Foo { public: int x = 20; };

class Bar : public Foo { public: int y = 0; };

META_TYPE(Foo, Meta::AddPOD<Type>());
META_TYPE(Bar, Meta::AddPOD<Type>(), Meta::AddInheritance<Type, Foo>());

int main()
{
	Meta::DumpInfo();

	auto b = Meta::Handle(true);
	Meta::Handle b2 = b;
	const auto b3 = Meta::Handle::Create<Foo>(Meta::Spandle());

	std::cout << std::boolalpha << b.as<bool>() << std::endl;
	std::cout << b2.as<bool>() << std::endl;
	std::cout << b3.as<const Foo&>().x << std::endl;

	bool truthy = true;

	auto span = Meta::Spandle(Meta::Handle(&truthy), 34, 3.14);
	// span[4] = 10; // Assert test

	const Meta::Method method = Meta::FromMethod<Meta::Handle, &Meta::Handle::valid, bool>();

	std::cout << std::boolalpha << method(Meta::View(b), Meta::Spandle()).as<bool>() << std::endl;

	Foo foo;
	foo.x = 1;

	const Meta::Member member = Meta::FromMember<Foo, &Foo::x, decltype(Foo::x)>();

	std::cout << member(Meta::View(foo)).as<int>() << std::endl;

	u8 bar_memory[sizeof(Bar)] = { 0 };
	const auto bar_view = Meta::View(&bar_memory[0], Meta::Info<Bar>(), Meta::kQualifier_Reference);

	const Meta::Constructor constructor = Meta::FromCtor<Bar>();

	constructor(bar_view, Meta::Spandle());

	bar_view.as<Bar&>().x = 2;
	bar_view.as<Bar&>().y = 3;

	std::cout << bar_view.as<const Bar&>().x << ", " << bar_view.as<const Bar&>().y << std::endl;
	std::cout << bar_view.as<const Foo&>().x << std::endl;

	const Meta::Destructor destructor = Meta::FromDtor<Bar>();

	destructor(bar_view);
}