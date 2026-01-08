#include "IPacket.hpp"
#include <vector>
#include <array>


struct sensorReading
{
    float temp;
    float ph;
};


class ReadingPacket : public IPacket {
    private:
        static constexpr size_t READING_SIZE = 35;
        std::array<sensorReading, READING_SIZE> readingList;
        std::string TAG;

    public:
        ReadingPacket(const std::string& id, const std::string& uri_endpoint,
                    const std::string& tag = "ReadingPacket")
            : TAG(tag)
        {
            nodeId = id;
            uri = uri_endpoint;
        }

        const uint8_t * toBuffer() override;
        
    
        /**
         * Will read from sensor convert raw sensor reading into sensorReading.
         * Add new sensorReading into readingList
         * Delay for 10ms
         * does that stuff 50 times.
         */
        void readSensor();
};