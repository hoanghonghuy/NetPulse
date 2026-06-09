# SPEC: Đánh Giá Khả Năng Di Chuyển NetPulse sang Fedora Linux

## 📋 Metadata

- **Dự án**: NetPulse Network Monitor
- **Phiên bản phân tích**: 2.3.0
- **Ngày tạo**: 2025-12-27
- **Người phân tích**: Spec Architect
- **Mục tiêu**: Đánh giá khả năng port ứng dụng từ Windows sang Fedora Linux

---

## 1. Tóm Tắt Đánh Giá (Executive Summary)

### Kết Luận Chính

**NetPulse KHÔNG THỂ di chuyển trực tiếp sang Fedora Linux.** Ứng dụng này được xây dựng hoàn toàn dựa trên Windows-native APIs và yêu cầu **viết lại hoàn toàn** (full rewrite) để hoạt động trên Linux.

### Mức Độ Phụ Thuộc Windows

| Thành Phần | Phụ Thuộc Windows | Mức Độ |
|-----------|-------------------|---------|
| GUI Framework | Win32 API | ⛔ Cao (100%) |
| Network Monitoring | GetIfTable2, ETW | ⛔ Cao (100%) |
| System Integration | Shell_NotifyIcon, Registry | ⛔ Cao (100%) |
| Graphics | GDI, Direct3D 11, DWM | ⛔ Cao (100%) |
| Core Logic | C++17 Standard | ✅ Portable (0%) |
| Database | SQLite | ✅ Portable (0%) |

### Ước Tính Effort

- **Effort để port**: ~80-100% mã nguồn cần viết lại
- **Thời gian ước tính**: 3-6 tháng (full-time developer)
- **Độ khó**: Rất cao (requires deep Linux system programming knowledge)

---

## 2. Phân Tích Chi Tiết Kiến Trúc

### 2.1. Tổng Quan Dự Án

NetPulse là một **Win32 application** thuần túy với các đặc điểm:

```
Ngôn ngữ: C++17
Build System: CMake + Visual Studio
Target OS: Windows 10/11 x64
GUI Framework: Win32 API (native Windows)
Architecture: Monolithic Win32 app
```

### 2.2. Cấu Trúc Thư Mục

```
NetworkMonitor/
├── include/NetPulse/          # Headers (chứa nhiều Windows types)
├── src/
│   ├── core/                  # Business logic (một phần portable)
│   ├── ui/                    # UI code (100% Win32)
│   └── entry/                 # WinMain entry point
├── resources/                 # Windows resources (.rc, .ico)
├── third_party/sqlite/        # ✅ Portable
└── tests/                     # Unit tests (một phần portable)
```

---

## 3. Phân Tích Dependencies Windows-Specific

### 3.1. System Headers Windows-Only

Từ [`Common.h`](include/NetPulse/Common.h:1):

```cpp
#include <winsock2.h>      // ⛔ Windows networking
#include <ws2tcpip.h>      // ⛔ Windows TCP/IP
#include <windows.h>       // ⛔ Core Win32 API
#include <winternl.h>      // ⛔ Windows internal APIs
```

### 3.2. Linked Libraries (từ CMakeLists.txt)

| Library | Mục Đích | Linux Alternative |
|---------|----------|-------------------|
| `Iphlpapi.lib` | Network interface enumeration | `/proc/net/dev`, `libnl`, `libnetlink` |
| `shell32.lib` | System tray integration | libappindicator, GTK StatusIcon, Qt SystemTray |
| `advapi32.lib` | Windows Registry | File-based config (INI, JSON, YAML) |
| `comctl32.lib` | Common Controls (dialogs, etc.) | GTK+, Qt Widgets |
| `Dwmapi.lib` | Desktop Window Manager (taskbar overlay) | X11 overlay, Wayland protocols |
| `d3d11.lib` | Direct3D 11 graphics | OpenGL, Vulkan |
| `dxgi.lib` | DirectX Graphics Infrastructure | EGL, GLX |
| `winhttp.lib` | HTTP requests | libcurl, libsoup |

### 3.3. Windows-Specific APIs Sử Dụng

#### A. Network Monitoring

**File**: [`NetworkMonitor.cpp`](src/core/NetworkMonitor.cpp:90-162)

```cpp
GetIfTable2(&pIfTable)           // ⛔ Windows IP Helper API
FreeMibTable(pIfTable)           // ⛔ Windows specific
MIB_IF_ROW2                      // ⛔ Windows structure
```

