#ifndef NETWORK_MONITOR_H
#define NETWORK_MONITOR_H

#include <vector>

#include "../models/Connection.h"

namespace network
{

class NetworkMonitor
{
public:
    std::vector<Connection> getActiveConnections();
};

} // namespace network

#endif