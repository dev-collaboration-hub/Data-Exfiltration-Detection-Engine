#include <chrono>
#include <iostream>
#include <string>

#include "../../src/network/utils/NetworkUtils.h"
#include "../../src/network/utils/NetworkUtils.cpp"
#include "../../src/network/models/Connection.h"
#include "../../src/network/models/Connection.cpp"
#include "../../src/network/models/ConnectionBandwidthStats.h"
#include "../../src/network/models/ConnectionBandwidthStats.cpp"
#include "../../src/network/models/DataTransferMetadata.h"
#include "../../src/network/models/DataTransferMetadata.cpp"

using namespace network;

static int g_failures = 0;

#define EXPECT_TRUE(expr)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(expr))                                                           \
        {                                                                      \
            std::cerr << "FAIL: " << #expr << " at " << __FILE__ << ":"        \
                      << __LINE__ << "\n";                                     \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

#define EXPECT_EQ(a, b)                                                        \
    do                                                                         \
    {                                                                          \
        if (!((a) == (b)))                                                     \
        {                                                                      \
            std::cerr << "FAIL: " << #a << " == " << #b << " at " << __FILE__  \
                      << __LINE__ << "\n";                                     \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

static void testStoresTransferFields()
{
    const auto now = std::chrono::system_clock::now();
    DataTransferMetadata meta(
        4120,
        "python.exe",
        "104.18.32.45",
        443,
        ProtocolType::TCP,
        13107200ull, // 12.5 MB
        3355443ull,  // 3.2 MB
        1.4 * 1024.0 * 1024.0,
        320.0 * 1024.0,
        now,
        now);

    EXPECT_EQ(meta.processId, 4120u);
    EXPECT_EQ(meta.processName, std::string("python.exe"));
    EXPECT_EQ(meta.remoteIp, std::string("104.18.32.45"));
    EXPECT_EQ(meta.remotePort, 443);
    EXPECT_TRUE(meta.protocol == ProtocolType::TCP);
    EXPECT_EQ(meta.uploadedBytes, 13107200ull);
    EXPECT_EQ(meta.downloadedBytes, 3355443ull);
    EXPECT_TRUE(meta.uploadRateBps == 1.4 * 1024.0 * 1024.0);
    EXPECT_TRUE(meta.downloadRateBps == 320.0 * 1024.0);
    EXPECT_EQ(meta.totalTransferredBytes(), 13107200ull + 3355443ull);
}

static void testFromConnection()
{
    const auto now = std::chrono::system_clock::now();
    Connection connection(
        4120,
        "192.168.1.10",
        53142,
        "104.18.32.45",
        443,
        ProtocolType::TCP,
        ConnectionState::ESTABLISHED,
        now);

    DataTransferMetadata meta = DataTransferMetadata::fromConnection(
        connection,
        13107200ull,
        3355443ull,
        1.4 * 1024.0 * 1024.0,
        320.0 * 1024.0,
        "python.exe");

    EXPECT_EQ(meta.processId, 4120u);
    EXPECT_EQ(meta.processName, std::string("python.exe"));
    EXPECT_EQ(meta.remoteIp, std::string("104.18.32.45"));
    EXPECT_EQ(meta.remotePort, 443);
    EXPECT_TRUE(meta.protocol == ProtocolType::TCP);
    EXPECT_EQ(meta.uploadedBytes, 13107200ull);
    EXPECT_EQ(meta.downloadedBytes, 3355443ull);
    EXPECT_TRUE(meta.firstObserved == now);
    EXPECT_TRUE(meta.lastUpdated == now);
}

static void testFromBandwidthStats()
{
    const auto now = std::chrono::system_clock::now();
    ConnectionBandwidthStats stats(
        4120,
        "python.exe",
        "104.18.32.45",
        443,
        ProtocolType::TCP,
        13107200ull,
        3355443ull,
        1.4 * 1024.0 * 1024.0,
        320.0 * 1024.0,
        now,
        now,
        true);

    DataTransferMetadata meta =
        DataTransferMetadata::fromBandwidthStats(stats);

    EXPECT_EQ(meta.processId, 4120u);
    EXPECT_EQ(meta.processName, std::string("python.exe"));
    EXPECT_EQ(meta.remoteIp, std::string("104.18.32.45"));
    EXPECT_EQ(meta.remotePort, 443);
    EXPECT_TRUE(meta.protocol == ProtocolType::TCP);
    EXPECT_EQ(meta.uploadedBytes, 13107200ull);
    EXPECT_EQ(meta.downloadedBytes, 3355443ull);
    EXPECT_TRUE(meta.uploadRateBps == 1.4 * 1024.0 * 1024.0);
    EXPECT_TRUE(meta.downloadRateBps == 320.0 * 1024.0);
    EXPECT_EQ(meta.totalTransferredBytes(), stats.totalTransferredBytes());
}

static void testUploadAndDownloadTogether()
{
    DataTransferMetadata meta(
        1000,
        "chrome.exe",
        "2001:db8::1",
        443,
        ProtocolType::TCP,
        1024ull,
        2048ull,
        100.0,
        200.0,
        std::chrono::system_clock::now(),
        std::chrono::system_clock::now());

    EXPECT_EQ(meta.uploadedBytes, 1024ull);
    EXPECT_EQ(meta.downloadedBytes, 2048ull);
    EXPECT_EQ(meta.totalTransferredBytes(), 3072ull);
}

static void testOmitsRemotePortWhenUnavailable()
{
    Connection connection(
        4120,
        "0.0.0.0",
        8080,
        "0.0.0.0",
        0,
        ProtocolType::TCP,
        ConnectionState::LISTENING,
        std::chrono::system_clock::now());

    DataTransferMetadata meta =
        DataTransferMetadata::fromConnection(connection, 0, 0);

    EXPECT_EQ(meta.remotePort, 0);

    const std::string formatted = meta.toString();
    EXPECT_TRUE(formatted.find("0.0.0.0:") == std::string::npos);
    EXPECT_TRUE(formatted.find("Protocol:\nTCP") != std::string::npos);
}

static void testFormattedOutput()
{
    const auto now = std::chrono::system_clock::now();
    DataTransferMetadata meta(
        4120,
        "python.exe",
        "104.18.32.45",
        443,
        ProtocolType::TCP,
        13107200ull, // 12.5 MB
        3355443ull,  // 3.2 MB
        1.4 * 1024.0 * 1024.0,
        320.0 * 1024.0,
        now,
        now);

    const std::string expected =
        "Process:\n"
        "python.exe\n"
        "\n"
        "Protocol:\n"
        "TCP\n"
        "\n"
        "Remote:\n"
        "104.18.32.45:443\n"
        "\n"
        "Uploaded:\n"
        "12.5 MB\n"
        "\n"
        "Downloaded:\n"
        "3.2 MB\n"
        "\n"
        "Upload Rate:\n"
        "1.4 MB/s\n"
        "\n"
        "Download Rate:\n"
        "320.0 KB/s\n"
        "\n"
        "Total Transfer:\n"
        "15.7 MB";

    EXPECT_EQ(meta.toString(), expected);
}

static void testEmptyProcessNameFormatsAsUnknown()
{
    DataTransferMetadata meta(
        1,
        "",
        "10.0.0.1",
        80,
        ProtocolType::UDP,
        0,
        0,
        0.0,
        0.0,
        std::chrono::system_clock::now(),
        std::chrono::system_clock::now());

    const std::string formatted = meta.toString();
    EXPECT_TRUE(formatted.find("Process:\nUnknown\n") != std::string::npos);
    EXPECT_TRUE(formatted.find("Protocol:\nUDP\n") != std::string::npos);
}

int main()
{
    testStoresTransferFields();
    testFromConnection();
    testFromBandwidthStats();
    testUploadAndDownloadTogether();
    testOmitsRemotePortWhenUnavailable();
    testFormattedOutput();
    testEmptyProcessNameFormatsAsUnknown();

    if (g_failures == 0)
    {
        std::cout << "All DataTransferMetadata tests passed.\n";
        return 0;
    }

    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
