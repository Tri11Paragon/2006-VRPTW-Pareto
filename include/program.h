#pragma once
/*
 * Created by Brett on 30/09/23.
 * Licensed under GNU General Public License V3.0
 * See LICENSE file for license detail
 */

#ifndef INC_2006_VRPTW_PARETO_PROGRAM_H
#define INC_2006_VRPTW_PARETO_PROGRAM_H

#include <cstdint>

namespace ga
{
    
    class program
    {
        private:
            std::size_t customers;
        public:
            explicit program(std::size_t customers);
            
            ~program();
            
            program(const program& p) = delete;
            
            program& operator=(const program& p) = delete;
            
            int execute();
    };
    
}

#endif //INC_2006_VRPTW_PARETO_PROGRAM_H
