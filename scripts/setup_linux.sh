#!/usr/bin/env bash
set -e
BOLD='\033[1m'; GREEN='\033[0;32m'; RED='\033[0;31m'; NC='\033[0m'

echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║       Hotel MS — Linux/macOS Setup Script           ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""

# ── Detect package manager ────────────────────────────────────────────────────
step() { echo -e "${BOLD}[$1/5]${NC} $2"; }

step 1 "Checking and installing system dependencies..."

if command -v apt-get &>/dev/null; then
    sudo apt-get update -q
    sudo apt-get install -y cmake git build-essential libssl-dev wget unzip
elif command -v brew &>/dev/null; then
    brew install cmake git openssl wget
elif command -v dnf &>/dev/null; then
    sudo dnf install -y cmake git gcc-c++ openssl-devel wget unzip
else
    echo -e "${RED}ERROR:${NC} Unsupported package manager. Install cmake, git, g++ manually."
    exit 1
fi
echo -e "${GREEN}  Dependencies installed.${NC}"

# ── SQLite3 amalgamation ──────────────────────────────────────────────────────
step 2 "Downloading SQLite3 amalgamation..."
if [ ! -f "backend/lib/sqlite3.h" ]; then
    SQLITE_URL="https://www.sqlite.org/2024/sqlite-amalgamation-3450100.zip"
    wget -q "$SQLITE_URL" -O sqlite_amal.zip
    unzip -q sqlite_amal.zip -d sqlite_tmp
    cp sqlite_tmp/sqlite-amalgamation-3450100/sqlite3.c backend/lib/
    cp sqlite_tmp/sqlite-amalgamation-3450100/sqlite3.h backend/lib/
    rm -rf sqlite_tmp sqlite_amal.zip
    echo -e "${GREEN}  SQLite3 amalgamation ready.${NC}"
else
    echo "  SQLite3 already present, skipping."
fi

# ── Create directories ────────────────────────────────────────────────────────
step 3 "Creating runtime directories..."
mkdir -p bin data/backups receipts
echo -e "${GREEN}  Directories ready.${NC}"

# ── CMake configure ───────────────────────────────────────────────────────────
step 4 "Configuring CMake (Release)..."
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1
cd ..
echo -e "${GREEN}  CMake configured.${NC}"

# ── Build ─────────────────────────────────────────────────────────────────────
step 5 "Building (first build ~2-5 min, subsequent builds fast)..."
cmake --build build --parallel $(nproc)
echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║   Build SUCCESS!                                     ║${NC}"
echo -e "${GREEN}║   Run:  ./bin/hotel_server                           ║${NC}"
echo -e "${GREEN}║   Open: http://localhost:8080                        ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════╝${NC}"