**Linux Alternative**: 
- `/proc/net/dev` (simple)
- `netlink` sockets (advanced)
- `libpcap` (packet level)
- `libnl` (netlink library)

#### B. ETW Network Monitoring

**File**: [`EtwNetworkMonitor.cpp`](src/core/EtwNetworkMonitor.cpp:1-317)

```cpp
StartTraceW()                    // ⛔ Event Tracing for Windows
EnableTraceEx2()                 // ⛔ ETW specific
ProcessTrace()                   // ⛔ ETW specific
PEVENT_RECORD                    // ⛔ Windows structure
```

**Linux Alternative**:
- `eBPF` (extended Berkeley Packet Filter)
- `SystemTap`
- `/proc/<pid>/net/` files
- `libpcap` với Berkeley Packet Filter

#### C. System Tray Icon

**File**: [`TrayIcon.cpp`](src/ui/TrayIcon.cpp:1-686)

```cpp
Shell_NotifyIconW(NIM_ADD)       // ⛔ Windows Shell API
NOTIFYICONDATAW                  // ⛔ Windows structure
LoadImageW()                     // ⛔ Windows resource loading
```

**Linux Alternative**:
- **GTK**: `GtkStatusIcon` (deprecated), `libappindicator`
- **Qt**: `QSystemTrayIcon`
- **KDE**: KStatusNotifierItem
- **GNOME**: Shell extensions

#### D. Taskbar Overlay

**File**: [`TaskbarOverlay.cpp`](src/ui/TaskbarOverlay.cpp:1-716)

```cpp
FindWindowW(L"Shell_TrayWnd")    // ⛔ Find Windows taskbar
SetLayeredWindowAttributes()     // ⛔ Windows layered window
DwmApi functions                 // ⛔ Desktop Window Manager
```

**Linux Alternative**:
- **X11**: Overlay windows với XComposite
- **Wayland**: Layer Shell protocol
- Khó khăn: Không có "taskbar" standard trên Linux (GNOME Panel, KDE Panel, etc.)

#### E. GUI Dialogs và Windows

**File**: [`main.cpp`](src/entry/main.cpp:10-14)

```cpp
int WINAPI WinMain(...)          // ⛔ Windows entry point
CreateWindowExW()                // ⛔ Win32 window creation
DefWindowProcW()                 // ⛔ Win32 message loop
```

**Linux Alternative**:
- **GTK**: `gtk_window_new()`, GTK main loop
- **Qt**: `QMainWindow`, Qt event loop
- **wxWidgets**: Cross-platform wrapper

#### F. Configuration Storage

```cpp
Registry API (HKCU\Software\...)  // ⛔ Windows Registry
```

**Linux Alternative**:
- `~/.config/netpulse/config.ini`
- `dconf` (GNOME)
- KConfig (KDE)
- Plain text files (JSON, TOML, YAML)

---

## 4. Thành Phần CÓ THỂ Port (Portable Components)

### 4.1. Core Business Logic (Một Phần)

Các class sau CÓ THỂ tái sử dụng với refactoring nhẹ:

- [`NetworkCalculator`](src/core/NetworkCalculator.cpp) - Tính toán tốc độ mạng (cần thay đổi input)
- [`HistoryLogger`](src/core/HistoryLogger.cpp) - SQLite logging (portable)
- [`SpeedTestHistory`](src/core/SpeedTestHistory.cpp) - SQLite storage (portable)
- Các utility functions trong [`Utils.cpp`](src/core/Utils.cpp) (ngoại trừ Windows-specific)

### 4.2. Third-Party Libraries

- **SQLite**: ✅ Hoàn toàn portable, có sẵn trên Fedora
- **C++17 Standard Library**: ✅ Portable

### 4.3. Algorithms và Data Structures

```cpp
struct NetworkStats              // ✅ Portable (chỉ cần thay Windows types)
enum class SpeedUnit            // ✅ Portable
```

---

## 5. Roadmap Nếu Muốn Port sang Linux

### Phase 1: Architecture Redesign (2-3 tuần)

1. **Chọn GUI Framework**:
   - **Option A**: GTK+ 4 (native cho GNOME/Fedora)
   - **Option B**: Qt 6 (cross-platform, modern)
   - **Option C**: wxWidgets (cross-platform wrapper)
   
   **Khuyến nghị**: **Qt 6** (tốt nhất cho cross-platform, modern C++)

