#include "Utils.h"

#include <stdexcept>
#include <fstream>


std::vector<char> Utils::readFile(const std::string& filename)
{
    std::ifstream inFile(filename, std::ios::ate | std::ios::binary);

    if (!inFile.is_open()) {
        throw std::runtime_error("failed to open file");
    }

    size_t fileSize = (size_t)inFile.tellg();
    std::vector<char> buffer(fileSize);

    inFile.seekg(0);
    inFile.read(buffer.data(), fileSize);
    inFile.close();

    return buffer;
}
