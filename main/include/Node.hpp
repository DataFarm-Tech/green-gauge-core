#pragma once

#include <string>
#include <cstdint>

/**
 * @class Node
 * @brief Represents a node with a unique identifier.
 *
 * The Node class provides functionality for generating and storing a unique 6-character
 * node ID. This ID can be used for device identification in a networked environment.
 */
class Node {
private:
    std::string nodeID;  ///< Stores the unique node identifier

public:
    /**
     * @brief Constructs a Node instance and automatically generates a node ID.
     */
    Node();

    /**
     * @brief Retrieves the current node ID.
     *
     * @return The 6-character node ID as a std::string.
     */
    std::string getNodeID() const;
};