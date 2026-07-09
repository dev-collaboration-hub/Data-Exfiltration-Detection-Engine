// INetworkMonitor.h

#pragma once

class INetworkMonitor
{
public:
    virtual ~INetworkMonitor() = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;
};