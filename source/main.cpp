#include <iostream>

#include "CuLogger.hpp"

int main(int argc, char* argv[])
{
    std::string logPath = argv[0];
    if (logPath.rfind("\\") != std::string::npos) {
        logPath = logPath.substr(0, logPath.rfind("\\")) + "\\log.txt";
    } else if (logPath.rfind("/") != std::string::npos) {
        logPath = logPath.substr(0, logPath.rfind("/")) + "/log.txt";
    }
    
    {
        CuLogger::CreateLogger(CuLogger::LOG_DEBUG, logPath);
    }

    {
        const auto &logger = CuLogger::GetLogger();
        logger->Error("This is log output.");
        logger->Warning("This is log output.");
        logger->Info("This is log output.");
        logger->Debug("This is log output.");
    }

    {
        const auto &logger = CuLogger::GetLogger();
        logger->ResetLogLevel(CuLogger::LOG_ERROR);
        logger->Error("This is log output.");
        logger->Warning("This is log output.");
        logger->Info("This is log output.");
        logger->Debug("This is log output.");
    }

    {
        const auto &logger = CuLogger::GetLogger();
        {
            std::cout << "test Singleton: " << (CuLogger::GetLogger() == logger) << std::endl;
        }
    }

    try {
        CuLogger::CreateLogger(CuLogger::LOG_NONE, CuLogger::LOG_PATH_NONE);
    } catch (std::exception e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
