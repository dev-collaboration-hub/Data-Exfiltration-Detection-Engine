#include <chrono>
#include <ctime>
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
#include "../../src/process/models/ProcessActivitySummary.h"
#include "../../src/process/models/ProcessActivitySummary.cpp"
#include "../../src/process/events/ProcessEvent.h"
#include "../../src/process/events/ProcessEvent.cpp"
#include "../../src/process/events/ProcessActivityEvent.h"
#include "../../src/process/events/ProcessActivityEvent.cpp"
#include "../../src/process/tracker/TimelineEventCollector.h"
#include "../../src/process/tracker/TimelineEventCollector.cpp"
#include "../../src/process/tracker/ProcessTimelineBuilder.h"
#include "../../src/process/tracker/ProcessTimelineBuilder.cpp"
#include "../../src/process/tracker/TimelineEventHistory.h"
#include "../../src/process/tracker/TimelineEventHistory.cpp"
#include "../../src/process/tracker/TimelineSummaryBuilder.h"
#include "../../src/process/tracker/TimelineSummaryBuilder.cpp"

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
                      << __LINE__ << "\n";                                     \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

#define EXPECT_CONTAINS(haystack, needle)                                      \
    do                                                                         \
    {                                                                          \
        const std::string& _haystack = (haystack);                             \
        const std::string _needle = (needle);                                  \
        if (_haystack.find(_needle) == std::string::npos)                      \
        {                                                                      \
            std::cerr << "FAIL: expected to find \"" << _needle                \
                      << "\" in \"" << _haystack << "\" at " << __FILE__       \
                      << ":" << __LINE__ << "\n";                              \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

static std::chrono::system_clock::time_point makeUtcTime(
    int year, int month, int day, int hour, int minute, int second = 0)
{
    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;

#if defined(_WIN32)
    const std::time_t value = _mkgmtime(&tm);
#else
    const std::time_t value = timegm(&tm);
#endif

    return std::chrono::system_clock::from_time_t(value);
}

static Connection makeConnection(
    uint32_t pid,
    const std::string& localIp,
    uint16_t localPort,
    const std::string& remoteIp,
    uint16_t remotePort,
    std::chrono::system_clock::time_point when)
{
    return Connection(
        pid,
        localIp,
        localPort,
        remoteIp,
        remotePort,
        ProtocolType::TCP,
        ConnectionState::ESTABLISHED,
        when);
}

/**
 * Builds the shared fake incident used by system tests and the sample report.
 *
 * Scenario:
 *   python.exe (4120) starts, opens one remote connection, uploads 12.5 MB,
 *   downloads 3.2 MB, then closes the connection.
 *   chrome.exe (2204) starts later and opens a separate browsing connection.
 *
 * Events are intentionally inserted out of chronological order so sorting is
 * exercised end-to-end through the collector and timeline builder.
 */
static TimelineEventCollector buildSampleIncidentCollector()
{
    TimelineEventCollector collector;

    const auto tPythonSeen = makeUtcTime(2024, 1, 15, 10, 1, 2);
    const auto tPythonOpen = makeUtcTime(2024, 1, 15, 10, 1, 10);
    const auto tPythonUpload = makeUtcTime(2024, 1, 15, 10, 1, 15);
    const auto tPythonDownload = makeUtcTime(2024, 1, 15, 10, 1, 20);
    const auto tPythonClose = makeUtcTime(2024, 1, 15, 10, 1, 30);
    const auto tChromeSeen = makeUtcTime(2024, 1, 15, 10, 1, 5);
    const auto tChromeOpen = makeUtcTime(2024, 1, 15, 10, 1, 12);

    ProcessInfo python(4120, "python.exe", 1000);
    ProcessInfo chrome(2204, "chrome.exe", 800);

    // Fake process events (out of chronological order on purpose).
    ProcessEvent chromeCreated(ProcessEventType::CREATED, chrome, tChromeSeen);
    ProcessEvent pythonCreated(ProcessEventType::CREATED, python, tPythonSeen);

    Connection pythonConn = makeConnection(
        4120,
        "192.168.1.10",
        53142,
        "104.18.32.45",
        443,
        tPythonOpen);
    Connection chromeConn = makeConnection(
        2204,
        "192.168.1.10",
        53150,
        "142.250.190.78",
        443,
        tChromeOpen);

    // Fake network events (also shuffled).
    ConnectionEvent pythonOpened(
        ConnectionEventType::CREATED, pythonConn, tPythonOpen);
    ConnectionEvent chromeOpened(
        ConnectionEventType::CREATED, chromeConn, tChromeOpen);
    ConnectionEvent pythonClosed(
        ConnectionEventType::CLOSED, pythonConn, tPythonClose);

    ConnectionUploadStats pythonUpload(
        4120,
        "python.exe",
        "104.18.32.45",
        443,
        ProtocolType::TCP,
        13107200ull, // 12.5 MB
        tPythonOpen,
        tPythonUpload,
        true);

    ConnectionDownloadStats pythonDownload(
        4120,
        "python.exe",
        "104.18.32.45",
        443,
        ProtocolType::TCP,
        3355443ull, // 3.2 MB
        tPythonOpen,
        tPythonDownload,
        true);

    std::unordered_map<uint32_t, ProcessInfo> lookup = {
        {4120, python},
        {2204, chrome}};

    collector.collect(
        {chromeCreated, pythonCreated},
        {pythonClosed, chromeOpened, pythonOpened},
        {pythonUpload},
        {pythonDownload},
        lookup);

    return collector;
}

static void testFakeProcessEvents()
{
    TimelineEventCollector collector;

    const auto t1 = makeUtcTime(2024, 1, 15, 10, 0, 1);
    const auto t2 = makeUtcTime(2024, 1, 15, 10, 0, 2);

    ProcessInfo python(4120, "python.exe", 1);
    ProcessInfo chrome(2204, "chrome.exe", 1);

    ProcessEvent pythonCreated(ProcessEventType::CREATED, python, t1);
    ProcessEvent chromeCreated(ProcessEventType::CREATED, chrome, t2);
    ProcessEvent pythonTerminated(ProcessEventType::TERMINATED, python, t2);

    const std::size_t added =
        collector.collectProcessEvents({pythonCreated, chromeCreated, pythonTerminated});

    // TERMINATED is ignored by the collector; only CREATED becomes PROCESS_SEEN.
    EXPECT_EQ(added, 2u);
    EXPECT_EQ(collector.eventCount(), 2u);

    const auto pythonEvents = collector.getEventsForProcess(4120);
    const auto chromeEvents = collector.getEventsForProcess(2204);

    EXPECT_EQ(pythonEvents.size(), 1u);
    EXPECT_EQ(chromeEvents.size(), 1u);
    EXPECT_TRUE(pythonEvents[0].eventType == ProcessActivityEventType::PROCESS_SEEN);
    EXPECT_TRUE(chromeEvents[0].eventType == ProcessActivityEventType::PROCESS_SEEN);
    EXPECT_EQ(pythonEvents[0].processName, std::string("python.exe"));
    EXPECT_EQ(chromeEvents[0].processName, std::string("chrome.exe"));
}

static void testFakeNetworkEvents()
{
    TimelineEventCollector collector;
    const auto tOpen = makeUtcTime(2024, 1, 15, 10, 2, 0);
    const auto tUpload = makeUtcTime(2024, 1, 15, 10, 2, 5);
    const auto tDownload = makeUtcTime(2024, 1, 15, 10, 2, 8);
    const auto tClose = makeUtcTime(2024, 1, 15, 10, 2, 15);

    ProcessInfo python(4120, "python.exe", 1);
    Connection connection = makeConnection(
        4120, "192.168.1.10", 53142, "104.18.32.45", 443, tOpen);

    ConnectionEvent opened(ConnectionEventType::CREATED, connection, tOpen);
    ConnectionEvent closed(ConnectionEventType::CLOSED, connection, tClose);

    ConnectionUploadStats upload(
        4120,
        "python.exe",
        "104.18.32.45",
        443,
        ProtocolType::TCP,
        13107200ull,
        tOpen,
        tUpload,
        true);

    ConnectionDownloadStats download(
        4120,
        "python.exe",
        "104.18.32.45",
        443,
        ProtocolType::TCP,
        3355443ull,
        tOpen,
        tDownload,
        true);

    std::unordered_map<uint32_t, ProcessInfo> lookup = {{4120, python}};

    const std::size_t added = collector.collect(
        {},
        {opened, closed},
        {upload},
        {download},
        lookup);

    EXPECT_EQ(added, 4u);

    const auto events = collector.getEventsForProcess(4120);
    EXPECT_EQ(events.size(), 4u);
    EXPECT_TRUE(events[0].eventType == ProcessActivityEventType::CONNECTION_OPENED);
    EXPECT_TRUE(events[1].eventType == ProcessActivityEventType::CONNECTION_CLOSED);
    EXPECT_TRUE(events[2].eventType == ProcessActivityEventType::UPLOAD_ACTIVITY);
    EXPECT_TRUE(events[3].eventType == ProcessActivityEventType::DOWNLOAD_ACTIVITY);
    EXPECT_EQ(events[0].remoteAddress, std::string("104.18.32.45:443"));
    EXPECT_EQ(events[2].uploadedBytes, 13107200ull);
    EXPECT_EQ(events[3].downloadedBytes, 3355443ull);
}

static void testEventSorting()
{
    TimelineEventCollector collector = buildSampleIncidentCollector();

    ProcessTimelineBuilder builder;
    builder.build(collector);

    const auto& global = builder.getGlobalTimeline();
    EXPECT_EQ(global.size(), 7u);

    // Expected chronological order across both processes.
    EXPECT_TRUE(global[0].eventType == ProcessActivityEventType::PROCESS_SEEN);
    EXPECT_EQ(global[0].processId, 4120u); // 10:01:02 python

    EXPECT_TRUE(global[1].eventType == ProcessActivityEventType::PROCESS_SEEN);
    EXPECT_EQ(global[1].processId, 2204u); // 10:01:05 chrome

    EXPECT_TRUE(global[2].eventType == ProcessActivityEventType::CONNECTION_OPENED);
    EXPECT_EQ(global[2].processId, 4120u); // 10:01:10 python

    EXPECT_TRUE(global[3].eventType == ProcessActivityEventType::CONNECTION_OPENED);
    EXPECT_EQ(global[3].processId, 2204u); // 10:01:12 chrome

    EXPECT_TRUE(global[4].eventType == ProcessActivityEventType::UPLOAD_ACTIVITY);
    EXPECT_EQ(global[4].processId, 4120u); // 10:01:15

    EXPECT_TRUE(global[5].eventType == ProcessActivityEventType::DOWNLOAD_ACTIVITY);
    EXPECT_EQ(global[5].processId, 4120u); // 10:01:20

    EXPECT_TRUE(global[6].eventType == ProcessActivityEventType::CONNECTION_CLOSED);
    EXPECT_EQ(global[6].processId, 4120u); // 10:01:30

    for (std::size_t i = 1; i < global.size(); ++i)
    {
        EXPECT_TRUE(global[i - 1].timestamp <= global[i].timestamp);
    }
}

static void testPerProcessTimelineGeneration()
{
    TimelineEventCollector collector = buildSampleIncidentCollector();

    ProcessTimelineBuilder builder;
    builder.build(collector);

    const auto python = builder.getProcessTimeline(4120);
    const auto chrome = builder.getProcessTimeline(2204);
    const auto missing = builder.getProcessTimeline(9999);

    EXPECT_EQ(python.size(), 5u);
    EXPECT_EQ(chrome.size(), 2u);
    EXPECT_TRUE(missing.empty());

    for (const auto& event : python)
    {
        EXPECT_EQ(event.processId, 4120u);
        EXPECT_EQ(event.processName, std::string("python.exe"));
    }

    for (const auto& event : chrome)
    {
        EXPECT_EQ(event.processId, 2204u);
        EXPECT_EQ(event.processName, std::string("chrome.exe"));
    }

    EXPECT_TRUE(python[0].eventType == ProcessActivityEventType::PROCESS_SEEN);
    EXPECT_TRUE(python[1].eventType == ProcessActivityEventType::CONNECTION_OPENED);
    EXPECT_TRUE(python[2].eventType == ProcessActivityEventType::UPLOAD_ACTIVITY);
    EXPECT_TRUE(python[3].eventType == ProcessActivityEventType::DOWNLOAD_ACTIVITY);
    EXPECT_TRUE(python[4].eventType == ProcessActivityEventType::CONNECTION_CLOSED);

    EXPECT_TRUE(chrome[0].eventType == ProcessActivityEventType::PROCESS_SEEN);
    EXPECT_TRUE(chrome[1].eventType == ProcessActivityEventType::CONNECTION_OPENED);

    for (std::size_t i = 1; i < python.size(); ++i)
    {
        EXPECT_TRUE(python[i - 1].timestamp <= python[i].timestamp);
    }
}

static void testSampleTimelineReportOutput()
{
    TimelineEventCollector collector = buildSampleIncidentCollector();

    ProcessTimelineBuilder builder;
    builder.build(collector);

    const std::string expectedGlobal =
        "10:01:02 | python.exe | Process Seen\n"
        "10:01:05 | chrome.exe | Process Seen\n"
        "10:01:10 | python.exe | Connected to 104.18.32.45:443\n"
        "10:01:12 | chrome.exe | Connected to 142.250.190.78:443\n"
        "10:01:15 | python.exe | Upload Activity: 12.5 MB\n"
        "10:01:20 | python.exe | Download Activity: 3.2 MB\n"
        "10:01:30 | python.exe | Disconnected from 104.18.32.45:443";

    const std::string expectedPython =
        "10:01:02 | python.exe | Process Seen\n"
        "10:01:10 | python.exe | Connected to 104.18.32.45:443\n"
        "10:01:15 | python.exe | Upload Activity: 12.5 MB\n"
        "10:01:20 | python.exe | Download Activity: 3.2 MB\n"
        "10:01:30 | python.exe | Disconnected from 104.18.32.45:443";

    const std::string expectedChrome =
        "10:01:05 | chrome.exe | Process Seen\n"
        "10:01:12 | chrome.exe | Connected to 142.250.190.78:443";

    EXPECT_EQ(builder.formatGlobalTimeline(), expectedGlobal);
    EXPECT_EQ(builder.formatProcessTimeline(4120), expectedPython);
    EXPECT_EQ(builder.formatProcessTimeline(2204), expectedChrome);

    TimelineSummaryBuilder summaryBuilder(builder);
    const ProcessActivitySummary pythonSummary = summaryBuilder.forProcessId(4120);
    const std::string summaryText = pythonSummary.toSummaryString();

    EXPECT_CONTAINS(summaryText, "python.exe was active for");
    EXPECT_CONTAINS(summaryText, "It connected to 1 remote endpoint.");
    EXPECT_CONTAINS(summaryText, "Total upload: 12.5 MB.");
    EXPECT_CONTAINS(summaryText, "Total download: 3.2 MB.");
    EXPECT_CONTAINS(summaryText, "Most active destination: 104.18.32.45:443.");
    EXPECT_CONTAINS(summaryText, "Hint: Elevated outbound transfer volume observed.");
    EXPECT_TRUE(summaryText.find("ALERT") == std::string::npos);
}

int main()
{
    testFakeProcessEvents();
    testFakeNetworkEvents();
    testEventSorting();
    testPerProcessTimelineGeneration();
    testSampleTimelineReportOutput();

    if (g_failures == 0)
    {
        std::cout << "All TimelineSystem tests passed.\n";
        return 0;
    }

    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