2. **Thiết kế lại Architecture**:
   ```
   Linux Port Architecture:
   ├── Core Layer (C++, reuse partial logic)
   ├── Platform Abstraction Layer (PAL)
   │   ├── INetworkMonitor (interface)
   │   ├── ITrayIcon (interface)
   │   └── IConfiguration (interface)
   ├── Linux Implementation
   │   ├── LinuxNetworkMonitor (netlink/eBPF)
   │   ├── QtSystemTray
   │   └── FileConfiguration
   └── GUI Layer (Qt Widgets/QML)
   ```

### Phase 2: Core Porting (4-6 tuần)

#### 2.1. Network Monitoring

**Thay thế**: `GetIfTable2()` → Linux netlink

```cpp
// Pseudocode
class LinuxNetworkMonitor {
    int netlinkSocket;
    
    void QueryInterfaces() {
        // Parse /proc/net/dev
        // OR use netlink (NETLINK_ROUTE)
        struct ifaddrs *ifaddr;
        getifaddrs(&ifaddr);
        // Parse interface stats
    }
};
```

**Thư viện cần dùng**:
- `libnl-3` hoặc `libnl-route-3`
- Hoặc parse `/proc/net/dev` trực tiếp

#### 2.2. Per-Process Traffic Monitoring

**Thay thế**: ETW → eBPF hoặc /proc

```cpp
// Approach 1: eBPF (requires root)
// - Attach BPF programs to network stack
// - Track per-socket traffic

// Approach 2: /proc parsing (simpler)
class LinuxProcessMonitor {
    void ScanProcesses() {
        // Read /proc/<pid>/net/tcp
        // Read /proc/<pid>/net/udp
    }
};
```

**Lưu ý**: eBPF yêu cầu kernel ≥ 4.x và privileges cao

#### 2.3. System Tray

**Thay thế**: Win32 Shell API → Qt SystemTrayIcon

```cpp
// Qt implementation
QSystemTrayIcon trayIcon;
trayIcon.setIcon(QIcon(":/icons/app.png"));
trayIcon.show();

QMenu *menu = new QMenu();
menu->addAction("Settings", this, &App::showSettings);
trayIcon.setContextMenu(menu);
```

### Phase 3: GUI Rewrite (6-8 tuần)

Tất cả dialogs cần viết lại:

| Windows Dialog | Qt Equivalent |
|---------------|---------------|
| Win32 Dialog Resources | `.ui` files (Qt Designer) hoặc code |
| `CreateWindowExW()` | `QWidget`, `QDialog` |
| `DefWindowProcW()` | Qt Event System |
| GDI Drawing | QPainter, QGraphicsView |

### Phase 4: Platform-Specific Features (2-3 tuần)

#### Taskbar Overlay Trên Linux

**Thách thức**: Linux không có taskbar thống nhất

**Giải pháp**:
- **GNOME**: Shell extension (JavaScript + GJS)
- **KDE Plasma**: Plasmoid widget
- **Generic**: Tạo separate panel widget

**Khuyến nghị**: Bỏ qua taskbar overlay, chỉ giữ system tray + floating widget

#### Auto-start Trên Linux

**Thay thế**: Registry Auto-start → Desktop Entry

```ini
# ~/.config/autostart/netpulse.desktop
[Desktop Entry]
Type=Application
Exec=/usr/bin/netpulse
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
Name=NetPulse
Comment=Network Monitor
```

### Phase 5: Testing & Packaging (2-3 tuần)

1. **Unit Tests**: Port existing tests
2. **Integration Tests**: Test trên Fedora
3. **Packaging**:
   - RPM package (`.spec` file cho Fedora)
   - Flatpak (modern Linux app distribution)
   - AppImage (portable)

---

## 6. Công Nghệ Stack Khuyến Nghị Cho Linux Port

### 6.1. Tech Stack Mới

```yaml
Language: C++17/20
Build System: CMake
GUI Framework: Qt 6
Network: libnl-3 hoặc /proc parsing
Database: SQLite (giữ nguyên)
Config: QSettings (Qt) hoặc libconfig
HTTP: libcurl
```

### 6.2. Dependencies Trên Fedora

