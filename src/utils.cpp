#include <Arduino.h>
#include "utils.h"
#include "config.h"

/**
 * @brief Constructs the endpoint URL.
 * @param endpoint The endpoint to be appended to the URL.
 * @return char* - The constructed endpoint.
 */
char* constr_endp(const char* endpoint)
{
    static char endp[200];  // Static buffer to persist after function returns

    if (strncmp(endpoint, "", sizeof(endpoint)) == 0)
    {
        PRINT_STR("Endpoint is empty");
        return NULL;
    }
    
    snprintf(endp, sizeof(endp), "http://%s:%s%s", API_HOST, API_PORT, endpoint);
    return endp;
}

