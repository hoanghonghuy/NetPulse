#include "NetworkMonitor/HistoryLogger.h"
#include "NetworkMonitor/Utils.h"

#include <cwchar>   // wcsrchr
#include <ctime>
#include <string>
#include "sqlite3.h"

namespace NetworkMonitor
{

HistoryLogger& HistoryLogger::Instance()
{
    static HistoryLogger instance;
    return instance;
}

HistoryLogger::HistoryLogger()
    : m_initialized(false)
    , m_sqliteAvailable(false)
    , m_db(nullptr)
{
}

HistoryLogger::~HistoryLogger()
{
    ShutdownSQLite();
}

void HistoryLogger::EnsureInitialized()
{
    if (m_initialized)
    {
        return;
    }

    m_initialized = true;
    InitializeSQLite();
}

void HistoryLogger::InitializeSQLite()
{
    m_sqliteAvailable = false;

    // Build database path next to the executable
    wchar_t exePath[MAX_PATH] = {0};
    if (!GetModuleFileNameW(nullptr, exePath, MAX_PATH))
    {
        LogError(L"HistoryLogger::InitializeSQLite: GetModuleFileNameW failed: " + GetLastErrorString());
        ShutdownSQLite();
        return;
    }

    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash)
    {
        *lastSlash = L'\0';
    }

    wchar_t dbPath[MAX_PATH] = {0};
    swprintf_s(dbPath, L"%s\\network_usage.db", exePath);

    int openRc = sqlite3_open16(dbPath, &m_db);
    if (openRc != SQLITE_OK || !m_db)
    {
        LogError(L"HistoryLogger::InitializeSQLite: sqlite3_open16 failed, rc=" + std::to_wstring(openRc));
        if (m_db)
        {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
        return;
    }

    // Create table and index if they don't exist yet
    const char* createSql =
        "CREATE TABLE IF NOT EXISTS usage ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp INTEGER NOT NULL,"
        "interface TEXT NOT NULL,"
        "bytes_down INTEGER NOT NULL,"
        "bytes_up INTEGER NOT NULL);"
        "CREATE INDEX IF NOT EXISTS idx_usage_ts ON usage(timestamp);";

    int createRc = sqlite3_exec(m_db, createSql, nullptr, nullptr, nullptr);
    if (createRc != SQLITE_OK)
    {
        LogError(L"HistoryLogger::InitializeSQLite: sqlite3_exec(create table) failed, rc=" + std::to_wstring(createRc));
    }

    m_sqliteAvailable = true;
}

void HistoryLogger::ShutdownSQLite()
{
    if (m_db)
    {
        sqlite3_close(m_db);
        m_db = nullptr;
    }

    m_sqliteAvailable = false;
}

void HistoryLogger::AppendSample(const std::wstring& interfaceName,
                                 unsigned long long bytesDown,
                                 unsigned long long bytesUp)
{
    if (bytesDown == 0 && bytesUp == 0)
    {
        return;
    }

    EnsureInitialized();
    if (!m_sqliteAvailable || !m_db)
    {
        return;
    }

    std::time_t now = std::time(nullptr);
    InsertSampleSQLite(now, interfaceName, bytesDown, bytesUp);
}

bool HistoryLogger::InsertSampleSQLite(std::time_t ts,
                                       const std::wstring& iface,
                                       unsigned long long down,
                                       unsigned long long up)
{
    if (!m_sqliteAvailable || !m_db)
    {
        return false;
    }

    static const wchar_t* INSERT_SQL =
        L"INSERT INTO usage (timestamp, interface, bytes_down, bytes_up) "
        L"VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare16_v2(m_db, INSERT_SQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt)
    {
        LogError(L"HistoryLogger::InsertSampleSQLite: sqlite3_prepare16_v2 failed, rc=" + std::to_wstring(rc));
        return false;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(ts));
    sqlite3_bind_text16(stmt, 2, iface.c_str(), -1, nullptr);
    sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(down));
    sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(up));

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_OK)
    {
        LogError(L"HistoryLogger::InsertSampleSQLite: sqlite3_step failed, rc=" + std::to_wstring(rc));
    }
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE || rc == SQLITE_OK);
}