```bash
# Cài đặt build dependencies
sudo dnf install -y \
    cmake \
    gcc-c++ \
    qt6-qtbase-devel \
    qt6-qtsvg-devel \
    libnl3-devel \
    libcurl-devel \
    sqlite-devel
```

---

## 7. Phương Án Thay Thế (Alternatives)

### Option 1: Cross-Platform Rewrite (Khuyến nghị)

**Viết lại với Qt 6 để support cả Windows & Linux**

**Ưu điểm**:
- Một codebase cho nhiều platforms
- Modern C++ với Qt
- Dễ maintain hơn

**Nhược điểm**:
- Effort cao (3-6 tháng)
- Mất một số tính năng Windows-specific (taskbar overlay)

### Option 2: Wine Compatibility Layer

**Chạy Windows binary qua Wine**

**Ưu điểm**:
- Không cần viết lại code
- Nhanh chóng

**Nhược điểm**:
- Hiệu suất kém
- Không native experience
- Nhiều tính năng không hoạt động (system tray, notifications)
- Không khuyến nghị cho system monitoring apps

### Option 3: Dùng Sẵn Linux Network Monitors

**Thay vì port, dùng các tool Linux có sẵn**:

- **nethogs** - Per-process bandwidth monitor (CLI)
- **iftop** - Network bandwidth monitoring (CLI)
- **bmon** - Bandwidth monitor with graphs (CLI/ncurses)
- **GNOME System Monitor** - GUI, built-in
- **KSystemGuard** / **Plasma System Monitor** - KDE GUI

**Ưu điểm**:
- Không cần phát triển
- Đã stable và tested

**Nhược điểm**:
- Không có tất cả tính năng của NetPulse
- Không có system tray integration tương tự

---

## 8. Ước Tính Chi Phí & Effort

### 8.1. Breakdown Effort (Full Rewrite với Qt)

| Task | Effort | Priority |
|------|--------|----------|
| Architecture Design | 2 tuần | High |
| Linux Network Monitoring | 3 tuần | High |
| Qt GUI Development | 6 tuần | High |
| System Integration (Tray, Autostart) | 2 tuần | Medium |
| Per-Process Monitoring (eBPF) | 3 tuần | Medium |
| Testing & Debugging | 2 tuần | High |
| Packaging (RPM, Flatpak) | 1 tuần | Medium |
| Documentation | 1 tuần | Low |
| **TOTAL** | **20 tuần (~5 tháng)** | - |

### 8.2. Skill Requirements

- **C++17/20** expertise
- **Qt 6** framework knowledge
- **Linux system programming** (netlink, eBPF, /proc)
- **CMake** build system
- **Fedora packaging** (RPM, spec files)

---

## 9. Rủi Ro & Thách Thức

### 9.1. Technical Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| eBPF per-process monitoring yêu cầu root | High | Fallback to /proc parsing (limited accuracy) |
| Taskbar overlay không khả thi trên Linux | Medium | Focus on tray icon + floating widget |
| Cross-desktop compatibility (GNOME vs KDE) | Medium | Use Qt (better cross-desktop support) |
| Wayland vs X11 differences | Low | Qt handles this internally |

### 9.2. Business Risks

- **User base nhỏ**: Linux desktop market share thấp (~2-3%)
- **Phân mảnh**: Nhiều distros, desktop environments
- **Maintenance**: Phải maintain 2 codebases (Windows + Linux)

---

## 10. Kết Luận & Khuyến Nghị

### 10.1. Câu Trả Lời Trực Tiếp

**❌ NetPulse KHÔNG THỂ "chuyển sang dùng cho Fedora" mà không viết lại code.**

### 10.2. Lý Do Chính

1. **100% Win32 GUI**: Không có cross-platform abstraction
2. **Windows-only APIs**: GetIfTable2, ETW, Registry, Shell API, DWM
3. **Không có Linux equivalent trực tiếp** cho nhiều tính năng (taskbar overlay, ETW)

### 10.3. Khuyến Nghị

#### Nếu MỤC ĐÍCH chỉ cần monitor network trên Fedora:

**→ Dùng tools Linux có sẵn**: `nethogs`, `iftop`, `bmon`, hoặc GNOME System Monitor

#### Nếu MUỐN có NetPulse trên Linux:

**→ Viết lại với Qt 6** (cross-platform rewrite):
- Effort: 5-6 tháng
- Codebase mới: ~80-90% code mới
- Kết quả: Native Linux app + có thể support cả Windows sau này

