# Hotel Management System

**Stack:** C++17 · Crow REST API · SQLite3 · Plain HTML/CSS/JS

---

## Project Structure

```
HotelManagementSystem/
├── CMakeLists.txt
├── README.md
├── backend/
│   ├── include/             ← All .h header files
│   │   ├── Models.h         ← All data structs (Hotel, Room, Booking …)
│   │   ├── Database.h       ← SQLite3 RAII wrapper
│   │   ├── AuthManager.h    ← Login, sessions, roles
│   │   ├── HotelManager.h   ← Multi-hotel CRUD
│   │   ├── RoomManager.h    ← Room availability & status
│   │   ├── BookingManager.h ← Create/view bookings
│   │   ├── Managers.h       ← Billing, Cancel, Search, Reports, Receipt, Backup
│   │   ├── APIRouter.h      ← Crow route registrations
│   │   └── Utils.h          ← Hashing, dates, validators
│   ├── src/                 ← All .cpp implementation files
│   │   ├── main.cpp         ← Entry point
│   │   ├── Database.cpp
│   │   ├── APIRouter.cpp
│   │   └── …
│   └── lib/                 ← sqlite3.c + sqlite3.h (add via setup script)
├── frontend/
│   ├── index.html           ← Landing / login
│   ├── css/styles.css
│   ├── js/
│   │   ├── api.js           ← Fetch wrapper for all REST calls
│   │   ├── app.js           ← Router / auth state
│   │   ├── dashboard.js
│   │   ├── bookings.js
│   │   ├── reports.js
│   │   └── admin.js
│   └── pages/
│       ├── dashboard.html
│       ├── bookings.html
│       ├── rooms.html
│       ├── reports.html
│       └── admin.html
├── data/
│   ├── hotel.db             ← SQLite database (auto-created)
│   └── backups/             ← Timestamped backups
├── receipts/                ← Generated .txt receipts
├── bin/                     ← Compiled binary output
└── scripts/
    ├── setup_linux.sh
    └── setup_windows.bat
```

---

## Quick Start

### Prerequisites

| Tool    | Minimum Version | Download |
|---------|-----------------|----------|
| CMake   | 3.16            | https://cmake.org |
| Git     | 2.x             | https://git-scm.com |
| GCC/G++ | 9+ (Linux/Mac)  | via apt/brew |
| MSVC    | VS 2019+        | Visual Studio |

---

### Linux / macOS

```bash
# 1. Clone or unzip the project
cd HotelManagementSystem

# 2. Run setup (installs deps + downloads SQLite + builds)
chmod +x scripts/setup_linux.sh
./scripts/setup_linux.sh

# 3. Start the server
./bin/hotel_server

# 4. Open browser
#    http://localhost:8080
```

---

### Windows (Visual Studio / Command Prompt)

```bat
cd HotelManagementSystem
scripts\setup_windows.bat

:: After build:
bin\hotel_server.exe
```

**Using Visual Studio IDE:**
1. Open Visual Studio → "Open a local folder" → select `HotelManagementSystem/`
2. VS will detect `CMakeLists.txt` automatically
3. Set startup item to `hotel_server.exe`
4. Press **F5** to build and run

---

### Manual SQLite3 Download (if setup script doesn't work)

1. Go to https://www.sqlite.org/download.html
2. Download `sqlite-amalgamation-*.zip`
3. Extract `sqlite3.c` and `sqlite3.h` into `backend/lib/`

---

## API Reference (Phase 3+)

| Method | Endpoint                       | Description               |
|--------|--------------------------------|---------------------------|
| POST   | `/api/auth/login`              | Login, returns token      |
| POST   | `/api/auth/logout`             | Invalidate session        |
| GET    | `/api/hotels`                  | List hotels               |
| POST   | `/api/hotels`                  | Create hotel (admin)      |
| GET    | `/api/hotels/:id/rooms`        | List rooms                |
| GET    | `/api/bookings`                | List bookings             |
| POST   | `/api/bookings`                | Create booking            |
| DELETE | `/api/bookings/:id`            | Cancel booking            |
| GET    | `/api/reports/revenue`         | Revenue report            |
| GET    | `/api/reports/occupancy`       | Occupancy stats           |
| POST   | `/api/backup`                  | Create backup (admin)     |

---

## Default Admin Credentials

Created automatically on first run:

| Field    | Value         |
|----------|---------------|
| Username | `admin`       |
| Password | `Admin@1234`  |
| Role     | Super Admin   |

> **Change the password immediately** via Admin Panel → Profile.

---

## Build Phases

| Phase | Module               | Status  |
|-------|----------------------|---------|
| 1     | Project Setup        | ✅ Done |
| 2     | Database Layer       | 🔜 Next |
| 3     | Auth Module          | Pending |
| 4     | Hotel & Room Mgmt    | Pending |
| 5     | Booking Module       | Pending |
| 6     | Billing System       | Pending |
| 7     | Cancellation         | Pending |
| 8     | Search & Reports     | Pending |
| 9     | Receipt Module       | Pending |
| 10    | Frontend UI          | Pending |
| 11    | AI Smart Features    | Pending |
| 12    | Backup & Polish      | Pending |
