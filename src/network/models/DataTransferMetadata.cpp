#include "DataTransferMetadata.h"

#include <sstream>

#include "../utils/NetworkUtils.h"

namespace network
{

DataTransferMetadata::DataTransferMetadata(
    uint32_t processId,
    const std::string& processName,
    const std::string& remoteIp,
    uint16_t remotePort,
    ProtocolType protocol,
    uint64_t uploadedBytes,
    uint64_t downloadedBytes,
    double uploadRateBps,
    double downloadRateBps,
    std::chrono::system_clock::time_point firstObserved,
    std::chrono::system_clock::time_point lastUpdated)
    : processId(processId),
      processName(processName),
      remoteIp(remoteIp),
      remotePort(remotePort),
      protocol(protocol),
      uploadedBytes(uploadedBytes),
      downloadedBytes(downloadedBytes),
      uploadRateBps(uploadRateBps),
      downloadRateBps(downloadRateBps),
      firstObserved(firstObserved),
      lastUpdated(lastUpdated)
{
}

DataTransferMetadata DataTransferMetadata::fromConnection(
    const Connection& connection,
    uint64_t uploadedBytes,
    uint64_t downloadedBytes,
    double uploadRateBps,
    double downloadRateBps,
    const std::string& processName,
    std::chrono::system_clock::time_point firstObserved)
{
    const auto observed = firstObserved == std::chrono::system_clock::time_point{}
                              ? connection.timestamp
                              : firstObserved;

    return DataTransferMetadata(
        connection.processId,
        processName,
        connection.remoteAddress,
        connection.remotePort,
        connection.protocol,
        uploadedBytes,
        downloadedBytes,
        uploadRateBps,
        downloadRateBps,
        observed,
        connection.timestamp);
}

DataTransferMetadata DataTransferMetadata::fromBandwidthStats(
    const ConnectionBandwidthStats& stats)
{
    return DataTransferMetadata(
        stats.processId,
        stats.processName,
        stats.remoteAddress,
        stats.remotePort,
        stats.protocol,
        stats.uploadedBytes,
        stats.downloadedBytes,
        stats.uploadRateBps,
        stats.downloadRateBps,
        stats.firstObserved,
        stats.lastUpdated);
}

uint64_t DataTransferMetadata::totalTransferredBytes() const
{
    return uploadedBytes + downloadedBytes;
}

std::string DataTransferMetadata::toString() const
{
    std::ostringstream out;
    out << "Process:\n"
        << (processName.empty() ? "Unknown" : processName)
        << "\n\n"
        << "Protocol:\n"
        << NetworkUtils::protocolToString(protocol)
        << "\n\n"
        << "Remote:\n"
        << remoteIp;

    if (NetworkUtils::hasRemotePort(remotePort))
    {
        out << ":" << remotePort;
    }

    out << "\n\n"
        << "Uploaded:\n"
        << NetworkUtils::formatBytes(uploadedBytes)
        << "\n\n"
        << "Downloaded:\n"
        << NetworkUtils::formatBytes(downloadedBytes)
        << "\n\n"
        << "Upload Rate:\n"
        << NetworkUtils::formatRate(uploadRateBps)
        << "\n\n"
        << "Download Rate:\n"
        << NetworkUtils::formatRate(downloadRateBps)
        << "\n\n"
        << "Total Transfer:\n"
        << NetworkUtils::formatBytes(totalTransferredBytes());

    return out.str();
}

} // namespace network
