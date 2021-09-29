#include "engine/engine.h"

Engine::Engine(std::vector<ws::IWSClient*> gateways_arg) : gateways{ gateways_arg } {}

//Engine::Engine(std::vector<ws::IWSClient*>&& gateways_arg) : gateways{ std::move(gateways_arg) } {}

void Engine::start() {
	auto f = [](ws::IWSClient *gateway) {
		gateway->subscribe_ticker("BTC-PERP");
		gateway->on_message([](json j) { std::cout << "msg: " << j << "\n"; });
		gateway->connect();
	};

	for (unsigned int k = 0; k < gateways.size(); k++) {
		std::thread thread_instance(f, gateways[k]);
		thread_instance.join();
	}

	
}