#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../src/network/utils/NetworkUtils.h"
#include "../../src/network/utils/NetworkUtils.cpp"
#include "../../src/network/models/Connection.h"
#include "../../src/network/models/Connection.cpp"
#include "../../src/network/models/ConnectionUploadStats.h"
#include "../../src/network/models/ConnectionUploadStats.cpp"
#include "../../src/network/models/ConnectionDownloadStats.h"
#include "../../src/network/models/ConnectionDownloadStats.cpp"
#include "../../src/network/events/ConnectionEvent.h"
#include "../../src/network/events/ConnectionEvent.cpp"
#include "../../src/process/models/ProcessInfo.h"
#include "../../src/process/models/ProcessInfo.cpp"
#include "../../src/process/events/ProcessEvent.h"
#include "../../src/process/events/ProcessEvent.cpp"
#include "../../src/process/events/ProcessActivityEvent.h"
#include "../../src/process/events/ProcessActivityEvent.cpp"
#include "../../src/process/tracker/TimelineEventCollector.h"
#include "../../src/process/tracker/TimelineEventCollector.cpp"

using namespace process;
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
                      << ":" << __LINE__ << "\n";                              \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

static Connection makeConnection(
    uint32_t pid,
    const std::string& remoteIp,
    uint16_t remotePort,
    ProtocolType protocol = ProtocolType::TCP)
{
    return Connection(
        pid,
        "192.168.1.10",
        53142,
        remoteIp,
        remotePort,
        protocol,
        ConnectionState::ESTABLISHED,
        std::chrono::system_clock::now());
}

static void testEmptyInputDoesNotCrash()
{
    TimelineEventCollector collector;

    EXPECT_EQ(collector.collectProcessEvents({}), 0u);
    EXPECT_EQ(collector.collectActiveProcesses({}), 0u);
    EXPECT_EQ(collector.collectConnectionEvents({}), 0u);
    EXPECT_EQ(collector.collectUploadStats({}), 0u);
    EXPECT_EQ(collector.collectDownloadStats({}), 0u);
    EXPECT_EQ(
        collector.collect({}, {}, {}, {}),
        0u);
    EXPECT_EQ(collector.eventCount(), 0u);
    EXPECT_TRUE(collector.getEventsForProcess(4120).empty());
    EXPECT_TRUE(collector.getAllEvents().empty());
}

static void testMultipleEventsForOneProcess()
{
    TimelineEventCollector collector;
    const auto now = std::chrono::system_clock::now();

    ProcessInfo python(4120, "python.exe", 1000);
    ProcessEvent created(ProcessEventType::CREATED, python, now);

    Connection connection = makeConnection(4120, "104.18.32.45", 443);
    ConnectionEvent opened(ConnectionEventType::CREATED, connection, now);
    ConnectionEvent closed(ConnectionEventType::CLOSED, connection, now);

    ConnectionUploadStats upload(
        4120,
        "python.exe",
        "104.18.32.45",
        443,
        ProtocolType::TCP,
        2516582ull,
        now,
        now,
        true);

    std::unordered_map<uint32_t, ProcessInfo> lookup = {{4120, python}};

    const std::size_t added = collector.collect(
        {created}, {opened, closed}, {upload}, {}, lookup);

    EXPECT_EQ(added, 4u);
    EXPECT_EQ(collector.eventCount(), 4u);

    const auto events = collector.getEventsForProcess(4120);
    EXPECT_EQ(events.size(), 4u);

    EXPECT_TRUE(events[0].eventType == ProcessActivityEventType::PROCESS_SEEN);
    EXPECT_TRUE(
        events[1].eventType == ProcessActivityEventType::CONNECTION_OPENED);
    EXPECT_TRUE(
        events[2].eventType == ProcessActivityEventType::CONNECTION_CLOSED);
    EXPECT_TRUE(events[3].eventType == ProcessActivityEventType::UPLOAD_ACTIVITY);
    EXPECT_EQ(events[3].uploadedBytes, 2516582ull);
}

static void testNetworkEventsAttachToCorrectProcess()
{
    TimelineEventCollector collector;
    const auto now = std::chrono::system_clock::now();

    ProcessInfo python(4120, "python.exe", 1);
    ProcessInfo chrome(2204, "chrome.exe", 1);

    collector.collectActiveProcesses({python, chrome});

    ConnectionEvent pythonOpen(
        ConnectionEventType::CREATED,
        makeConnection(4120, "104.18.32.45", 443),
        now);
    ConnectionEvent chromeOpen(
        ConnectionEventType::CREATED,
        makeConnection(2204, "142.250.190.78", 443),
        now);

    std::unordered_map<uint32_t, ProcessInfo> lookup = {
        {4120, python},
        {2204, chrome}};

    collector.collectConnectionEvents({pythonOpen, chromeOpen}, lookup);

    const auto pythonEvents = collector.getEventsForProcess(4120);
    const auto chromeEvents = collector.getEventsForProcess(2204);

    EXPECT_EQ(pythonEvents.size(), 2u);
    EXPECT_EQ(chromeEvents.size(), 2u);

    EXPECT_EQ(pythonEvents[1].processId, 4120u);
    EXPECT_EQ(pythonEvents[1].processName, std::string("python.exe"));
    EXPECT_EQ(pythonEvents[1].remoteAddress, std::string("104.18.32.45:443"));

    EXPECT_EQ(chromeEvents[1].processId, 2204u);
    EXPECT_EQ(chromeEvents[1].processName, std::string("chrome.exe"));
    EXPECT_EQ(chromeEvents[1].remoteAddress, std::string("142.250.190.78:443"));
}

