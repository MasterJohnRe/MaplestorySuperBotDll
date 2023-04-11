#include "pch.h"
#include "fileHandler.h"
#include <fstream>

void FileHandler::log(std::string filePath , std::string message) {
    std::ofstream file;
    file.open(filePath , std::ios::app);

    if (file.is_open())
    {
        file << message;
        file << "\n";
        file.close();
    }
}