#ifndef NETWORK_MONITOR_HISTORYLOGGER_H
#define NETWORK_MONITOR_HISTORYLOGGER_H

#include "NetPulse/Common.h"
#include <string>
#include <vector>
#include <ctime>

struct sqlite3;

namespace NetPulse
{

struct HistorySample
{
    std::time_t timestamp;           // UTC timestamp (seconds since epoch)
    std::wstring interfaceName;      // Interface name or "All Interfaces"
    unsigned long long bytesDown;    // Bytes downloaded in interval
    unsigned long long bytesUp;      // Bytes uploaded in interval
};

// Chart data structures
struct DailyUsage
{
    int day;                         // 1-31
    uint64_t bytesDown;
    uint64_t bytesUp;
};

struct MonthlyUsage
{
    int month;                       // 1-12
    uint64_t bytesDown;
    uint64_t bytesUp;
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

    /**
     * Get total bytes (down + up) for current month - convenience method for data usage alerts
     */
    uint64_t GetThisMonthTotalBytes(const std::wstring* interfaceFilter = nullptr);

    // If onlyToday == true, restrict samples to from start-of-today (local time).
    bool GetRecentSamples(int limit, std::vector<HistorySample>& outSamples,
                          const std::wstring* interfaceFilter = nullptr,
                          bool onlyToday = false);

    // Chart data queries
    /**
     * Get daily usage aggregated by day for a specific month
     * @param year The year (e.g., 2024)
     * @param month The month (1-12)
     * @param outData Output vector of daily usage data
     * @return true if query succeeded
     */
    bool GetDailyUsage(int year, int month, std::vector<DailyUsage>& outData);

    /**
     * Get monthly usage aggregated by month for a specific year
     * @param year The year (e.g., 2024)
     * @param outData Output vector of monthly usage data
     * @return true if query succeeded
     */
    bool GetMonthlyUsage(int year, std::vector<MonthlyUsage>& outData);

    bool DeleteAll();
    bool TrimToRecentDays(int days);

    /**
     * Export history data to CSV file
     * @param filePath Path to the output CSV file
     * @param interfaceFilter Optional filter by interface name
     * @param daysBack Number of days of history to export (0 = all)
     * @return true if export succeeded
     */
    bool ExportToCSV(const std::wstring& filePath,
                     const std::wstring* interfaceFilter = nullptr,
                     int daysBack = 0);

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

} // namespace NetPulse

#endif // NETWORK_MONITOR_HISTORYLOGGER_H
