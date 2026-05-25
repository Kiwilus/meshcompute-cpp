#include "node/node_core.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string config = "mesh_node.json";
    if (argc > 1) config = argv[1];

    meshcompute::NodeCore node(config);
    node.run();
    return 0;
}