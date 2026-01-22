#pragma once

#include <cstddef>

/**
 * @class Node
 * @brief Represents a node with a unique identifier.
 *
 * The Node class provides functionality for generating and storing
 * a unique alphanumeric node ID.
 */
class Node {
private:
    static constexpr std::size_t ID_LENGTH = 6;
    static constexpr char CHARSET[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    static constexpr std::size_t CHARSET_LENGTH = sizeof(CHARSET) - 1; // exclude '\0'

    char nodeID[ID_LENGTH + 1]; // +1 for null terminator

public:
    /**
     * @brief Constructs a Node instance and automatically generates a node ID.
     */
    Node();

    /**
     * @brief Retrieves the current node ID.
     *
     * @return The node ID as a null-terminated C string.
     */
    const char* getNodeID() const;
};
