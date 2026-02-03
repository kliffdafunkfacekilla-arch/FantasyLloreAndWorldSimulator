#include <iostream>
#include <string>

// PNG/RAW Importer
class HeightmapLoader {
public:
    void load(const std::string& filename) {
        std::cout << "Loading heightmap from " << filename << "..." << std::endl;
    }
};
