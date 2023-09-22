#include <iostream>
#include <chrono>
#include "CuLogger.h"

int main(int argc, char* argv[])
{
    std::string logPath = argv[0];
    if (logPath.rfind("\\") != std::string::npos) {
        logPath = logPath.substr(0, logPath.rfind("\\")) + "\\log.txt";
    } else if (logPath.rfind("/") != std::string::npos) {
        logPath = logPath.substr(0, logPath.rfind("/")) + "/log.txt";
    }

    try {
        CuLogger::GetLogger();
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }
    
    CuLogger::CreateLogger(CuLogger::LOG_DEBUG, logPath);

    {
        auto logger = CuLogger::GetLogger();
        std::cout << "Singleton: " << ((CuLogger::GetLogger() == logger) ? "true" : "false") << std::endl;
    }

    {
        auto logger = CuLogger::GetLogger();
        logger->ResetLogLevel(CuLogger::LOG_ERROR);
        logger->Error("This is log output.");
        logger->Warning("This is log output.");
        logger->Info("This is log output.");
        logger->Debug("This is log output.");
    }

    {
        auto logger = CuLogger::GetLogger();
        logger->ResetLogLevel(CuLogger::LOG_INFO);
        logger->Error("This is log output.");
        logger->Warning("This is log output.");
        logger->Info("This is log output.");
        logger->Debug("This is log output.");
    }

    {
        std::thread thread0([&]() {
            auto &logger = *CuLogger::GetLogger();
            for (int i = 1; i <= 100000; i++) {
                logger.Info("thread0 log %lld.", i);
            }
        });
        thread0.detach();
    }
    
    {
        std::thread thread1([&]() {
            auto logger = CuLogger::GetLogger();
            for (int i = 1; i <= 100000; i++) {
                logger->Info("thread1 log %d.", i);
            }
        });
        thread1.detach();
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}
