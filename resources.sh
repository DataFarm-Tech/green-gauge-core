#!/usr/bin/env bash
set -e

ELF="$1"

if [[ -z "$ELF" || ! -f "$ELF" ]]; then
  echo "Usage: $0 firmware.elf"
  exit 1
fi

# ---- ESP32 memory limits (approx, bytes) ----
IRAM_LIMIT=$((128 * 1024))
DRAM_LIMIT=$((320 * 1024))   # internal DRAM usable
RTC_FAST_LIMIT=$((8 * 1024))
RTC_SLOW_LIMIT=$((8 * 1024))

WARN_PCT=80
FAIL_PCT=95

# ---- helper ----
pct() { echo $(( ($1 * 100) / $2 )); }

# ---- extract sizes ----
read_section() {
  xtensa-esp32-elf-size -A "$ELF" | awk -v sec="$1" '$1==sec {print $2}'
}

IRAM_USED=$(( $(read_section .iram0.text) + $(read_section .iram0.vectors) ))
DRAM_USED=$(( $(read_section .dram0.data) + $(read_section .dram0.bss) ))
RTC_FAST_USED=$(read_section .rtc.force_fast)
RTC_SLOW_USED=$(read_section .rtc_slow_bss)

IRAM_PCT=$(pct $IRAM_USED $IRAM_LIMIT)
DRAM_PCT=$(pct $DRAM_USED $DRAM_LIMIT)

# ---- report ----
echo "==== ESP32 Resource Check ===="
printf "IRAM: %6d / %6d bytes (%d%%)\n" $IRAM_USED $IRAM_LIMIT $IRAM_PCT
printf "DRAM: %6d / %6d bytes (%d%%)\n" $DRAM_USED $DRAM_LIMIT $DRAM_PCT
printf "RTC FAST: %d bytes\n" ${RTC_FAST_USED:-0}
printf "RTC SLOW: %d bytes\n" ${RTC_SLOW_USED:-0}
echo "-------------------------------"

status=0

check_limit () {
  local name=$1 used=$2 limit=$3
  local p=$(pct $used $limit)

  if (( p >= FAIL_PCT )); then
    echo "❌ FAIL: $name at ${p}%"
    status=2
  elif (( p >= WARN_PCT )); then
    echo "⚠️  WARN: $name at ${p}%"
    (( status < 1 )) && status=1
  else
    echo "✅ OK: $name"
  fi
}

check_limit "IRAM" $IRAM_USED $IRAM_LIMIT
check_limit "DRAM" $DRAM_USED $DRAM_LIMIT

echo "==============================="
exit $status