bool HistoryLogger::GetTotalsToday(unsigned long long& totalDown, unsigned long long& totalUp,
                                   const std::wstring* interfaceFilter)
{
    totalDown = 0;
    totalUp = 0;

    EnsureInitialized();
    if (!m_sqliteAvailable || !m_db)
    {
        LogError(L"HistoryLogger::GetTotalsToday: SQLite not available");
        return false;
    }

    std::time_t now = std::time(nullptr);
    std::tm localTime = {};
    if (localtime_s(&localTime, &now) != 0)
    {
        LogError(L"HistoryLogger::GetTotalsToday: localtime_s failed");
        return false;
    }

    localTime.tm_hour = 0;
    localTime.tm_min = 0;
    localTime.tm_sec = 0;

    std::time_t start = std::mktime(&localTime);
    if (start == static_cast<std::time_t>(-1))
    {
        LogError(L"HistoryLogger::GetTotalsToday: mktime(start) failed");
        return false;
    }

    std::time_t end = start + 24 * 60 * 60;

    const char* sql =
        "SELECT COALESCE(SUM(bytes_down), 0), COALESCE(SUM(bytes_up), 0) "
        "FROM usage WHERE timestamp >= ? AND timestamp < ?";

    bool useFilter = (interfaceFilter != nullptr && !interfaceFilter->empty());
    if (useFilter)
    {
        sql =
            "SELECT COALESCE(SUM(bytes_down), 0), COALESCE(SUM(bytes_up), 0) "
            "FROM usage WHERE timestamp >= ? AND timestamp < ? AND interface = ?";
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt)
    {
        LogError(L"HistoryLogger::GetTotalsToday: sqlite3_prepare_v2 failed, rc=" + std::to_wstring(rc));
        return false;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(start));
    sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(end));

    if (useFilter)
    {
        sqlite3_bind_text16(stmt, 3, interfaceFilter->c_str(), -1, nullptr);
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        totalDown = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 0));
        totalUp = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 1));
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        LogError(L"HistoryLogger::GetTotalsToday: sqlite3_step failed, rc=" + std::to_wstring(rc));
    }

    return (rc == SQLITE_ROW || rc == SQLITE_DONE);
}

bool HistoryLogger::GetTotalsThisMonth(unsigned long long& totalDown, unsigned long long& totalUp,
                                       const std::wstring* interfaceFilter)
{
    totalDown = 0;
    totalUp = 0;

    EnsureInitialized();
    if (!m_sqliteAvailable || !m_db)
    {
        LogError(L"HistoryLogger::GetTotalsThisMonth: SQLite not available");
        return false;
    }

    std::time_t now = std::time(nullptr);
    std::tm startTm = {};
    if (localtime_s(&startTm, &now) != 0)
    {
        LogError(L"HistoryLogger::GetTotalsThisMonth: localtime_s failed");
        return false;
    }

    startTm.tm_mday = 1;
    startTm.tm_hour = 0;
    startTm.tm_min = 0;
    startTm.tm_sec = 0;

    std::time_t start = std::mktime(&startTm);
    if (start == static_cast<std::time_t>(-1))
    {
        LogError(L"HistoryLogger::GetTotalsThisMonth: mktime(start) failed");
        return false;
    }

    std::tm endTm = startTm;
    endTm.tm_mon += 1;
    if (endTm.tm_mon >= 12)
    {
        endTm.tm_mon -= 12;
        endTm.tm_year += 1;
    }

    std::time_t end = std::mktime(&endTm);
    if (end == static_cast<std::time_t>(-1))
    {
        LogError(L"HistoryLogger::GetTotalsThisMonth: mktime(end) failed");
        return false;
    }

    const char* sql =
        "SELECT COALESCE(SUM(bytes_down), 0), COALESCE(SUM(bytes_up), 0) "
        "FROM usage WHERE timestamp >= ? AND timestamp < ?";

    bool useFilter = (interfaceFilter != nullptr && !interfaceFilter->empty());
    if (useFilter)
    {
        sql =
            "SELECT COALESCE(SUM(bytes_down), 0), COALESCE(SUM(bytes_up), 0) "
            "FROM usage WHERE timestamp >= ? AND timestamp < ? AND interface = ?";
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt)
    {
        LogError(L"HistoryLogger::GetTotalsThisMonth: sqlite3_prepare_v2 failed, rc=" + std::to_wstring(rc));
        return false;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(start));
    sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(end));

    if (useFilter)
    {
        sqlite3_bind_text16(stmt, 3, interfaceFilter->c_str(), -1, nullptr);
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        totalDown = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 0));
        totalUp = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 1));
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        LogError(L"HistoryLogger::GetTotalsThisMonth: sqlite3_step failed, rc=" + std::to_wstring(rc));
    }

    return (rc == SQLITE_ROW || rc == SQLITE_DONE);
}

