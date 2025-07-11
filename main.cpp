#include <iostream>
#include "Meta.h"
// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.

class Foo { public: int x = 0; };

class Bar : public Foo { public: int y = 0; };

META(Foo)
META(Bar, Meta::AddInheritance<Type, Foo>())

int main()
{
	// // TIP Press <shortcut actionId="RenameElement"/> when your caret is at the <b>lang</b> variable name to see how CLion can help you rename it.
	// const auto language = "C++";
	// std::cout << "Hello and welcome to " << language << "!\n";
	//
	// for (int i = 1; i <= 5; i++)
	// {
	// 	// TIP Press <shortcut actionId="Debug"/> to start debugging your code. We have set one <icon src="AllIcons.Debugger.Db_set_breakpoint"/> breakpoint for you, but you can always add more by pressing <shortcut actionId="ToggleLineBreakpoint"/>.
	// 	std::cout << "i = " << i << std::endl;
	// }
	//
	// return 0;
	// TIP See CLion help at <a href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>. Also, you can try interactive lessons for CLion by selecting 'Help | Learn IDE Features' from the main menu.

	Meta::Dump();

	Meta::Spandle span(3);

	Meta::Handle b = true;

	span[0] = true;
	span[1] = 34;
	span[2] = 3.14;

	const auto result = span.expand<bool, i32, f64>();

	std::cout << std::get<0>(result) << std::endl;
	std::cout << std::get<1>(result) << std::endl;
	std::cout << std::get<2>(result) << std::endl;

	const Meta::Method f = Meta::From(&Meta::Handle::valid);

	std::cout << std::boolalpha << f(b, Meta::Spandle()).as<bool>() << std::endl;
}