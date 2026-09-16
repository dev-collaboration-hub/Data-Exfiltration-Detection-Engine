#include <chrono>
#include <ctime>
#include <iostream>
#include <string>

#include "../../src/network/models/Connection.h"
#include "../../src/network/models/Connection.cpp"
#include "../../src/network/utils/NetworkUtils.h"
#include "../../src/network/utils/NetworkUtils.cpp"
#include "../../src/process/events/ProcessActivityEvent.h"
#include "../../src/process/events/ProcessActivityEvent.cpp"

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

static void testStoresAllFields()
{
    const auto when = makeUtcTime(2024, 1, 15, 10, 1);
    ProcessActivityEvent event(
        when,
        4120,
        "python.exe",
        ProcessActivityEventType::UPLOAD_ACTIVITY,
        "192.168.1.10:53142",
        "104.18.32.45:443",
        ProtocolType::TCP,
        13107200ull,
        3355443ull,
        "Upload Activity Detected");

    EXPECT_TRUE(event.timestamp == when);
    EXPECT_EQ(event.processId, 4120u);
    EXPECT_EQ(event.processName, std::string("python.exe"));
    EXPECT_TRUE(event.eventType == ProcessActivityEventType::UPLOAD_ACTIVITY);
    EXPECT_EQ(event.localAddress, std::string("192.168.1.10:53142"));
    EXPECT_EQ(event.remoteAddress, std::string("104.18.32.45:443"));
    EXPECT_TRUE(event.protocol == ProtocolType::TCP);
    EXPECT_EQ(event.uploadedBytes, 13107200ull);
    EXPECT_EQ(event.downloadedBytes, 3355443ull);
    EXPECT_EQ(event.description, std::string("Upload Activity Detected"));
}

static void testEventTypeLabels()
{
    EXPECT_EQ(
        ProcessActivityEvent::eventTypeToString(
            ProcessActivityEventType::PROCESS_SEEN),
        std::string("process_seen"));
    EXPECT_EQ(
        ProcessActivityEvent::eventTypeToString(
            ProcessActivityEventType::CONNECTION_OPENED),
        std::string("connection_opened"));
    EXPECT_EQ(
        ProcessActivityEvent::eventTypeToString(
            ProcessActivityEventType::UPLOAD_ACTIVITY),
        std::string("upload_activity"));
    EXPECT_EQ(
        ProcessActivityEvent::eventTypeToString(
            ProcessActivityEventType::DOWNLOAD_ACTIVITY),
        std::string("download_activity"));
    EXPECT_EQ(
        ProcessActivityEvent::eventTypeToString(
            ProcessActivityEventType::CONNECTION_CLOSED),
        std::string("connection_closed"));
    EXPECT_EQ(
        ProcessActivityEvent::eventTypeToString(
            ProcessActivityEventType::SUSPICIOUS_ACTIVITY),
        std::string("suspicious_activity"));
}

static void testDefaultDescriptions()
{
    EXPECT_EQ(
        ProcessActivityEvent::defaultDescription(
            ProcessActivityEventType::PROCESS_SEEN),
        std::string("Process Started"));
    EXPECT_EQ(
        ProcessActivityEvent::defaultDescription(
            ProcessActivityEventType::CONNECTION_OPENED),
        std::string("Network Connection Opened"));
    EXPECT_EQ(
        ProcessActivityEvent::defaultDescription(
            ProcessActivityEventType::UPLOAD_ACTIVITY),
        std::string("Upload Activity Detected"));
    EXPECT_EQ(
        ProcessActivityEvent::defaultDescription(
            ProcessActivityEventType::CONNECTION_CLOSED),
        std::string("Connection Closed"));
}

