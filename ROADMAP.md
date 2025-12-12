# NetworkMonitor Roadmap

This document tracks planned features and improvements for future development.

---

## 🔥 New Features

### 1. VPN & Proxy Detection
- [ ] Detect when connected via VPN/Proxy and display indicator
- [ ] Display current public IP (periodic update)
- **Priority**: Medium
- **Complexity**: Medium

### 2. Integrated Bandwidth Speed Test
- [ ] "Test Speed" button in Dashboard to test actual speed
- [ ] Save speed test history for comparison
- **Priority**: Medium
- **Complexity**: High

### 3. Advanced Network Graphs
- [ ] Live real-time chart in Floating Window (mini graph/sparkline)
- [ ] Sparklines for recent 30s/1m/5m speeds
- [ ] Export chart to PNG/CSV
- **Priority**: High
- **Complexity**: Medium

### 4. Network Quality Score (NQS)
- [ ] Quality score based on: latency, jitter, packet loss
- [ ] Tray icon color representing overall network status
- **Priority**: Medium
- **Complexity**: Medium

### 5. Data Usage Quota per App
- [ ] Set data limits for specific applications
- [ ] Alert when app exceeds quota
- **Priority**: Low
- **Complexity**: High

### 6. Scheduled Reports
- [ ] Email/export weekly/monthly usage reports
- [ ] PDF summary with graphs
- **Priority**: Low
- **Complexity**: High

### 7. Gaming/Streaming Mode
- [ ] Detect when gaming/streaming and prominently display ping
- [ ] Auto-disable notifications during gaming
- **Priority**: Medium
- **Complexity**: Medium

---

## 🛠️ UI/UX Improvements

### Floating Window Enhancements
- [x] ✅ Mini-mode with only 1-2 metrics displayed (Phase 1)
- [ ] Mini sparkline chart for speed (Phase 2)
- [x] ✅ Click-through mode (transparent to mouse) (Phase 1)
- [x] ✅ Snap-to-edge when dragging near screen edges (Phase 1)
- [ ] Multiple Floating Windows with different content
- **Priority**: High
- **Complexity**: Medium

### Dashboard Improvements
- [ ] Draggable/rearrangeable widgets
- [ ] Customizable layout
- **Priority**: Medium
- **Complexity**: Medium

### Tray Icon Enhancements
- [ ] Animated icon during high network activity
- [ ] Smooth color transition instead of sudden changes
- [ ] Gradient effects
- **Priority**: Low
- **Complexity**: Low

---

## ⚙️ Performance & Technical

### ETW Integration
- [ ] Improve PerAppMonitor with ETW events instead of polling
- **Priority**: Medium
- **Complexity**: High

### IPv6 Statistics
- [ ] Display separate IPv4/IPv6 statistics
- **Priority**: Low
- **Complexity**: Medium

### Process Tree Grouping
- [ ] Group child processes (e.g., chrome with chrome.exe subprocesses)
- **Priority**: Medium
- **Complexity**: Medium

---

## 📊 Data & Analytics

### Usage Comparison
- [ ] Weekly/Monthly usage comparison
- **Priority**: Medium
- **Complexity**: Low

### Peak Hours Analysis
- [ ] Analyze peak network usage hours
- **Priority**: Low
- **Complexity**: Medium

### Connection History
- [ ] Save connect/disconnect history with details
- **Priority**: Low
- **Complexity**: Low

---

## ⚡ Settings & Configuration

### Settings Profiles
- [ ] Save multiple settings profiles (Home, Office, Gaming)
- [ ] Auto-switch based on network name (SSID)
- **Priority**: Medium
- **Complexity**: Medium

---

## 📋 Priority Legend

| Priority | Description |
|----------|-------------|
| **High** | Should be implemented soon, high user value |
| **Medium** | Nice to have, moderate effort |
| **Low** | Future consideration, low priority |

## 📋 Complexity Legend

| Complexity | Description |
|------------|-------------|
| **Low** | Simple implementation, < 1 day |
| **Medium** | Moderate effort, 1-3 days |
| **High** | Significant effort, > 3 days |

---

*Last updated: 2025-12-12*
