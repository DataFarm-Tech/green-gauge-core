#pragma once

#include <string>
#include <cstdint>
#include "Types.hpp"
/**
 * @class Node
 * @brief Represents a node with a unique identifier.
 *
 * The Node class provides functionality for generating and storing a unique 6-character
 * node ID. This ID can be used for device identification in a networked environment.
 */
class Node {
private:
    char nodeID[GEN_STRING_SIZE];  ///< Stores the unique node identifier (6 chars + null terminator)

public:
    /**
     * @brief Constructs a Node instance and automatically generates a node ID.
     */
    Node();

    /**
     * @brief Retrieves the current node ID.
     *
     * @return The 6-character node ID as a C-style string.
     */
    const char* getNodeID() const;
};