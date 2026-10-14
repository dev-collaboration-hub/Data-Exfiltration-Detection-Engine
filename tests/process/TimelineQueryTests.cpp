#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "../../src/network/utils/NetworkUtils.h"
#include "../../src/network/utils/NetworkUtils.cpp"
#include "../../src/network/models/Connection.h"
#include "../../src/network/models/Connection.cpp"
#include "../../src/process/events/ProcessActivityEvent.h"
#include "../../src/process/events/ProcessActivityEvent.cpp"
#include "../../src/process/events/ProcessEvent.h"
#include "../../src/process/events/ProcessEvent.cpp"
#include "../../src/process/models/ProcessInfo.h"
#include "../../src/process/models/ProcessInfo.cpp"
#include "../../src/network/events/ConnectionEvent.h"
#include "../../src/network/events/ConnectionEvent.cpp"
#include "../../src/network/models/ConnectionUploadStats.h"
#include "../../src/network/models/ConnectionUploadStats.cpp"
#include "../../src/network/models/ConnectionDownloadStats.h"
#include "../../src/network/models/ConnectionDownloadStats.cpp"
#include "../../src/process/tracker/TimelineEventCollector.h"
#include "../../src/process/tracker/TimelineEventCollector.cpp"
#include "../../src/process/tracker/ProcessTimelineBuilder.h"
#include "../../src/process/tracker/ProcessTimelineBuilder.cpp"
#include "../../src/process/tracker/TimelineEventHistory.h"
#include "../../src/process/tracker/TimelineEventHistory.cpp"
#include "../../src/process/tracker/TimelineQuery.h"
#include "../../src/process/tracker/TimelineQuery.cpp"

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

static ProcessActivityEvent makeEvent(
    std::chrono::system_clock::time_point when,
    uint32_t pid,
    const std::string& name,
    ProcessActivityEventType type,
    const std::string& remote = "",
    const std::string& description = "")
{
    return ProcessActivityEvent(
        when,
        pid,
        name,
        type,
        "192.168.1.10:53142",
        remote,
        ProtocolType::TCP,
        0,
        0,
        description);
}

static std::vector<ProcessActivityEvent> sampleEvents()
{
    const auto t1 = makeUtcTime(2024, 1, 15, 10, 1, 2);
    const auto t2 = makeUtcTime(2024, 1, 15, 10, 1, 10);
    const auto t3 = makeUtcTime(2024, 1, 15, 10, 1, 15);
    const auto t4 = makeUtcTime(2024, 1, 15, 10, 1, 20);

    return {
        makeEvent(t1, 4120, "python.exe", ProcessActivityEventType::PROCESS_SEEN),
        makeEvent(
            t2,
            4120,
            "python.exe",
            ProcessActivityEventType::CONNECTION_OPENED,
            "104.18.32.45:443"),
        makeEvent(
            t3,
            4120,
            "python.exe",
            ProcessActivityEventType::UPLOAD_ACTIVITY,
            "104.18.32.45:443"),
        makeEvent(
            t4,
            2204,
            "chrome.exe",
            ProcessActivityEventType::CONNECTION_OPENED,
            "api.example.com:443",
            "Connected to api.example.com")};
}

static void testQueryByPid()
{
    TimelineQuery query(sampleEvents());
    const auto events = query.byProcessId(4120);

    EXPECT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0].processId, 4120u);
    EXPECT_EQ(events[1].processId, 4120u);
    EXPECT_EQ(events[2].processId, 4120u);
    EXPECT_TRUE(events[0].timestamp <= events[1].timestamp);
    EXPECT_TRUE(query.byProcessId(9999).empty());
}

static void testQueryByProcessName()
{
    TimelineQuery query(sampleEvents());
    const auto python = query.byProcessName("python.exe");
    const auto chrome = query.byProcessName("chrome.exe");

    EXPECT_EQ(python.size(), 3u);
    EXPECT_EQ(chrome.size(), 1u);
    EXPECT_EQ(chrome[0].processId, 2204u);
    EXPECT_TRUE(query.byProcessName("missing.exe").empty());
}

static void testQueryByRemoteDestination()
{
    TimelineQuery query(sampleEvents());

    const auto byIp = query.byRemoteDestination("104.18.32.45");
    EXPECT_EQ(byIp.size(), 2u);
    EXPECT_EQ(byIp[0].remoteAddress, std::string("104.18.32.45:443"));

    const auto byEndpoint = query.byRemoteDestination("104.18.32.45:443");
    EXPECT_EQ(byEndpoint.size(), 2u);

    const auto byDomain = query.byRemoteDestination("api.example.com");
    EXPECT_EQ(byDomain.size(), 1u);
    EXPECT_EQ(byDomain[0].processName, std::string("chrome.exe"));

    EXPECT_TRUE(query.byRemoteDestination("10.0.0.1").empty());
    EXPECT_TRUE(query.byRemoteDestination("").empty());
}

static void testQueryByEventType()
{
    TimelineQuery query(sampleEvents());

    const auto opened =
        query.byEventType(ProcessActivityEventType::CONNECTION_OPENED);
    EXPECT_EQ(opened.size(), 2u);

    const auto uploads =
        query.byEventType(ProcessActivityEventType::UPLOAD_ACTIVITY);
    EXPECT_EQ(uploads.size(), 1u);
    EXPECT_EQ(uploads[0].processId, 4120u);

    EXPECT_TRUE(
        query.byEventType(ProcessActivityEventType::SUSPICIOUS_ACTIVITY).empty());
}

static void testQueryRecentN()
{
    TimelineQuery query(sampleEvents());

    const auto all = query.recent(0);
    EXPECT_EQ(all.size(), 4u);

    const auto lastTwo = query.recent(2);
    EXPECT_EQ(lastTwo.size(), 2u);
    EXPECT_EQ(lastTwo[0].processId, 4120u);
    EXPECT_TRUE(
        lastTwo[0].eventType == ProcessActivityEventType::UPLOAD_ACTIVITY);
    EXPECT_EQ(lastTwo[1].processId, 2204u);
}

static void testQueryFromHistory()
{
    TimelineEventHistory history(100, std::chrono::seconds(0));
    history.add(sampleEvents());

    TimelineQuery query(history);
    EXPECT_EQ(query.size(), 4u);
    EXPECT_EQ(query.byProcessId(4120).size(), 3u);
    EXPECT_EQ(query.byRemoteDestination("104.18.32.45").size(), 2u);
    EXPECT_EQ(query.recent(1).size(), 1u);
}

static void testEmptyQueryIsSafe()
{
    TimelineQuery query(std::vector<ProcessActivityEvent>{});
    EXPECT_EQ(query.size(), 0u);
    EXPECT_TRUE(query.byProcessId(1).empty());
    EXPECT_TRUE(query.byProcessName("x").empty());
    EXPECT_TRUE(query.byRemoteDestination("1.2.3.4").empty());
    EXPECT_TRUE(query.byEventType(ProcessActivityEventType::PROCESS_SEEN).empty());
    EXPECT_TRUE(query.recent(5).empty());
}

int main()
{
    testQueryByPid();
    testQueryByProcessName();
    testQueryByRemoteDestination();
    testQueryByEventType();
    testQueryRecentN();
    testQueryFromHistory();
    testEmptyQueryIsSafe();

    if (g_failures == 0)
    {
        std::cout << "All TimelineQuery tests passed.\n";
        return 0;
    }

    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
