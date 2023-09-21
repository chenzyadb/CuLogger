#include "CuLogger.h"

CuLogger* CuLogger::instance_ = nullptr;
std::once_flag CuLogger::flag_{};
