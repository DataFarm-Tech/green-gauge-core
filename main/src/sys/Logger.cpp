// #include "Logger.hpp"
// #include <cstdio>
// #include <cstdarg>
// #include <cstring>
// #include <ctime>
// #include <sys/stat.h>
// #include <dirent.h>
// #include <algorithm>
// #include <vector>
// #include <string>
// #include "esp_log.h"
// #include "esp_littlefs.h"

// static const char* TAG = "Logger";

// namespace {
// struct PendingPktFile {
//     std::string full_path;
//     uint32_t sequence;
// };

// bool is_digits_only(const char* str) {
//     if (!str || *str == '\0') {
//         return false;
//     }

//     for (const char* p = str; *p != '\0'; ++p) {
//         if (*p < '0' || *p > '9') {
//             return false;
//         }
//     }
//     return true;
// }
// /**
//  * @brief Collect pending packet files matching the given filename pattern.
//  * Scans the basePath directory for files named like "filename.N" where N is a numeric sequence.
//  * Returns a sorted vector of PendingPktFile structs containing the full path and sequence number.
//  */
// std::vector<PendingPktFile> collect_packet_files(const char* basePath, const char* filename) {
//     std::vector<PendingPktFile> files;
//     DIR* dir = opendir(basePath);
//     if (!dir) {
//         return files;
//     }

//     const size_t filename_len = strlen(filename);
//     struct dirent* entry = nullptr;

//     while ((entry = readdir(dir)) != nullptr) {
//         const char* name = entry->d_name;
//         if (strncmp(name, filename, filename_len) != 0) {
//             continue;
//         }

//         if (name[filename_len] != '.') {
//             continue;
//         }

//         const char* suffix = name + filename_len + 1;
//         if (!is_digits_only(suffix)) {
//             continue;
//         }

//         unsigned long seq = strtoul(suffix, nullptr, 10);
//         std::string full_path = std::string(basePath) + "/" + name;
//         files.push_back(PendingPktFile{full_path, static_cast<uint32_t>(seq)});
//     }

//     closedir(dir);

//     std::sort(files.begin(), files.end(), [](const PendingPktFile& a, const PendingPktFile& b) {
//         return a.sequence < b.sequence;
//     });

//     return files;
// }

// uint32_t get_next_packet_sequence(const std::vector<PendingPktFile>& files) {
//     uint32_t max_sequence = 0;
//     for (const auto& file : files) {
//         if (file.sequence > max_sequence) {
//             max_sequence = file.sequence;
//         }
//     }
//     return max_sequence + 1;
// }
// }

// /**
//  * @brief Global Logger instance.
//  * This instance can be used throughout the application.
//  */
// Logger g_logger; // Global instance

// Logger::Logger(const char* basePath_)
//     : basePath(basePath_), initialized(false) {}


// esp_err_t Logger::init() {
//     if (initialized) return ESP_OK;

//     // Create mutex BEFORE filesystem init
//     if (!mutex) {
//         mutex = xSemaphoreCreateMutex();
//         if (!mutex) {
//             ESP_LOGE(TAG, "Failed to create logger mutex");
//             return ESP_FAIL;
//         }
//     }

//     esp_vfs_littlefs_conf_t conf = {};  // zero-initialize

//     conf.base_path = basePath;
//     conf.partition_label = "littlefs";
//     conf.format_if_mount_failed = true;
//     conf.dont_mount = false;
//     conf.read_only = false;
//     conf.grow_on_mount = true;
//     conf.partition = nullptr;

//     esp_err_t ret = esp_vfs_littlefs_register(&conf);
//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to mount LittleFS: %s", esp_err_to_name(ret));
//         return ret;
//     }

//     initialized = true;
//     ESP_LOGI(TAG, "Logger initialized at %s", basePath);
//     return ESP_OK;
// }

// bool Logger::hasSpace(size_t bytes) {
//     size_t total = 0, used = 0;
//     if (esp_littlefs_info("littlefs", &total, &used) != ESP_OK) {
//         return false;
//     }
//     return (total - used) >= (bytes + MIN_FREE_SPACE);
// }

// esp_err_t Logger::rotateIfNeeded(const char* filename, size_t newSize) {
//     char path[256];
//     snprintf(path, sizeof(path), "%s/%s", basePath, filename);

//     struct stat st;
//     if (stat(path, &st) == 0 && st.st_size + newSize > MAX_LOG_SIZE) {
//         // Rotate old backups
//         for (int i = MAX_LOG_BACKUPS - 1; i > 0; i--) {
//             char old[256], newer[256];
//             snprintf(old, sizeof(old), "%s/%s.%d", basePath, filename, i);
//             snprintf(newer, sizeof(newer), "%s/%s.%d", basePath, filename, i + 1);
//             rename(old, newer); // Ignore errors if file doesn't exist
//         }
        
//         // Move current to .1
//         char backup[256];
//         snprintf(backup, sizeof(backup), "%s/%s.1", basePath, filename);
//         rename(path, backup);
        
