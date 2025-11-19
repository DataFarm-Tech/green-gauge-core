#include "Node.hpp"
#include <cstdlib>   // for rand() and srand()
#include <ctime>     // for time()
#include <string>

Node::Node() {
    // Seed the random number generator once
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(nullptr)));
        seeded = true;
    }

    char c;
    int rand_val;
    const size_t id_len = 6;

    for (size_t i = 0; i < id_len; ++i) {
        rand_val = rand() % 62; // 10 digits + 26 uppercase + 26 lowercase

        if (rand_val < 10) {
            c = '0' + rand_val;          // digits 0-9
        } else if (rand_val < 36) {
            c = 'A' + (rand_val - 10);   // uppercase A-Z
        } else {
            c = 'a' + (rand_val - 36);   // lowercase a-z
        }

        nodeID += c;
    }
}

std::string Node::getNodeID() const {
    return nodeID;
}
