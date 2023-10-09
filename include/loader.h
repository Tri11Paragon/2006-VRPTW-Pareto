//
// Created by brett on 09/10/23.
//

#ifndef INC_2006_VRPTW_PARETO_LOADER_H
#define INC_2006_VRPTW_PARETO_LOADER_H

#include <cstdint>
#include <vector>
#include <string>

struct record
{
    std::int32_t customer_number;
    double x, y;
    double demand;
    double ready;
    double due;
    double service_time;
};

std::vector<record> load_problem(const std::string& path);

#endif //INC_2006_VRPTW_PARETO_LOADER_H
