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
#include "../../src/process/models/ProcessActivitySummary.h"
#include "../../src/process/models/ProcessActivitySummary.cpp"
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
                      << ":" << __LINE__ << "\n";                              \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

#define EXPECT_CONTAINS(text, fragment)                                        \
    do                                                                         \
    {                                                                          \
        if ((text).find(fragment) == std::string::npos)                        \
        {                                                                      \
            std::cerr << "FAIL: expected `" << (fragment)                       \
                      << "` in summary at " << __FILE__ << ":" << __LINE__     \
                      << "\nGot:\n"                                            \
                      << (text) << "\n";                                       \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

#define EXPECT_NOT_CONTAINS(text, fragment)                                    \
    do                                                                         \
    {                                                                          \
        if ((text).find(fragment) != std::string::npos)                        \
        {                                                                      \
            std::cerr << "FAIL: unexpected `" << (fragment)                     \
                      << "` in summary at " << __FILE__ << ":" << __LINE__     \
                      << "\nGot:\n"                                            \
                      << (text) << "\n";                                       \
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
    uint64_t uploaded = 0,
    uint64_t downloaded = 0,
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
        uploaded,
        downloaded,
        description);
}

static std::vector<ProcessActivityEvent> samplePythonEvents()
{
    const auto t0 = makeUtcTime(2024, 1, 15, 10, 0, 0);
    const auto t1 = makeUtcTime(2024, 1, 15, 10, 1, 0);
    const auto t2 = makeUtcTime(2024, 1, 15, 10, 5, 0);
    const auto t3 = makeUtcTime(2024, 1, 15, 10, 8, 0);
    const auto t4 = makeUtcTime(2024, 1, 15, 10, 12, 0);

    const uint64_t uploadMain =
        static_cast<uint64_t>(25.4 * 1024.0 * 1024.0); // ~25.4 MB
    const uint64_t uploadSecondary = 512ull * 1024ull;
    const uint64_t downloadMain = static_cast<uint64_t>(1.2 * 1024.0 * 1024.0);

    return {
        makeEvent(t0, 4120, "python.exe", ProcessActivityEventType::PROCESS_SEEN),
        makeEvent(
            t1,
            4120,
            "python.exe",
            ProcessActivityEventType::CONNECTION_OPENED,
            "api.example.com:443"),
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
            ProcessActivityEventType::CONNECTION_OPENED,
            "cdn.example.net:443"),
        makeEvent(
            t3,
            4120,
            "python.exe",
            ProcessActivityEventType::UPLOAD_ACTIVITY,
            "api.example.com:443",
            uploadMain),
        makeEvent(
            t3,
            4120,
            "python.exe",
            ProcessActivityEventType::UPLOAD_ACTIVITY,
            "104.18.32.45:443",
            uploadSecondary),
        makeEvent(
            t4,
            4120,
            "python.exe",
            ProcessActivityEventType::DOWNLOAD_ACTIVITY,
            "api.example.com:443",
            0,
            downloadMain)};
}

static void testExampleStyleSummary()
{
    TimelineSummaryBuilder builder(samplePythonEvents());
    const ProcessActivitySummary summary = builder.forProcessId(4120);
    const std::string text = summary.toSummaryString();

    EXPECT_EQ(summary.processName, std::string("python.exe"));
    EXPECT_EQ(summary.remoteEndpointCount, 3u);
    EXPECT_EQ(summary.connectionOpenCount, 3u);
    EXPECT_EQ(summary.mostActiveDestination, std::string("api.example.com:443"));
    EXPECT_TRUE(summary.activeDuration().count() == 12 * 60);

    EXPECT_CONTAINS(text, "python.exe was active for 12 minutes.");
    EXPECT_CONTAINS(text, "It connected to 3 remote endpoints.");
    EXPECT_CONTAINS(text, "Total upload: 25.9 MB.");
    EXPECT_CONTAINS(text, "Total download: 1.2 MB.");
    EXPECT_CONTAINS(text, "Most active destination: api.example.com:443.");
}

static void testPeakBytesPerDestination()
{
    const auto t1 = makeUtcTime(2024, 1, 15, 11, 0, 0);
    const auto t2 = makeUtcTime(2024, 1, 15, 11, 1, 0);

    std::vector<ProcessActivityEvent> events = {
        makeEvent(
            t1,
            100,
            "curl.exe",
            ProcessActivityEventType::UPLOAD_ACTIVITY,
            "files.example.com:443",
            1024ull * 1024ull),
        makeEvent(
            t2,
            100,
            "curl.exe",
            ProcessActivityEventType::UPLOAD_ACTIVITY,
            "files.example.com:443",
            8ull * 1024ull * 1024ull)};

    TimelineSummaryBuilder builder(events);
    const ProcessActivitySummary summary = builder.forProcessId(100);

    EXPECT_EQ(summary.totalUploadedBytes, 8ull * 1024ull * 1024ull);
    EXPECT_EQ(summary.remoteEndpointCount, 1u);
}

static void testBehaviorHintsWithoutAlerts()
{
    const auto t0 = makeUtcTime(2024, 1, 15, 12, 0, 0);
    std::vector<ProcessActivityEvent> events;

    for (int i = 0; i < 8; ++i)
    {
        events.push_back(makeEvent(
            t0 + std::chrono::minutes(i),
            77,
            "agent.exe",
            ProcessActivityEventType::CONNECTION_OPENED,
            "host" + std::to_string(i) + ".example.com:443"));
    }

    for (int i = 0; i < 5; ++i)
    {
        events.push_back(makeEvent(
            t0 + std::chrono::minutes(i),
            77,
            "agent.exe",
            ProcessActivityEventType::UPLOAD_ACTIVITY,
            "host0.example.com:443",
            20ull * 1024ull * 1024ull));
    }

    events.push_back(makeEvent(
        t0 + std::chrono::minutes(9),
        77,
        "agent.exe",
        ProcessActivityEventType::SUSPICIOUS_ACTIVITY,
        "host0.example.com:443",
        0,
        0,
        "Unusual sustained upload"));

    TimelineSummaryBuilder builder(events);
    const ProcessActivitySummary summary = builder.forProcessId(77);
    const std::string text = summary.toSummaryString();

    EXPECT_TRUE(summary.behaviorHints.size() >= 3u);
    EXPECT_CONTAINS(text, "Hint: Elevated outbound transfer volume observed.");
    EXPECT_CONTAINS(text, "Hint: Connected to many distinct remote endpoints.");
    EXPECT_CONTAINS(
        text,
        "Hint: Timeline includes suspicious_activity observations");
    EXPECT_NOT_CONTAINS(text, "Alert");
    EXPECT_NOT_CONTAINS(text, "ALERT");
}

static void testForProcessNameAndAllProcesses()
{
    auto events = samplePythonEvents();
    events.push_back(makeEvent(
        makeUtcTime(2024, 1, 15, 10, 20, 0),
        2204,
        "chrome.exe",
        ProcessActivityEventType::CONNECTION_OPENED,
        "cdn.google.com:443"));

    TimelineSummaryBuilder builder(events);

    const ProcessActivitySummary python = builder.forProcessName("python.exe");
    EXPECT_EQ(python.processId, 4120u);
    EXPECT_EQ(python.remoteEndpointCount, 3u);

    const auto all = builder.forAllProcesses();
    EXPECT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0].processId, 2204u);
    EXPECT_EQ(all[1].processId, 4120u);
}

