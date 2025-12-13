#pragma once

#include "NetworkMonitor/Interfaces/ISpeedTestProvider.h"
#include <vector>
#include <string>
#include <mutex>

namespace NetworkMonitor
{

/**
 * @brief Stores and manages speed test history
 * 
 * Saves results to JSON file in %APPDATA%/NetworkMonitor/
 */
class SpeedTestHistory
{
public:
    SpeedTestHistory();
    ~SpeedTestHistory() = default;
    
    /**
     * @brief Add a new test result to history
     */
    void AddResult(const SpeedTestResult& result);
    
    /**
     * @brief Get test history (most recent first)
     * @param limit Maximum number of results to return (0 = all)
     */
    std::vector<SpeedTestResult> GetHistory(int limit = 30) const;
    
    /**
     * @brief Clear all history
     */
    void ClearHistory();
    
    /**
     * @brief Get the path to the history file
     */
    std::wstring GetHistoryFilePath() const;
    
private:
    void LoadFromFile();
    void SaveToFile() const;
    
    mutable std::mutex m_mutex;
    std::vector<SpeedTestResult> m_history;
    std::wstring m_filePath;
    
    static constexpr int MAX_HISTORY_ENTRIES = 100;
};

} // namespace NetworkMonitor
