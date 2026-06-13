#include "NetPulse/SpeedTestHistory.h"
#include <Windows.h>
#include <ShlObj.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <string>
#include <stdexcept>
#include "NetPulse/Utils.h"

namespace NetPulse
{

SpeedTestHistory::SpeedTestHistory()
{
    const wchar_t* testDataDir = _wgetenv(L"NETPULSE_TEST_DATA_DIR");
    if (testDataDir && testDataDir[0] != L'\0')
    {
        m_filePath = testDataDir;
        CreateDirectoryW(m_filePath.c_str(), nullptr);
        m_filePath += L"\\speed_test_history.json";
    }
    else
    {
        wchar_t appDataPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath)))
        {
            m_filePath = appDataPath;
            m_filePath += L"\\NetPulse";

            CreateDirectoryW(m_filePath.c_str(), nullptr);

            m_filePath += L"\\speed_test_history.json";
        }
    }
    
    LoadFromFile();
}

void SpeedTestHistory::AddResult(const SpeedTestResult& result)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Add to beginning (most recent first)
    m_history.insert(m_history.begin(), result);
    
    // Trim to max entries
    if (m_history.size() > MAX_HISTORY_ENTRIES)
    {
        m_history.resize(MAX_HISTORY_ENTRIES);
    }
    
    SaveToFile();
}

std::vector<SpeedTestResult> SpeedTestHistory::GetHistory(int limit) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (limit <= 0 || limit >= static_cast<int>(m_history.size()))
    {
        return m_history;
    }
    
    return std::vector<SpeedTestResult>(m_history.begin(), m_history.begin() + limit);
}

void SpeedTestHistory::ClearHistory()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_history.clear();
    SaveToFile();
}

const std::wstring& SpeedTestHistory::GetHistoryFilePath() const
{
    return m_filePath;
}

void SpeedTestHistory::LoadFromFile()
{
    if (m_filePath.empty()) return;
    
    std::ifstream file{std::filesystem::path(m_filePath)};
    if (!file.is_open()) return;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Simple JSON parsing (manual for minimal dependencies)
    // Format: [{"download":x,"upload":y,"ping":z,"server":"s","timestamp":t,"success":b},...]
    
    m_history.clear();
    
    size_t pos = 0;
    while ((pos = content.find('{', pos)) != std::string::npos)
    {
        size_t endPos = content.find('}', pos);
        if (endPos == std::string::npos) break;
        
        std::string entry = content.substr(pos, endPos - pos + 1);

        try
        {
            SpeedTestResult result;

            size_t downloadPos = entry.find("\"download\":");
            if (downloadPos != std::string::npos)
            {
                result.downloadMbps = std::stod(entry.substr(downloadPos + 11));
            }

            size_t uploadPos = entry.find("\"upload\":");
            if (uploadPos != std::string::npos)
            {
                result.uploadMbps = std::stod(entry.substr(uploadPos + 9));
            }

            size_t pingPos = entry.find("\"ping\":");
            if (pingPos != std::string::npos)
            {
                result.pingMs = std::stoi(entry.substr(pingPos + 7));
            }

            size_t serverPos = entry.find("\"server\":\"");
            if (serverPos != std::string::npos)
            {
                size_t serverEnd = entry.find('\"', serverPos + 10);
                if (serverEnd != std::string::npos)
                {
                    std::string serverStr = entry.substr(serverPos + 10, serverEnd - serverPos - 10);
                    result.serverName.clear();
                    for (char c : serverStr)
                    {
                        result.serverName += static_cast<wchar_t>(static_cast<unsigned char>(c));
                    }
                }
            }

            size_t tsPos = entry.find("\"timestamp\":");
            if (tsPos != std::string::npos)
            {
                result.timestamp = std::stoll(entry.substr(tsPos + 12));
            }

            size_t successPos = entry.find("\"success\":");
            if (successPos != std::string::npos)
            {
                result.success = (entry.find("true", successPos) == successPos + 10);
            }

            m_history.push_back(result);
        }
        catch (const std::exception&)
        {
            LogDebug(L"SpeedTestHistory::LoadFromFile: skipped invalid history entry");
        }

        pos = endPos + 1;
    }
}

void SpeedTestHistory::SaveToFile() const
{
    if (m_filePath.empty()) return;
    
    std::ofstream file{std::filesystem::path(m_filePath)};
    if (!file.is_open()) return;
    
    file << "[\n";
    
    for (size_t i = 0; i < m_history.size(); i++)
    {
        const auto& r = m_history[i];
        
        // Convert wstring to string for JSON (ASCII-safe)
        std::string serverStr;
        for (wchar_t wc : r.serverName) {
            serverStr += static_cast<char>(static_cast<unsigned char>(wc));
        }
        
        file << "  {";
        file << "\"download\":" << r.downloadMbps << ",";
        file << "\"upload\":" << r.uploadMbps << ",";
        file << "\"ping\":" << r.pingMs << ",";
        file << "\"server\":\"" << serverStr << "\",";
        file << "\"timestamp\":" << r.timestamp << ",";
        file << "\"success\":" << (r.success ? "true" : "false");
        file << "}";
        
        if (i < m_history.size() - 1)
        {
            file << ",";
        }
        file << "\n";
    }
    
    file << "]\n";
}

} // namespace NetPulse