static void testTimelineString()
{
    ProcessActivityEvent started(
        makeUtcTime(2024, 1, 15, 10, 1),
        4120,
        "python.exe",
        ProcessActivityEventType::PROCESS_SEEN,
        "",
        "",
        ProtocolType::UNKNOWN,
        0,
        0,
        "");

    ProcessActivityEvent opened(
        makeUtcTime(2024, 1, 15, 10, 3),
        4120,
        "python.exe",
        ProcessActivityEventType::CONNECTION_OPENED,
        "192.168.1.10:53142",
        "104.18.32.45:443",
        ProtocolType::TCP,
        0,
        0,
        "");

    ProcessActivityEvent upload(
        makeUtcTime(2024, 1, 15, 10, 4),
        4120,
        "python.exe",
        ProcessActivityEventType::UPLOAD_ACTIVITY,
        "192.168.1.10:53142",
        "104.18.32.45:443",
        ProtocolType::TCP,
        2516582ull,
        0,
        "");

    ProcessActivityEvent closed(
        makeUtcTime(2024, 1, 15, 10, 10),
        4120,
        "python.exe",
        ProcessActivityEventType::CONNECTION_CLOSED,
        "192.168.1.10:53142",
        "104.18.32.45:443",
        ProtocolType::TCP,
        2516582ull,
        0,
        "");

    EXPECT_EQ(started.toTimelineString(), std::string("10:01 Process Started"));
    EXPECT_EQ(
        opened.toTimelineString(),
        std::string("10:03 Network Connection Opened"));
    EXPECT_EQ(
        upload.toTimelineString(),
        std::string("10:04 Upload Activity Detected"));
    EXPECT_EQ(
        closed.toTimelineString(), std::string("10:10 Connection Closed"));
}

static void testFormattedOutput()
{
    ProcessActivityEvent event(
        makeUtcTime(2024, 1, 15, 10, 4),
        4120,
        "python.exe",
        ProcessActivityEventType::UPLOAD_ACTIVITY,
        "192.168.1.10:53142",
        "104.18.32.45:443",
        ProtocolType::TCP,
        13107200ull,
        3355443ull,
        "Upload Activity Detected");

    const std::string expected =
        "Timestamp:\n"
        "10:04\n"
        "\n"
        "PID:\n"
        "4120\n"
        "\n"
        "Process:\n"
        "python.exe\n"
        "\n"
        "Event Type:\n"
        "upload_activity\n"
        "\n"
        "Local Address:\n"
        "192.168.1.10:53142\n"
        "\n"
        "Remote Address:\n"
        "104.18.32.45:443\n"
        "\n"
        "Protocol:\n"
        "TCP\n"
        "\n"
        "Uploaded:\n"
        "12.5 MB\n"
        "\n"
        "Downloaded:\n"
        "3.2 MB\n"
        "\n"
        "Description:\n"
        "Upload Activity Detected";

    EXPECT_EQ(event.toString(), expected);
}

static void testEmptyProcessNameFormatsAsUnknown()
{
    ProcessActivityEvent event(
        makeUtcTime(2024, 1, 15, 10, 1),
        1,
        "",
        ProcessActivityEventType::PROCESS_SEEN,
        "",
        "",
        ProtocolType::UNKNOWN,
        0,
        0,
        "");

    const std::string formatted = event.toString();
    EXPECT_TRUE(formatted.find("Process:\nUnknown\n") != std::string::npos);
    EXPECT_TRUE(formatted.find("Event Type:\nprocess_seen\n") != std::string::npos);
}

static void testCustomDescriptionOverridesDefault()
{
    ProcessActivityEvent event(
        makeUtcTime(2024, 1, 15, 10, 10),
        4120,
        "python.exe",
        ProcessActivityEventType::SUSPICIOUS_ACTIVITY,
        "192.168.1.10:53142",
        "104.18.32.45:443",
        ProtocolType::TCP,
        50ull * 1024ull * 1024ull,
        0,
        "Unusual outbound transfer volume");

    EXPECT_EQ(
        event.toTimelineString(),
        std::string("10:10 Unusual outbound transfer volume"));
    EXPECT_TRUE(
        event.toString().find("Description:\nUnusual outbound transfer volume") !=
        std::string::npos);
}

int main()
{
    testStoresAllFields();
    testEventTypeLabels();
    testDefaultDescriptions();
    testTimelineString();
    testFormattedOutput();
    testEmptyProcessNameFormatsAsUnknown();
    testCustomDescriptionOverridesDefault();

    if (g_failures == 0)
    {
        std::cout << "All ProcessActivityEvent tests passed.\n";
        return 0;
    }

    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
