// LoggerService.h
#pragma once
#include "IService.h"
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <iostream>

class LoggerService : public IService
{
private:
    std::ofstream file;
    std::mutex mtx;

public:
    LoggerService(const std::string &filename = "debug.log")
    {
        file.open(filename, std::ios::app);
        if (!file.is_open())
        {
            throw std::runtime_error("No se pudo abrir el archivo de log");
        }
    }

    ~LoggerService()
    {
        if (file.is_open())
            file.close();
    }

    void log(const std::string &msg, const char *fileName, int line)
    {
        std::lock_guard<std::mutex> lock(mtx);

        std::time_t t = std::time(nullptr);
        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

        file << "[" << buffer << "] "
             << fileName << ":" << line << " -> " << msg << "\n";
        file.flush();
    }
};
