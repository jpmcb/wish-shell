/*********************
 * Author: John McBride
 * Email: mcbridej@oregonstate.edu
 * CS 344 - Operating systems
 * Date: May 27th 2018
 *
 * Description: The various utility functions used throughout the wish program
 *      Includes redirection functions, the built in cd function, and the $$ expansion function
 * ********************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>

#include "wish.h"


/********************
 * builtIn_cd
 * Description: Implemnets the built in cd (change directory) functionality
 * -----
 * Input: path - string that points to a relitive or absolute path. May also be a null string
 * Output: NA - the current working diretory will now be changed or an error message will be displayed
 * ******************/

void builtIn_cd(char *path)
{
    // No arguments provided, go to the home directory
    if(path == 0)
    {
        // Get the home environment variable and go to that directory
        if(chdir(getenv("HOME")) != 0)
        {
            perror("Error - HOME");
        }
    }

    // One argument provided, the path to a relative or absolute directory
    else
    {
        // Attempt to go to change directories into the path
        if(chdir(path) != 0)
        {
            perror("Error - provided path");
        }
    }
}


/*********************
 * expandProcessID
 * Description: Expands the $$ for the input string
 * -----
 * Input: argList - the pointer array to string that represents the input from the command line
 *        argString - The index of argList of which string holds the $$ variable
 *        argChar  the index of the argString that holds the exact location of $$
 * Output: NA - The $$ will be expanded in the passed in string to the processID
 * *******************/

void expandProcessID(char ** argList, int argString, int argChar)
{
    // Two temporary buffers
    char firstPart[ARG_SIZE];
    char finalString[ARG_SIZE];

    // Set these buffers to NULL
    memset(firstPart, '\0', ARG_SIZE);
    memset(finalString, '\0', ARG_SIZE);

    // Copy the very first part of the string (up to the first instance of the provided $)
    strncpy(firstPart, argList[argString], argChar);

    // Build the string with the process ID next
    sprintf(finalString, "%s%d", firstPart, getpid());

    // Loop through the string characters after the last occurence of the $
    // and place them into the finalString buffer, character by character
    int k = 0;
    argChar += 2;
    while(argList[argString][argChar] != '\0')
    {
        // Loop until the finalString is at the end of it's buffer
        if(finalString[k] != '\0')
        {
            k++;
        }
        // Then, place each character from the actual argList string into the final string buffer
        else
        {
            finalString[k] = argList[argString][argChar];
            // Increment the char in both finalString and argList[argString] (the two strings)
            k++;
            argChar++;
        }
    } 

    // Free up the memory that was in the argList array
    free(argList[argString]);

    // ... and allocate some new memory for this string that is the size of finalString
    argList[argString] = (char*)malloc((sizeof(char) * strlen(finalString)) + 1);

    // Copy the final string into the new memory allocated for the argument list string
    strcpy(argList[argString], finalString);

    // Ensure the null terminator was copied over
    argList[argString][strlen(finalString)] = '\0';

    // The argList[argString] will now be the newly allocated string with the process ID expanded!
}


/********************
 * cleanShell
 * Description: Kills any children that need to be cleaned up and de-allocates memory 
 * -----
 * Input: argList - Dynamically allocated memory that will be freed
 *        background_ps - the background process id array. Each child process will be cleaned up and killed
 *        numPs - the number of process IDs
 * Output: NA - Shell is now ok to exit
 * ********************/

void cleanShell(char **argList, pid_t *background_ps, int numPs)
{
    // Kill all processes or jobs that the shell started
    int i, counter = 0;

    // Loop as long as the counter has not reached the number of background processes and not greater than
    // the max number of background processes
    while(counter != numPs && i < MAX_PS)
    {
        if(background_ps[i] != -5)
        {
            // If valid PID, sent that process a SIGTERM signal
            kill(background_ps[i], SIGTERM);
            counter++;
        }

        // Go to the next background process ID in the array
        i++;
    }
    
    // Call the clean up function for the dynamic memory in the argList array
    cleanBuffer(argList);
}


/*****************
 * redirectToNull
 * Description: Redirects both stdin and stdout to /dev/null
 * ------
 * Input: NA
 * Output: Returns -1 if failed, otherwise, returns 0 if succesfully redirected stdin and stdout
 * ***************/

int redirectToNull()
{
    // Open /dev/null for input and output
    int nullFile_write = open("/dev/null", O_WRONLY);
    int nullFile_read = open("/dev/null", O_RDONLY);

    // Check if opening worked
    if(nullFile_write == -1 || nullFile_read == -1)
    {
        perror("Error with opening dev null\n");
        return -1;
    }

    // Attempt redirection of stdin  and dispaly error as needed
    int result = dup2(nullFile_read, 0);
    if(result == -1)
    {
        perror("Error with dup2 using stdout and 0\n");
        return -1;
    }

    // Attempt redirection of stdout now. Display error as needed
    result = dup2(nullFile_write, 1);
    if(result == -1)
    {
        perror("Error with dup2 using stdin and 1\n");
        return -1;
    }

    // Close the file descriptors
    close(nullFile_write);
    close(nullFile_read);

    return 0;
}


/**********************
 * redirectStdin
 * Description: Redirects stdin to /dev/null
 * -----
 * Input: NA
 * Output: Returns -1 if failed, otherwise, returns 0 if successfully redirected stdin
 * *********************/

int redirectStdin()
{
    // Open the /dev/null file for reading
    int myFile = open("/dev/null", O_RDONLY);

    // Check if successfully opened
    if(myFile == -1)
    {
        perror("Error with openeing file for stdin");
        return -1;    
    }

    // Redirect stdin and dispaly error if failed
    int result = dup2(myFile, 0);
    if(result == -1)
    {
        perror("Error with dup2 in stdin\n");
        return -1;
    }

    close(myFile);

    return 0;
}


/*********************
 * redirectStdout
 * Description: Redirects stdout to /dev/null
 * ------
 * Input: NA
 * Output: Returns -1 if failed, otherwise 0 if successfully redirected stdout
 * *******************/

int redirectStdout()
{
    // Open dev/null for writing
    int myFile = open("/dev/null", O_WRONLY);

    // Check if error and dispaly error message
    if(myFile == -1)
    {
        perror("Error with opening stdout redirection file");
        return -1;
    }

    // Redirect stdout and dispaly error as needed
    int result = dup2(myFile, 1);
    if(result == -1)
    {
        perror("Error with dup2 in stdout");
        return -1;
    }

    close(myFile);

    return 0;
}
