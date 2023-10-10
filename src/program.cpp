/*
 * Created by Brett on 30/09/23.
 * Licensed under GNU General Public License V3.0
 * See LICENSE file for license detail
 */
#include <program.h>

#include <blt/std/logging.h>
#include <valarray>

namespace ga
{
    
    std::int32_t capacity;
    std::vector<record> records;
    
    void init(std::int32_t c, std::vector<record>&& r)
    {
        capacity = c;
        records = std::move(r);
    }
    
    void destroy()
    {
    
    }
    
    int execute()
    {
        return 0;
    }
    
    double distance(std::int32_t c1, std::int32_t c2)
    {
        auto customer1 = records[c1];
        auto customer2 = records[c2];
        auto x = customer1.x - customer2.x;
        auto y = customer1.y - customer2.y;
        return std::sqrt(x * x + y * y);
    }
    
    std::vector<route> constructRoute(const chromosome& c)
    {
        std::vector<route> routes;
        
        const double dueTime = records[0].due;
        
        // phase 1
        int index = 0;
        while (index < CUSTOMER_COUNT) {
            route currentRoute;
            
            double currentCapacity = 0;
            double lastDepartTime = records[0].ready;
            
            while (index < CUSTOMER_COUNT){
                auto r = records[c.genes[index]];
                // constraint violated, add route and reset
                // we assume when a vehicle leaves it will teleport to the next destination immediately but must be able to service BEFORE closing
                // if this isn't the intended behaviour remove the r.service_time from the second condition
                // lastDepartTime + r.service_time is consistent with "and must return before or at time bn+1"
                if (currentCapacity + r.demand > capacity || lastDepartTime + r.service_time > r.due || lastDepartTime + r.service_time > dueTime)
                    break;
                currentCapacity += r.demand;
                // wait until the server opens, add the service time, move on
                lastDepartTime = std::max(lastDepartTime, r.ready) + r.service_time;
                currentRoute.customers.push_back(c.genes[index++]);
            }
            
            routes.push_back(currentRoute);
        }
        
        // phase 2
        
        
        return routes;
    }
    
}
