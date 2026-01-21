#pragma once
#include <string.h>

 /**
  * @brief Calibration structure for NPK sensor
  */
enum class MeasurementType {
    Nitrogen,
    Phosphorus,
    Potassium,
    Moisture,
    PH
};

/**
 * @brief Table linking MeasurementType enums to string names.
 * Acts like a 2D array: each row contains the enum and its string.
 */
struct MeasurementEntry {
    MeasurementType type;
    const char* name;
};



constexpr const char* NITROGEN = "nitrogen";
constexpr const char* PHOSPHORUS = "phosphorus";
constexpr const char* POTASSIUM = "potassium";
constexpr const char* MOISTURE = "moisture";
constexpr const char* PH = "ph";

// Static constexpr table for compile-time mapping
static constexpr MeasurementEntry MEASUREMENT_TABLE[] = {
    { MeasurementType::Nitrogen,    NITROGEN },
    { MeasurementType::Moisture,    MOISTURE},
    { MeasurementType::PH,           PH},
    { MeasurementType::Phosphorus,  PHOSPHORUS},
    { MeasurementType::Potassium,  POTASSIUM}
};


class ReadingPacket;

/**
 * @brief Class for handling NPK sensor operations
 */
class NPK {
public:
    /**
     * @brief Constructor for NPK class
     */
    NPK();
    /**
     * @brief Collect NPK readings and store them in the provided ReadingPacket
     * @param readings Reference to ReadingPacket to store the readings
     */
    void npk_collect(ReadingPacket& readings);

    /**
     * @brief Calibrate the NPK sensor
     */
    void npk_calib();
};