//         ESP_LOGI(TAG, "Rotated %s", filename);
//     }
//     return ESP_OK;
// }


// esp_err_t Logger::flushPkts(const char* filename, std::function<bool(const uint8_t*, size_t)> send_cb) {
//     if (!initialized) return ESP_ERR_INVALID_STATE;

//     if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
//         ESP_LOGE(TAG, "Failed to acquire mutex for flushPkts");
//         return ESP_ERR_TIMEOUT;
//     }

//     char legacy_path[256];
//     snprintf(legacy_path, sizeof(legacy_path), "%s/%s", basePath, filename);

//     struct stat legacy_st;
//     bool has_legacy_file = (stat(legacy_path, &legacy_st) == 0);
//     std::vector<PendingPktFile> pkt_files = collect_packet_files(basePath, filename);

//     if (!has_legacy_file && pkt_files.empty()) {
//         ESP_LOGI(TAG, "No pending packets file found, skipping flush");
//         xSemaphoreGive(mutex);
//         return ESP_OK;
//     }

//     int total = 0;
//     int sent = 0;
//     int retained = 0;

//     // Legacy support: old aggregated packet file format
//     if (has_legacy_file) {
//         FILE* f = fopen(legacy_path, "rb");
//         if (!f) {
//             ESP_LOGE(TAG, "Failed to open %s for reading", legacy_path);
//             xSemaphoreGive(mutex);
//             return ESP_FAIL;
//         }

//         std::vector<std::vector<uint8_t>> failed_pkts;

//         while (true) {
//             uint8_t size_buf[2];
//             if (fread(size_buf, 1, sizeof(size_buf), f) != sizeof(size_buf)) {
//                 break;
//             }

//             uint16_t pkt_size = static_cast<uint16_t>(size_buf[0]) |
//                                 (static_cast<uint16_t>(size_buf[1]) << 8);

//             std::vector<uint8_t> pkt(pkt_size);
//             if (fread(pkt.data(), 1, pkt_size, f) != pkt_size) {
//                 ESP_LOGE(TAG, "Corrupt packet in %s, stopping read", filename);
//                 break;
//             }

//             total++;
//             xSemaphoreGive(mutex);

//             if (send_cb(pkt.data(), pkt_size)) {
//                 sent++;
//                 ESP_LOGI(TAG, "Flushed queued packet %d successfully", total);
//             } else {
//                 failed_pkts.push_back(std::move(pkt));
//                 retained++;
//                 ESP_LOGW(TAG, "Failed to flush queued packet %d, will retain", total);
//             }

//             if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
//                 ESP_LOGE(TAG, "Failed to reacquire mutex after send");
//                 fclose(f);
//                 return ESP_ERR_TIMEOUT;
//             }
//         }

//         fclose(f);

//         if (failed_pkts.empty()) {
//             remove(legacy_path);
//         } else {
//             FILE* fw = fopen(legacy_path, "wb");
//             if (!fw) {
//                 ESP_LOGE(TAG, "Failed to rewrite %s", legacy_path);
//                 xSemaphoreGive(mutex);
//                 return ESP_FAIL;
//             }

//             for (const auto& pkt : failed_pkts) {
//                 uint16_t pkt_size = static_cast<uint16_t>(pkt.size());
//                 uint8_t size_buf[2] = {
//                     static_cast<uint8_t>(pkt_size & 0xFF),
//                     static_cast<uint8_t>((pkt_size >> 8) & 0xFF)
//                 };
//                 fwrite(size_buf, 1, sizeof(size_buf), fw);
//                 fwrite(pkt.data(), 1, pkt.size(), fw);
//             }

//             fclose(fw);
//         }
//     }

//     // New format: one packet per file, each file contains raw packet bytes
//     for (const auto& pkt_file : pkt_files) {
//         FILE* f = fopen(pkt_file.full_path.c_str(), "rb");
//         if (!f) {
//             ESP_LOGE(TAG, "Failed to open %s for reading", pkt_file.full_path.c_str());
//             continue;
//         }

//         if (fseek(f, 0, SEEK_END) != 0) {
//             fclose(f);
//             continue;
//         }

//         long file_size = ftell(f);
//         if (file_size <= 0) {
//             fclose(f);
//             continue;
//         }

//         if (fseek(f, 0, SEEK_SET) != 0) {
//             fclose(f);
//             continue;
//         }

//         std::vector<uint8_t> pkt(static_cast<size_t>(file_size));
//         if (fread(pkt.data(), 1, pkt.size(), f) != pkt.size()) {
//             fclose(f);
//             ESP_LOGE(TAG, "Failed to read packet file %s", pkt_file.full_path.c_str());
//             continue;
//         }

//         fclose(f);
//         total++;

//         xSemaphoreGive(mutex);
//         bool ok = send_cb(pkt.data(), pkt.size());
//         if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
//             ESP_LOGE(TAG, "Failed to reacquire mutex after send");
//             return ESP_ERR_TIMEOUT;
//         }

