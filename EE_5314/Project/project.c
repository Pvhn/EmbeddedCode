/*
 * project.c
 *
 *  Created on: Nov 10, 2020
 *      Author: peter
 *
 * This header contains all functions for string processing
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "project.h"

// Defines used for argument type
#define ALPHA 'a'
#define NUMBER 'n'
#define DELIM 'd'

// Convert character to integer
int32_t my_atoi(const char *str)
{
    int32_t result = 0;
    int8_t sign = 1;
    int i = 0;
    if (str[0] == '-') //Sign of number
    {
        sign = -1;
        i++;
    }
    for (; str[i] != '\0'; i++)
    {
        if (!(str[i] >= '0' && str[i] <= '9')) //Is c a number
        {
            return 0;
        }
        result = result * 10 + str[i] - '0';
    }
    return sign * result;
}

// Convert integer to character


// Compare two strings
bool my_strcmp(char *input, const char str[])
{
    int i = 0;
    while (input[i] != '\0' || str[i] != '\0')
    {
        if (input[i] == str[i])
        {
            i++;
        }
        else
        {
            return false;
        }
    }
    return true;
}

void parse(USER_DATA *data)
{
    uint8_t arg_count = 0;
    uint8_t i;
    char prev_type = DELIM;
    char c;
    for (i = 0; data->buffer[i] != '\0'; i++)
    {
        c = data->buffer[i];
        if (c >= '0' && c <= '9' && prev_type != NUMBER)
        {
            data->Type[arg_count] = NUMBER;
            prev_type = data->Type[arg_count];
            data->Index[arg_count++] = i;
        }
        else if (((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                && prev_type != ALPHA)
        {
            data->Type[arg_count] = ALPHA;
            prev_type = data->Type[arg_count];
            data->Index[arg_count++] = i;
        }
        else if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                || (c >= '0' && c <= '9') || c == '-'))
        {
            prev_type = DELIM;
            data->buffer[i] = '\0';
        }
        if (arg_count > MAX_ARGS)
        {
            arg_count = 0;
            break;
        }
    }
    data->argCount = arg_count;
#ifdef DEBUG
    for (i = 0; i < data->argCount; i++)
    {
        putcUart0(data->Type[i]);
        putcUart0(',');
        putsUart0(&data->buffer[data->Index[i]]);
        putcUart0('\n');

    }
#endif
}

char* getFieldString(USER_DATA *data, uint8_t fieldNumber)
{
    if (fieldNumber < data->argCount)
    {
        return &(data->buffer[data->Index[fieldNumber]]);
    }
    else
    {
        return '\0';
    }
}

uint32_t getFieldValue(USER_DATA *data, uint8_t fieldNumber)
{
    if (fieldNumber < data->argCount && data->Type[fieldNumber] == NUMBER)
    {
        return my_atoi(&data->buffer[data->Index[fieldNumber]]);
    }
    else
    {
        return 0;
    }
}

bool isCommand(USER_DATA *data, const char strCommand[], uint8_t minArguments)
{
    int i;
    char *command = &data->buffer[data->Index[0]];
    if (minArguments == data->argCount - 1)
    {
        for (i = 1; i < data->argCount; i++)
        {
            if (data->Type[i] != NUMBER) //Check if other args are numerical
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
    return my_strcmp(command, strCommand);
}
