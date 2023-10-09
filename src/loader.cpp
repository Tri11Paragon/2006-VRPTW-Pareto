//
// Created by brett on 09/10/23.
//
#include <loader.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>

std::vector<record> load_problem(const std::string& path)
{
    std::vector<record> records;
    std::ifstream input(path);
    
    // read first line, contains useless text
    std::string line;
    std::getline(input, line);
    
    double values[7];
    while (std::getline(input, line)) {
        std::istringstream stream(line);
        std::copy(std::istream_iterator<double>(stream), std::istream_iterator<double>(), values);
        records.emplace_back(static_cast<int>(values[0]), values[1], values[2], values[3], values[4], values[5], values[6]);
    }
    
    return records;
}
