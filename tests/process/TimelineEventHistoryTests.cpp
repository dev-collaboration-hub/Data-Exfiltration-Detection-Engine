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
#include "../../src/process/tracker/TimelineEventHistory.h"
#include "../../src/process/tracker/TimelineEventHistory.cpp"

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
    ProcessActivityEventType type = ProcessActivityEventType::PROCESS_SEEN)
{
    return ProcessActivityEvent(
        when,
        pid,
        name,
        type,
        "",
        "",
        ProtocolType::UNKNOWN,
        0,
        0,
        "");
}

static void testStoresRecentEvents()
{
    TimelineEventHistory history(100, std::chrono::seconds(0));
    const auto now = std::chrono::system_clock::now();

    history.add(makeEvent(now, 4120, "python.exe"));
    history.add(makeEvent(
        now + std::chrono::seconds(1),
        4120,
        "python.exe",
        ProcessActivityEventType::CONNECTION_OPENED));

    EXPECT_EQ(history.size(), 2u);
    EXPECT_EQ(history.getRecent().size(), 2u);
    EXPECT_TRUE(
        history.getRecent()[0].eventType == ProcessActivityEventType::PROCESS_SEEN);
    EXPECT_TRUE(
        history.getRecent()[1].eventType ==
        ProcessActivityEventType::CONNECTION_OPENED);
}

static void testEnforcesMaximumEventLimit()
{
    TimelineEventHistory history(3, std::chrono::seconds(0));
    const auto base = makeUtcTime(2024, 1, 15, 10, 0, 0);

    history.add(makeEvent(base + std::chrono::seconds(1), 1, "a.exe"));
    history.add(makeEvent(base + std::chrono::seconds(2), 2, "b.exe"));
    history.add(makeEvent(base + std::chrono::seconds(3), 3, "c.exe"));
    history.add(makeEvent(base + std::chrono::seconds(4), 4, "d.exe"));
    history.add(makeEvent(base + std::chrono::seconds(5), 5, "e.exe"));

    EXPECT_EQ(history.size(), 3u);

    const auto recent = history.getRecent();
    EXPECT_EQ(recent.size(), 3u);
    EXPECT_EQ(recent[0].processId, 3u);
    EXPECT_EQ(recent[1].processId, 4u);
    EXPECT_EQ(recent[2].processId, 5u);
}

static void testCleanupRemovesOldEvents()
{
    TimelineEventHistory history(100, std::chrono::seconds(60));
    const auto now = std::chrono::system_clock::now();

    history.add(makeEvent(now - std::chrono::seconds(120), 4120, "python.exe"));
    history.add(makeEvent(now - std::chrono::seconds(30), 4120, "python.exe",
                          ProcessActivityEventType::CONNECTION_OPENED));
    history.add(makeEvent(now, 2204, "chrome.exe"));

    EXPECT_EQ(history.size(), 2u);

    const auto python = history.getByProcessId(4120);
    EXPECT_EQ(python.size(), 1u);
    EXPECT_TRUE(
        python[0].eventType == ProcessActivityEventType::CONNECTION_OPENED);
}

static void testQueryByPidAndProcessName()
{
    TimelineEventHistory history(100, std::chrono::seconds(0));
    const auto base = makeUtcTime(2024, 1, 15, 10, 1, 0);

    history.add(makeEvent(base, 4120, "python.exe"));
    history.add(makeEvent(
        base + std::chrono::seconds(5),
        4120,
        "python.exe",
        ProcessActivityEventType::UPLOAD_ACTIVITY));
    history.add(makeEvent(base + std::chrono::seconds(2), 2204, "chrome.exe"));

    const auto byPid = history.getByProcessId(4120);
    EXPECT_EQ(byPid.size(), 2u);
    EXPECT_EQ(byPid[0].processId, 4120u);
    EXPECT_EQ(byPid[1].processId, 4120u);
    EXPECT_TRUE(byPid[0].timestamp < byPid[1].timestamp);

    const auto byName = history.getByProcessName("python.exe");
    EXPECT_EQ(byName.size(), 2u);
    EXPECT_EQ(byName[0].processName, std::string("python.exe"));

    const auto chrome = history.getByProcessName("chrome.exe");
    EXPECT_EQ(chrome.size(), 1u);
    EXPECT_EQ(chrome[0].processId, 2204u);

    EXPECT_TRUE(history.getByProcessId(9999).empty());
    EXPECT_TRUE(history.getByProcessName("missing.exe").empty());
}

static void testAddFromCollectorAndClear()
{
    TimelineEventCollector collector;
    collector.collectActiveProcesses(
        {ProcessInfo(4120, "python.exe", 1),
         ProcessInfo(2204, "chrome.exe", 1)});

    TimelineEventHistory history(100, std::chrono::seconds(0));
    history.add(collector);

    EXPECT_EQ(history.size(), 2u);
    EXPECT_EQ(history.getByProcessName("python.exe").size(), 1u);

    history.clear();
    EXPECT_EQ(history.size(), 0u);
    EXPECT_TRUE(history.getRecent().empty());
}

static void testSetMaxEventsTriggersCleanup()
{
    TimelineEventHistory history(10, std::chrono::seconds(0));
    const auto base = makeUtcTime(2024, 1, 15, 11, 0, 0);

    for (uint32_t i = 0; i < 5; ++i)
    {
        history.add(makeEvent(base + std::chrono::seconds(i), i + 1, "app.exe"));
    }

    EXPECT_EQ(history.size(), 5u);
    history.setMaxEvents(2);
    EXPECT_EQ(history.size(), 2u);

    const auto recent = history.getRecent();
    EXPECT_EQ(recent[0].processId, 4u);
    EXPECT_EQ(recent[1].processId, 5u);
}

static void testEmptyHistoryIsSafe()
{
    TimelineEventHistory history;
    EXPECT_EQ(history.size(), 0u);
    EXPECT_EQ(history.cleanup(), 0u);
    EXPECT_TRUE(history.getByProcessId(1).empty());
    EXPECT_TRUE(history.getByProcessName("x").empty());
    EXPECT_TRUE(history.getRecent().empty());
}

int main()
{
    testStoresRecentEvents();
    testEnforcesMaximumEventLimit();
    testCleanupRemovesOldEvents();
    testQueryByPidAndProcessName();
    testAddFromCollectorAndClear();
    testSetMaxEventsTriggersCleanup();
    testEmptyHistoryIsSafe();

    if (g_failures == 0)
    {
        std::cout << "All TimelineEventHistory tests passed.\n";
        return 0;
    }

    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
