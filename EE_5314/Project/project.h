/*
 * my_string.h
 *
 *  Created on: Nov 10, 2020
 *      Author: peter
 */

#ifndef PROJECT_H_
#define PROJECT_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_CHARS 80
#define MAX_ARGS 5



typedef struct USER_DATA
{
    char buffer[MAX_CHARS + 1];
    uint8_t argCount;
    uint8_t Index[MAX_ARGS];
    char Type[MAX_ARGS];
} USER_DATA;

int32_t my_atoi(const char *str);
//char itoa(uint32_t value);
bool my_strcmp(char *input, const char str[]);
void parse(USER_DATA *data);
char* getFieldString(USER_DATA *data, uint8_t fieldNumber);
uint32_t getFieldValue(USER_DATA *data, uint8_t fieldNumber);
bool isCommand(USER_DATA *data, const char strCommand[], uint8_t minArguments);


#endif /* PROJECT_H_ */