static void testAvoidsDuplicateEvents()
{
    TimelineEventCollector collector;
    const auto now = std::chrono::system_clock::now();

    ProcessInfo python(4120, "python.exe", 1);
    ProcessEvent created(ProcessEventType::CREATED, python, now);
    Connection connection = makeConnection(4120, "104.18.32.45", 443);
    ConnectionEvent opened(ConnectionEventType::CREATED, connection, now);

    ConnectionUploadStats upload(
        4120,
        "python.exe",
        "104.18.32.45",
        443,
        ProtocolType::TCP,
        4096ull,
        now,
        now,
        true);

    EXPECT_EQ(collector.collect({created}, {opened}, {upload}, {}), 3u);
    EXPECT_EQ(collector.collect({created}, {opened}, {upload}, {}), 0u);
    EXPECT_EQ(collector.collectActiveProcesses({python}), 0u);
    EXPECT_EQ(collector.eventCount(), 3u);

    // Growing upload totals create a new timeline observation.
    ConnectionUploadStats grown = upload;
    grown.uploadedBytes = 8192ull;
    EXPECT_EQ(collector.collectUploadStats({grown}), 1u);
    EXPECT_EQ(collector.eventCount(), 4u);
}

static void testUploadAndDownloadCollection()
{
    TimelineEventCollector collector;
    const auto now = std::chrono::system_clock::now();

    ConnectionUploadStats upload(
        4120,
        "python.exe",
        "104.18.32.45",
        443,
        ProtocolType::TCP,
        13107200ull,
        now,
        now,
        true);

    ConnectionDownloadStats download(
        4120,
        "python.exe",
        "104.18.32.45",
        443,
        ProtocolType::TCP,
        3355443ull,
        now,
        now,
        true);

    // Zero-byte stats are ignored.
    ConnectionUploadStats zeroUpload = upload;
    zeroUpload.uploadedBytes = 0;

    EXPECT_EQ(collector.collectUploadStats({zeroUpload}), 0u);
    EXPECT_EQ(collector.collectUploadStats({upload}), 1u);
    EXPECT_EQ(collector.collectDownloadStats({download}), 1u);

    const auto events = collector.getEventsForProcess(4120);
    EXPECT_EQ(events.size(), 2u);
    EXPECT_TRUE(events[0].eventType == ProcessActivityEventType::UPLOAD_ACTIVITY);
    EXPECT_TRUE(events[1].eventType == ProcessActivityEventType::DOWNLOAD_ACTIVITY);
    EXPECT_EQ(events[0].uploadedBytes, 13107200ull);
    EXPECT_EQ(events[1].downloadedBytes, 3355443ull);
}

static void testGroupedByPidMap()
{
    TimelineEventCollector collector;

    collector.collectActiveProcesses(
        {ProcessInfo(4120, "python.exe", 1),
         ProcessInfo(2204, "chrome.exe", 1)});

    const auto& byProcess = collector.getEventsByProcess();
    EXPECT_EQ(byProcess.size(), 2u);
    EXPECT_TRUE(byProcess.find(4120) != byProcess.end());
    EXPECT_TRUE(byProcess.find(2204) != byProcess.end());
    EXPECT_EQ(byProcess.at(4120).size(), 1u);
    EXPECT_EQ(byProcess.at(2204).size(), 1u);
}

static void testResetClearsState()
{
    TimelineEventCollector collector;
    collector.collectActiveProcesses({ProcessInfo(4120, "python.exe", 1)});
    EXPECT_EQ(collector.eventCount(), 1u);

    collector.reset();
    EXPECT_EQ(collector.eventCount(), 0u);
    EXPECT_TRUE(collector.getAllEvents().empty());

    // After reset, the same process_seen can be collected again.
    EXPECT_EQ(
        collector.collectActiveProcesses({ProcessInfo(4120, "python.exe", 1)}),
        1u);
}

int main()
{
    testEmptyInputDoesNotCrash();
    testMultipleEventsForOneProcess();
    testNetworkEventsAttachToCorrectProcess();
    testAvoidsDuplicateEvents();
    testUploadAndDownloadCollection();
    testGroupedByPidMap();
    testResetClearsState();

    if (g_failures == 0)
    {
        std::cout << "All TimelineEventCollector tests passed.\n";
        return 0;
    }

    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
