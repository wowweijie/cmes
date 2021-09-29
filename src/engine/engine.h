#include <ws/client.h>

class Engine
{
public:
    Engine(std::vector<ws::IWSClient*> gateways_arg);
    //Engine(std::vector<ws::IWSClient*>&& gateways_arg) : gateways{ std::move(gateways_arg)};
    void start();

private:
    std::vector < ws::IWSClient*> gateways;
    std::vector<std::thread> gateway_threads;
};