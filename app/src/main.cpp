#include "pre-compiled-header.h"

#include "vk_engine.h"

int main() {

	Engine engine;
	
	try
	{
		engine.init();

		engine.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}