#### Nếu CHỈ TEST khả năng chạy:

**→ Thử Wine** (không khuyến nghị cho production):
```bash
# Trên Fedora
sudo dnf install wine
wine NetPulse.exe
```
*Lưu ý: Nhiều tính năng sẽ không hoạt động*

---

## 11. Tài Liệu Tham Khảo

### Linux Network Monitoring

- [libnl Documentation](https://www.infradead.org/~tgr/libnl/)
- [eBPF Guide](https://ebpf.io/)
- [/proc Filesystem](https://man7.org/linux/man-pages/man5/proc.5.html)

### Qt Development

- [Qt 6 Documentation](https://doc.qt.io/qt-6/)
- [Qt System Tray Icon](https://doc.qt.io/qt-6/qsystemtrayicon.html)

### Fedora Packaging

- [Fedora Packaging Guidelines](https://docs.fedoraproject.org/en-US/packaging-guidelines/)
- [RPM Spec File Guide](https://rpm-packaging-guide.github.io/)

---

## Phụ Lục A: Code Mapping Windows → Linux

### A.1. Network Interface Enumeration

**Windows**:
```cpp
PMIB_IF_TABLE2 pIfTable;
GetIfTable2(&pIfTable);
for (ULONG i = 0; i < pIfTable->NumEntries; i++) {
    MIB_IF_ROW2* pIfRow = &pIfTable->Table[i];
    // Process interface
}
FreeMibTable(pIfTable);
```

**Linux** (approach 1 - /proc):
```cpp
#include <fstream>
std::ifstream netdev("/proc/net/dev");
std::string line;
while (std::getline(netdev, line)) {
    // Parse: "eth0: 1234 5678 ..." format
}
```

**Linux** (approach 2 - getifaddrs):
```cpp
#include <ifaddrs.h>
#include <net/if.h>

struct ifaddrs *ifaddr, *ifa;
getifaddrs(&ifaddr);
for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr->sa_family == AF_PACKET) {
        // Network interface
    }
}
freeifaddrs(ifaddr);
```

### A.2. System Tray

**Windows**:
```cpp
NOTIFYICONDATAW nid = {0};
nid.cbSize = sizeof(NOTIFYICONDATAW);
nid.hWnd = hwnd;
nid.uID = 1;
nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
nid.hIcon = LoadIcon(...);
Shell_NotifyIconW(NIM_ADD, &nid);
```

**Linux (Qt)**:
```cpp
QSystemTrayIcon *trayIcon = new QSystemTrayIcon(this);
trayIcon->setIcon(QIcon(":/icons/app.png"));
trayIcon->setToolTip("NetPulse");
trayIcon->show();

connect(trayIcon, &QSystemTrayIcon::activated,
        this, &MainWindow::onTrayActivated);
```

---

## Phụ Lục B: Feasibility Matrix

| Feature | Windows | Linux Feasibility | Notes |
|---------|---------|-------------------|-------|
| Network Speed Monitor | ✅ | ✅ | Khả thi với /proc hoặc netlink |
| System Tray Icon | ✅ | ✅ | Qt/GTK support tốt |
| Per-App Monitoring | ✅ (ETW) | ⚠️ | Cần eBPF (complex) hoặc /proc (limited) |
| Taskbar Overlay | ✅ (DWM) | ❌ | Không có standard taskbar trên Linux |
| Floating Widget | ✅ | ✅ | Khả thi với Qt/GTK |
| Ping Monitor | ✅ | ✅ | ICMP sockets (cần capabilities) |
| Speed Test | ✅ | ✅ | HTTP-based, portable |
| History Logging | ✅ | ✅ | SQLite portable |
| VPN Detection | ✅ | ✅ | Parse routing table / network interfaces |
| Dark Theme | ✅ | ✅ | Qt theming tốt |
| Auto-start | ✅ (Registry) | ✅ | Desktop entry files |

**Legend**:
- ✅ Khả thi tốt
- ⚠️ Khả thi nhưng khó hoặc hạn chế
- ❌ Không khả thi hoặc rất khó

---

**Kết thúc SPEC Document**

*Tài liệu này cung cấp đánh giá toàn diện về khả năng port NetPulse sang Fedora Linux. Kết luận: Cần viết lại ~80-90% mã nguồn với effort 5-6 tháng để có phiên bản Linux native.*
