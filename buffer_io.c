/**********************
 * Author: John McBride
 * Date: May 27th 2018
 *
 * Description: Several utility functions that work with the input 
 *      buffer for the command line and it's arguments as part of the wish implementation.
 * **********************/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wish.h"


/************************
 * getcommandLine
 * Description: updates the argList array with the various commands input at the command line.
 * ------
 * Input: inNum - Will be updated with the number of commands entered on the command line
 *        arglist - Empty array of pointers to strings (pointers to pointers). This array will be
 *        updated with each word (delimited by spaces) entered on the commandl ine
 * Output: NA - inNum and argList will both be updated with input from the command line
 * ***********************/

void getCommandLine(int *inNum, char **argList)
{
    // Temporary buffer to get input from the command line. Set it's memory to null
    char inBuffer[LINE_SIZE];
    memset(inBuffer, '\0', sizeof(LINE_SIZE));

    // Print out the prompt
    write(1, ":", 1); 

    // Get the command line from the user and place it into the inBuffer
    fgets(inBuffer, LINE_SIZE, stdin);

    // Place a newline character on the first occurence of the newline character. 
    // This ensures there is no buffer overflow or over bounding of the input buffer
    inBuffer[strcspn(inBuffer, "\n")] = '\0';

    // bust the string up into tokens delimited by spaces
    char *token = strtok(inBuffer, " ");

    // Loop through each of the space delimited words in the command line (until the token returns null)
    int i = 0;
    while(token != 0)
    {
        // Get the length of the current token
        int tokenLen = strlen(token);

        // Allocate enough space for that token
        argList[i] = (char*)malloc((sizeof(char) * tokenLen) + 1);

        // Copy the string over to the argList array
        strcpy(argList[i], token);

        // Ensure the new line char is in the right spot
        argList[i][tokenLen] = '\0';

        // Go to the next token and increment the counter
        token = strtok(NULL, " ");
        i++;
    }

    // Set the counter to the number of arguments processed
    *inNum = i;
}


/************************
 * cleanBuffer
 * Description: Cleans up the allocated memory for the argList array
 * -----
 * Input: argList - The array of pointers to strings where the strings memory was dynamically allocated.
 *          see the getcommandLine function above for allocation details.
 * Output: NA - Memory will be freed and the argList will be ready for the next round of input
 * ***********************/

void cleanBuffer(char **argList)
{
    // Loop through the argList array (until a null string is reached)
    int i = 0;
    while(argList[i] != 0)
    {
        // Free up the dynamic memory and set the pointer to NULLPTR (zero)
        free(argList[i]);
        argList[i] = 0;
        i++;
    }
}