bool HistoryLogger::GetRecentSamples(int limit, std::vector<HistorySample>& outSamples,
                                     const std::wstring* interfaceFilter,
                                     bool onlyToday)
{
    outSamples.clear();

    if (limit <= 0)
    {
        return true;
    }

    EnsureInitialized();
    if (!m_sqliteAvailable || !m_db)
    {
        return false;
    }

    // Build dynamic query based on filters
    std::string sql =
        "SELECT timestamp, interface, bytes_down, bytes_up FROM usage";

    std::time_t startToday = 0;
    bool restrictToday = onlyToday;
    if (restrictToday)
    {
        if (!ComputeStartOfToday(startToday))
        {
            restrictToday = false;
        }
    }

    bool useFilter = (interfaceFilter != nullptr && !interfaceFilter->empty());

    bool hasWhere = false;
    if (restrictToday)
    {
        sql += " WHERE timestamp >= ?";
        hasWhere = true;
    }

    if (useFilter)
    {
        sql += hasWhere ? " AND interface = ?" : " WHERE interface = ?";
        hasWhere = true;
    }

    sql += " ORDER BY timestamp DESC LIMIT ?";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt)
    {
        LogError(L"HistoryLogger::GetRecentSamples: sqlite3_prepare_v2 failed, rc=" + std::to_wstring(rc));
        return false;
    }

    int bindIndex = 1;
    if (restrictToday)
    {
        sqlite3_bind_int64(stmt, bindIndex++, static_cast<sqlite3_int64>(startToday));
    }
    if (useFilter)
    {
        sqlite3_bind_text16(stmt, bindIndex++, interfaceFilter->c_str(), -1, nullptr);
    }

    sqlite3_bind_int(stmt, bindIndex, limit);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        HistorySample sample;
        sample.timestamp = static_cast<std::time_t>(sqlite3_column_int64(stmt, 0));

        const void* ifaceText = sqlite3_column_text16(stmt, 1);
        if (ifaceText)
        {
            sample.interfaceName.assign(static_cast<const wchar_t*>(ifaceText));
        }
        else
        {
            sample.interfaceName.clear();
        }

        sample.bytesDown = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 2));
        sample.bytesUp = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 3));

        outSamples.push_back(std::move(sample));
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        LogError(L"HistoryLogger::GetRecentSamples: sqlite3_step ended with rc=" + std::to_wstring(rc));
    }

    LogRecentSamplesDebug(limit, onlyToday, interfaceFilter, outSamples);

    return (rc == SQLITE_DONE);
}

bool HistoryLogger::ComputeStartOfToday(std::time_t& startOut)
{
    std::time_t now = std::time(nullptr);
    std::tm localTime = {};
    if (localtime_s(&localTime, &now) != 0)
    {
        return false;
    }

    localTime.tm_hour = 0;
    localTime.tm_min = 0;
    localTime.tm_sec = 0;

    std::time_t start = std::mktime(&localTime);
    if (start == static_cast<std::time_t>(-1))
    {
        return false;
    }

    startOut = start;
    return true;
}

void HistoryLogger::LogRecentSamplesDebug(int limit,
                                          bool onlyToday,
                                          const std::wstring* interfaceFilter,
                                          const std::vector<HistorySample>& outSamples)
{
    if (outSamples.empty())
    {
        std::wstring info = L"HistoryLogger::GetRecentSamples: no samples returned (limit="
            + std::to_wstring(limit)
            + L", onlyToday=" + (onlyToday ? L"true" : L"false")
            + L", interfaceFilter="
            + ((interfaceFilter && !interfaceFilter->empty()) ? *interfaceFilter : L"<none>");
        LogDebug(info);
    }
    else
    {
        std::wstring info = L"HistoryLogger::GetRecentSamples: "
            + std::to_wstring(static_cast<unsigned long long>(outSamples.size()))
            + L" samples returned (limit="
            + std::to_wstring(limit)
            + L", onlyToday=" + (onlyToday ? L"true" : L"false")
            + L", interfaceFilter="
            + ((interfaceFilter && !interfaceFilter->empty()) ? *interfaceFilter : L"<none>");
        LogDebug(info);
    }
}

bool HistoryLogger::DeleteAll()
{
    EnsureInitialized();
    if (!m_sqliteAvailable || !m_db)
    {
        LogError(L"HistoryLogger::DeleteAll: SQLite not available");
        return false;
    }

    const char* sql = "DELETE FROM usage;";
    int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK && rc != SQLITE_DONE)
    {
        LogError(L"HistoryLogger::DeleteAll: sqlite3_exec failed, rc=" + std::to_wstring(rc));
        return false;
    }

    LogDebug(L"HistoryLogger::DeleteAll: deleted all history records");
    return true;
}

bool HistoryLogger::TrimToRecentDays(int days)
{
    if (days <= 0)
    {
        return DeleteAll();
    }

    EnsureInitialized();
    if (!m_sqliteAvailable || !m_db)
    {
        return false;
    }

    std::time_t now = std::time(nullptr);
    std::time_t cutoff = now - static_cast<std::time_t>(static_cast<long long>(days) * 24 * 60 * 60);

    const char* sql = "DELETE FROM usage WHERE timestamp < ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt)
    {
        LogError(L"HistoryLogger::TrimToRecentDays: sqlite3_prepare_v2 failed, rc=" + std::to_wstring(rc));
        return false;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(cutoff));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE && rc != SQLITE_OK)
    {
        LogError(L"HistoryLogger::TrimToRecentDays: sqlite3_step failed, rc=" + std::to_wstring(rc));
        return false;
    }

    LogDebug(L"HistoryLogger::TrimToRecentDays: trimmed history to last " + std::to_wstring(days) + L" days");
    return true;
}

} // namespace NetworkMonitor
