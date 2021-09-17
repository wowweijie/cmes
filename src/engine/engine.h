#include <ws/client.h>

class Engine
{
public:
    Engine();

    void set_gateways(std::vector<ws::IWSClient*> gateways);
}