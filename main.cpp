#include "VulkanTriangle.h"
#include "Logging.h"

using std::endl;
using std::cerr;
using std::exception;


int main()
{
    std::cout.setf(std::ios::boolalpha);
    std::cerr.setf(std::ios::boolalpha);

    VulkanTriangleApp app;

    try
    {
        app.run();
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
