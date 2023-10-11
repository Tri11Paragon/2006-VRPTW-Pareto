#pragma once
/*
 * Created by Brett on 30/09/23.
 * Licensed under GNU General Public License V3.0
 * See LICENSE file for license detail
 */

#ifndef INC_2006_VRPTW_PARETO_PROGRAM_H
#define INC_2006_VRPTW_PARETO_PROGRAM_H

#include <cstdint>
#include <loader.h>
#include <array>

namespace ga
{
    
    static constexpr std::int32_t CUSTOMER_COUNT = 100;
    static constexpr std::int32_t POPULATION_SIZE = 300;
    static constexpr std::int32_t GENERATION_COUNT = 350;
    static constexpr double CROSSOVER_RATE = 0.8;
    static constexpr double MUTATION_RATE = 0.1;
    
    struct chromosome
    {
        std::array<std::int32_t, CUSTOMER_COUNT> genes;
    };
    
    struct route
    {
        std::vector<std::int32_t> customers;
        double total_distance;
    };
    
    struct individual
    {
        chromosome c;
        std::vector<route> routes;
        std::int32_t rank = 0;
    };
    
    struct population
    {
        std::vector<individual> pops;
    };
    
    double distance(std::int32_t c1, std::int32_t c2);
    
    std::vector<route> constructRoute(const chromosome& c);
    
    chromosome createRandomChromosome();
    
    void rankPopulation();
    
    void init(std::int32_t c, std::vector<record>&& r);
    
    void destroy();
    
    int execute();
    
}

#endif //INC_2006_VRPTW_PARETO_PROGRAM_H