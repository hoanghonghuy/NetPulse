#ifndef NETWORK_MONITOR_HISTORYLOGGER_H
#define NETWORK_MONITOR_HISTORYLOGGER_H

#include "NetworkMonitor/Common.h"
#include <string>
#include <vector>
#include <ctime>

struct sqlite3;

namespace NetworkMonitor
{

struct HistorySample
{
    std::time_t timestamp;           // UTC timestamp (seconds since epoch)
    std::wstring interfaceName;      // Interface name or "All Interfaces"
    unsigned long long bytesDown;    // Bytes downloaded in interval
    unsigned long long bytesUp;      // Bytes uploaded in interval
};

class HistoryLogger
{
public:
    static HistoryLogger& Instance();

    /**
     * Append a usage sample (delta bytes for the interval).
     * If SQLite is not available, the call is a no-op.
     */
    void AppendSample(const std::wstring& interfaceName,
                      unsigned long long bytesDown,
                      unsigned long long bytesUp);

    // Dashboard queries
    // If interfaceFilter is non-null and non-empty, filter by that interface name.
    bool GetTotalsToday(unsigned long long& totalDown, unsigned long long& totalUp,
                        const std::wstring* interfaceFilter = nullptr);

    bool GetTotalsThisMonth(unsigned long long& totalDown, unsigned long long& totalUp,
                            const std::wstring* interfaceFilter = nullptr);

    // If onlyToday == true, restrict samples to from start-of-today (local time).
    bool GetRecentSamples(int limit, std::vector<HistorySample>& outSamples,
                          const std::wstring* interfaceFilter = nullptr,
                          bool onlyToday = false);

    bool DeleteAll();
    bool TrimToRecentDays(int days);

private:
    HistoryLogger();
    ~HistoryLogger();

    HistoryLogger(const HistoryLogger&) = delete;
    HistoryLogger& operator=(const HistoryLogger&) = delete;

    void EnsureInitialized();
    void InitializeSQLite();
    void ShutdownSQLite();

    bool InsertSampleSQLite(std::time_t ts,
                            const std::wstring& iface,
                            unsigned long long down,
                            unsigned long long up);

    bool ComputeStartOfToday(std::time_t& startOut);
    void LogRecentSamplesDebug(int limit,
                               bool onlyToday,
                               const std::wstring* interfaceFilter,
                               const std::vector<HistorySample>& samples);

    bool m_initialized;
    bool m_sqliteAvailable;

    sqlite3* m_db;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_HISTORYLOGGER_H