static void testFromHistory()
{
    TimelineEventHistory history(100, std::chrono::seconds(0));
    history.add(samplePythonEvents());

    TimelineSummaryBuilder builder(history);
    EXPECT_EQ(builder.eventCount(), 7u);
    EXPECT_EQ(builder.forProcessId(4120).remoteEndpointCount, 3u);
}

static void testEmptyIsSafe()
{
    TimelineSummaryBuilder builder(std::vector<ProcessActivityEvent>{});
    EXPECT_EQ(builder.eventCount(), 0u);
    EXPECT_TRUE(builder.forProcessId(1).empty());
    EXPECT_TRUE(builder.forProcessName("x").empty());
    EXPECT_TRUE(builder.forAllProcesses().empty());
    EXPECT_CONTAINS(
        builder.forProcessId(1).toSummaryString(),
        "No process activity to summarize.");
}

static void testDurationFormatting()
{
    EXPECT_EQ(
        ProcessActivitySummary::formatDuration(std::chrono::seconds(0)),
        std::string("less than a minute"));
    EXPECT_EQ(
        ProcessActivitySummary::formatDuration(std::chrono::seconds(1)),
        std::string("1 second"));
    EXPECT_EQ(
        ProcessActivitySummary::formatDuration(std::chrono::seconds(45)),
        std::string("45 seconds"));
    EXPECT_EQ(
        ProcessActivitySummary::formatDuration(std::chrono::seconds(60)),
        std::string("1 minute"));
    EXPECT_EQ(
        ProcessActivitySummary::formatDuration(std::chrono::seconds(12 * 60)),
        std::string("12 minutes"));
    EXPECT_EQ(
        ProcessActivitySummary::formatDuration(std::chrono::seconds(90 * 60)),
        std::string("1 hour 30 minutes"));
}

int main()
{
    testExampleStyleSummary();
    testPeakBytesPerDestination();
    testBehaviorHintsWithoutAlerts();
    testForProcessNameAndAllProcesses();
    testFromHistory();
    testEmptyIsSafe();
    testDurationFormatting();

    if (g_failures == 0)
    {
        std::cout << "All TimelineSummaryBuilder tests passed.\n";
        return 0;
    }

    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