//         if (ok) {
//             sent++;
//             remove(pkt_file.full_path.c_str());
//             ESP_LOGI(TAG, "Flushed queued packet file %s", pkt_file.full_path.c_str());
//         } else {
//             retained++;
//             ESP_LOGW(TAG, "Failed to flush queued packet file %s, retaining", pkt_file.full_path.c_str());
//         }
//     }

//     ESP_LOGI(TAG, "Flush complete: %d sent, %d retained, %d total", sent, retained, total);

//     xSemaphoreGive(mutex);
//     return ESP_OK;
// }

// esp_err_t Logger::flushPendingPkts(std::function<bool(const uint8_t*, size_t)> send_cb) {
//     return flushPkts(PENDING_PKT_FILENAME, send_cb);
// }


// uint8_t Logger::getPktCount(const char* filename) {
//     uint16_t count = 0;
//     char legacy_path[256];

//     std::vector<PendingPktFile> pkt_files = collect_packet_files(basePath, filename);
//     count += static_cast<uint16_t>(pkt_files.size());

//     // Include legacy aggregate file packets if present
//     snprintf(legacy_path, sizeof(legacy_path), "%s/%s", basePath, filename);
//     FILE* f = fopen(legacy_path, "rb");
//     if (f) {
//         uint8_t size_buf[2];
//         while (fread(size_buf, 1, sizeof(size_buf), f) == sizeof(size_buf)) {
//             uint16_t pkt_size = static_cast<uint16_t>(size_buf[0]) |
//                                 (static_cast<uint16_t>(size_buf[1]) << 8);
//             if (fseek(f, pkt_size, SEEK_CUR) != 0) {
//                 break;
//             }
//             count++;
//             if (count == UINT8_MAX) {
//                 break;
//             }
//         }
//         fclose(f);
//     }

//     if (count > UINT8_MAX) {
//         return UINT8_MAX;
//     }

//     return static_cast<uint8_t>(count);
// }

// uint8_t Logger::getPendingPktCount() {
//     return getPktCount(PENDING_PKT_FILENAME);
// }

// esp_err_t Logger::writePkt(const char * filename, const uint8_t *pkt, size_t pkt_len) {
//     if (!initialized) return ESP_ERR_INVALID_STATE;

//     if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
//         ESP_LOGE(TAG, "Failed to acquire logger mutex for writePkt");
//         return ESP_ERR_TIMEOUT;
//     }

//     if (!hasSpace(pkt_len)) {
//         ESP_LOGE(TAG, "Insufficient space to write packet");
//         xSemaphoreGive(mutex);  // RELEASE before return
//         return ESP_ERR_NO_MEM;
//     }

//     std::vector<PendingPktFile> pkt_files = collect_packet_files(basePath, filename);
//     uint32_t next_sequence = get_next_packet_sequence(pkt_files);

//     char path[256];
//     snprintf(path, sizeof(path), "%s/%s.%lu", basePath, filename, static_cast<unsigned long>(next_sequence));

//     FILE* f = fopen(path, "wb");
//     if (!f) {
//         ESP_LOGE(TAG, "Failed to open %s for writing packet", path);
//         xSemaphoreGive(mutex);  // RELEASE before return
//         return ESP_FAIL;
//     }

//     fwrite(pkt, 1, pkt_len, f);
//     fclose(f);
//     xSemaphoreGive(mutex);  // RELEASE MUTEX

//     return ESP_OK;
// }

// esp_err_t Logger::writePendingPkt(const uint8_t *pkt, size_t pkt_len, uint8_t queued_count) {
//     if (!initialized) return ESP_ERR_INVALID_STATE;

//     if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
//         ESP_LOGE(TAG, "Failed to acquire logger mutex for writePendingPkt");
//         return ESP_ERR_TIMEOUT;
//     }

//     if (!hasSpace(pkt_len)) {
//         ESP_LOGE(TAG, "Insufficient space to write pending packet");
//         xSemaphoreGive(mutex);
//         return ESP_ERR_NO_MEM;
//     }

//     uint32_t suffix = queued_count;
//     std::string path;
//     struct stat st;

//     while (true) {
//         path = std::string(basePath) + "/" + PENDING_PKT_FILENAME + "." + std::to_string(suffix);
//         if (stat(path.c_str(), &st) != 0) {
//             break;
//         }
//         ++suffix;
//     }

//     FILE* f = fopen(path.c_str(), "wb");
//     if (!f) {
//         ESP_LOGE(TAG, "Failed to open %s for writing packet", path.c_str());
//         xSemaphoreGive(mutex);
//         return ESP_FAIL;
//     }

//     fwrite(pkt, 1, pkt_len, f);
//     fclose(f);
//     xSemaphoreGive(mutex);

//     return ESP_OK;
// }


// void Logger::deinit() {
//     if (initialized) {
//         esp_vfs_littlefs_unregister("littlefs");
//         initialized = false;
//     }
//     if (mutex) {
//         vSemaphoreDelete(mutex);
//         mutex = nullptr;
//     }
// }