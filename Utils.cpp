#include "Utils.h"

#include <stdexcept>
#include <fstream>

using std::string;
using std::vector;
using std::ifstream;


vector<unsigned char> Utils::readFile(const string& filename)
{
    ifstream inFile(filename, std::ios::ate | std::ios::binary);

    if (!inFile.is_open()) {
        throw std::runtime_error("failed to open file");
    }

    size_t fileSize = (size_t)inFile.tellg();
    vector<unsigned char> buffer(fileSize);

    inFile.seekg(0, std::ios_base::beg);
    inFile.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    inFile.close();

    return buffer;
}
