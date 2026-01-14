#include "Node.hpp"
#include <cstdlib>   // rand(), srand()
#include <ctime>     // time()

Node::Node() {
    // Seed the random number generator once
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(nullptr)));
        seeded = true;
    }

    for (std::size_t i = 0; i < ID_LENGTH; ++i) {
        nodeID[i] = CHARSET[rand() % CHARSET_LENGTH];
    }

    nodeID[ID_LENGTH] = '\0';
}

const char* Node::getNodeID() const {
    return nodeID;
}
