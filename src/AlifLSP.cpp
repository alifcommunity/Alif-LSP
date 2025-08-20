#include "Server.h"

#include <iostream>

int main() {
	// Disable stdio buffering
	setvbuf(stdin, nullptr, _IONBF, 0);
	setvbuf(stdout, nullptr, _IONBF, 0);

	LSPServer server{};
	return server.run();
}
