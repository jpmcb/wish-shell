/**********************
 * Author: John McBride
 * Email: mcbridej@oregonstate.edu
 * CS 344 - Operating Systems
 * Date: May 27th 2018
 *
 * Description: Implementation of the wish shell. This shell supports
 *      basic file redirection, $$ process ID expansion, and other basic features.
 *      Refer to the various implementation details and comments below.
 * *******************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <signal.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h>

#include "wish.h" 

// The built in status tracker. Will be set to the exit status of the terminating program
int STATUS = 0;
int BACK_STATUS = 0;

// Flag for the signal terminating Ctr - C (SIGINT)
// Controls the flow of messages after a foreground child process has been terminated
int INT_MESSAGE = 0;

// Flags for the Ctrl - Z (SIGTSTP) signal
// Controls the background / foreground modes of the shell and the assosiated messages
int TSTP_FLAG = 0;
int TSTP_MESSAGE = 0;


/**********************
 * catch_SIGINT
 * Description: Signal handler function for the SIGINT signal. Executed after the user
 *      enters ctrl-c to exit a foreground process.
 * -----
 * Input: signum - signal number / data from signal (not used in handler)
 * Output: NA - sets the sigint message global flag
 * ********************/
void catch_SIGINT(int signum)
{
    INT_MESSAGE = 1;
}


/********************
 * catch_SIGTSTP
 * Description: Signal handler function for the SIGTSTP signal. Function will be executed
 *      after user enters ctrl-z. This will place the shell in "foreground-only" mode
 * -----
 * Input: signum - signal number / data from signal (not used in handler)
 * Output: NA - sets the sigtstp message and control global flags
 * ******************/

void catch_SIGTSTP(int signum)
{
    // Foreground-only mode is currently on
    if(TSTP_FLAG)
    {
        // Set control flag off
        TSTP_FLAG = 0;
        TSTP_MESSAGE = 2;
    }
    // Foreground-only mode is currently off
    else
    {
        // Set control flag on
        TSTP_FLAG = 1;
        TSTP_MESSAGE = 1;
    }
}


/*******************
 * Main method
 * Description: 
 * ------
 * Input: NA - expects no user, command line input at start of program
 * Output: NA - Performs the various small shell operations and program flow
 * ****************/

int main(void)
{
    // Used for various loop counters
    int i, j; 

    // Tracks the number of arguments (including program command) that was entered
    // Example: if user enters `echo hello world` on the command prompt, then numArgs will be 3
    int numArgs = 0;

    // Control flow flag for if the process is to be run in the background
    int background_flag, background_msg = 0;

    // Control flow flag for if the stdin is being redirected
    int stdin_flag = 0;

    // Control flow flag for if the stdout is being redirected
    int stdout_flag = 0;

    // Tracks the number of current background processes
    int numPs = 0;

    // Array to hold the current background processes. Initlized to empty, junk process id values
    pid_t background_ps[MAX_PS];
    for(i = 0; i < MAX_PS; i++)
        background_ps[i] = -5;

    // Control flow flag for if there was a redirection error (with the < or > operators)
    int redirectErrFlag = 0;

    // Initlize array of strings (pointers), all initially set to NULLPTR (zero)
    // This array will hold the various command prompt inputs
    char *argList[ARG_SIZE];
    for(i = 0; i < ARG_SIZE; i++)
        argList[i] = 0;

    // For spawning a new child process and control flow within the shell
    pid_t spawnPid = -5; 

    // Sigaction structs for handling the various incoming signals
    struct sigaction sigint_struct, sigtstp_struct, ignore_action;

    // --- SIGINT ---
    // Set up the handler and structure for the SIGINT signal. 
    // This signal will be ignored for all background processes
    sigint_struct.sa_handler = catch_SIGINT;
    sigfillset(&sigint_struct.sa_mask);
    sigint_struct.sa_flags = 0;

    // Set the action for the sigint signal to the sigint_struct previously defined
    sigaction(SIGINT, &sigint_struct, NULL);


    // --- SIGTSTP ---
    // Set up the handler and structure for the SIGTSTP signal.
    // This signal will be ignored for background and forground processes
    sigtstp_struct.sa_handler = catch_SIGTSTP;
    sigfillset(&sigtstp_struct.sa_mask);
    sigtstp_struct.sa_flags = 0;

    // Set the action for the sigtstp signal to the sigtstp_struct previously defined
    sigaction(SIGTSTP, &sigtstp_struct, NULL);

    
    // --- SIGIGN ---
    // Set up the handler for ignoring signals
    // The shell will ignore SIGHUP and SIGQUIT signals sent to it. Futher, as previously defined,
    // the shell will not quit upon receiving the SIGINT and SIGTSTP signal
    ignore_action.sa_handler = SIG_IGN;
    sigaction(SIGHUP, &ignore_action, NULL);
    sigaction(SIGQUIT, &ignore_action, NULL);

    
    // ---------------
    // Main Shell loop
    // Description: Performs the main operations of the shell and continues to loop (get the command prompt, 
    //      execute programs, etc) until user specifies otherwise using the "exit" command
    // ---------------
    
    while(1)
    {
        // Foreground-only mode is turned on
        if(TSTP_MESSAGE == 1)
        {
            // Because waitpid is non-re-entrant, ensure that we wait for whatever foreground process
            // to finish before displaying the message. Skip if last command was in the background
            if(background_flag == 0)
                waitpid(spawnPid, &STATUS, 0);

            // Reset the message indicator to NULL and print the message
            TSTP_MESSAGE = 0;
            printf("\nEntering foreground-only mode (& is now ignored)\n");
            fflush(stdout);
        }
        // Foreground-only mode is turned off
        else if(TSTP_MESSAGE == 2)
        {
            // Again, because waitpid is non-re-entrant, wait for any foreground process to finish up
            // before displaying the message. Skip if last command was in the background
            if(background_flag == 0)
                waitpid(spawnPid, &STATUS, 0);

            // Reset the message flag and display the message
            TSTP_MESSAGE = 0;
            printf("\nExiting foreground-only mode\n");
            fflush(stdout);
        }

        // User sent a kill command using the keyboard
        if(INT_MESSAGE == 1)
        {
            // Clean up the foreground process that was just terminated
            waitpid(spawnPid, &STATUS, 0);

            // If the terminated program indicated that it was terminated by signal (for some programs,
            // like ping, ctrl-c is the exiting command. So status will be set to 0), display message
            if(WIFSIGNALED(STATUS))
            {
                printf("\nterminated by signal %d\n", WTERMSIG(STATUS));
                fflush(stdout);
            }

            // Reset the message indicator
            INT_MESSAGE = 0;
        }


        
        // ----------
        // Clean finished background processes
        // Description: Attempt to clean up any background processes that have finished
        //      or that were terminated by signal
        // ----------
      
        // Scan the entie background processes array 
        for(i = 0; i < MAX_PS; i++)
        {
            // If there is a valid process ID ...
            if(background_ps[i] != -5)
            {
                // Attempt to clean it up
                pid_t returned = waitpid(background_ps[i], &BACK_STATUS, WNOHANG);
                if(returned != 0)
                {
                    // Terminated by a signal? Display signal termination message
                    if(WIFSIGNALED(BACK_STATUS))
                    {
                        printf("background pid %d is done: terminated by %d\n", background_ps[i], WTERMSIG(BACK_STATUS));
                        fflush(stdout);
                    }
                    // Otherwise, it exited normally. Display the normal exit message
                    else
                    {
                        printf("background pid %d is done: exit value %d\n", background_ps[i], WEXITSTATUS(BACK_STATUS));
                        fflush(stdout);
                    }

                    // Reset that process id to the junk, -5 PID value and decrement the number of current background processes
                    background_ps[i] = -5;
                    numPs--;
                }
            }
        }

        // Populates the argList array with strings and the numArgs with the number
        // of arguments entered (including command). Refer to buffer_io.c for details
        getCommandLine(&numArgs, argList);


        // ----------
        // Input command line parsing:
        // ----------

        // Checks for empy lines and lines that start with the comment mark ( # ). Ignore those lines completely.
        if(argList[0] != 0 && argList[0][0] != '#')
        {
            
            // .......................
            // Exapand the $$ operator
             
            // Loop through each of the entered argument strins
            for(i = 0; i < numArgs; i++)
            {
                // Loop through each of the strings characters. Check to ensure they are valid characters
                j = 0; 
                while(argList[i][j] != 0 && argList[i][j + 1] != 0)
                {
                    // Check if anny two dollar sign symbols appear next to each other in a word
                    if(argList[i][j] == '$' && argList[i][j + 1] == '$')
                    {
                        // Expand out the process id in the string. Refer to utility.c for details on this function
                        expandProcessID(argList, i, j);
                        j++;
                    }

                    // Increment to the next character
                    j++;
                }
            }

            
            // ..............
            // Built in: exit
            
            // Check if user entered exit as first argument on CL
            if(strcmp(argList[0], "exit") == 0)
            {
                // Clean up shell and exit the program. Refer to utility.c for details of this function
                cleanShell(argList, background_ps, numPs);
                exit(0);
            }


            // ............
            // Built in: cd

            // Check if user entered built in cd command as first argument on CL
            else if(strcmp(argList[0], "cd") == 0)
            {
                // Call the built in cd program found in utility.c. Pass the first argument after "cd" (may be NULL)
                builtIn_cd(argList[1]); 
            }


            // ................
            // Built in: status

            // Check if the user entered the built in status command as first argument on CL
            else if(strcmp(argList[0], "status") == 0)
            {
                // Display specific message depending on if the signal was terminated by signal or not
                if(WIFSIGNALED(STATUS))
                {
                    printf("terminated by signal %d\n", WTERMSIG(STATUS));
                    fflush(stdout);
                }
                else
                {
                    printf("exit value %d\n", WEXITSTATUS(STATUS));
                    fflush(stdout);
                }
            }


            // .....................................
            // Non-built in command. Requires exec()
            else
            {
                // Save the current stdout and stdin file descriptors 
                int saved_stdout = dup(1);
                int saved_stdin = dup(0);

               
                // ~~~~~~~~~~~~~~~~~~~~~~~~~~~
                // Start process in background?

                // Reset the background control flag for this current entered command.
                background_flag = 0;

                // Is the last commandl ine argument the background process indicator?
                if(strcmp(argList[numArgs - 1], "&") == 0)
                {
                    // Check to ensure we are not in foreground-only mode. 
                    if(TSTP_FLAG != 1)
                    {
                        background_flag = 1;
                        background_msg = 1;
                    }

                    // Remove the "&" operator from the list of arguments
                    free(argList[numArgs - 1]);
                    argList[numArgs - 1] = 0;

                    // The "&" is removed, decrement the number of arguments
                    numArgs--;
                }


                // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                // Check for the redirection < > operators

                // Loop through the arguments, last to first
                for(i = numArgs - 1; i >= 0; i--)
                {
                    // -----------------------
                    // --- REDIRECT STDOUT ---
                    // -----------------------

                    // If the ">" was found, perform the necessary redirection
                    if(argList[i] != 0 && strcmp(argList[i], ">") == 0)
                    {
                        // attempt to open the provided file name for writing. Per assignment specs,
                        // if the file exists, truncate it away
                        int targetFD = open(argList[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

                        // Check if it failed
                        if(targetFD == -1)
                        {
                            printf("cannot open %s for output\n", argList[i + 1]);
                            fflush(stdout);

                            redirectErrFlag = 1;
                        }
                        else
                        {
                            // Redirect the stdout to the file target
                            int result = dup2(targetFD, 1);

                            // Check if redirection failed and display error message if needed
                            if(result == -1)
                            {
                                perror("dup2");
                                redirectErrFlag = 1;
                            }
                            else
                            {
                                // Set the stdout control flag to ON
                                stdout_flag = 1;

                                // Remove the file redirection argument strings for the argument list
                                free(argList[i]);
                                argList[i] = 0;

                                free(argList[i + 1]);
                                argList[i + 1] = 0;

                                // Since file redirection args are gone, decrement the numArgs counter by 2
                                numArgs -= 2;
                            }
                        }

                        // Close the file descriptor as needed
                        close(targetFD);
                    }


                    // ----------------------
                    // --- REDIRECT STDIN ---
                    // ----------------------

                    // Found the input redirection operator in the argument list
                    else if(argList[i] != 0 && strcmp(argList[i], "<")  == 0)
                    {
                        // Attempt to open the input file for reading
                        int sourceFile = open(argList[i + 1], O_RDONLY);

                        // Check if opening was sucessful
                        if(sourceFile == -1)
                        {
                            printf("cannot open %s for input\n", argList[i + 1]);
                            fflush(stdout);

                            redirectErrFlag = 1;
                        }
                        else
                        {
                            // Otherwise, redirect the stdin to the source file
                            int result = dup2(sourceFile, 0);
                            
                            // Check if redirection successful
                            if(result == -1)
                            {
                                perror("dup2");
                                redirectErrFlag = 1;
                            }
                            else
                            {
                                // Set the standard input control flag
                                stdin_flag = 1;

                                // Otherwise, free up the redirection arguments from the arg list
                                free(argList[i]);
                                argList[i] = 0;

                                free(argList[i + 1]);
                                argList[i + 1] = 0;

                                // Decrement by 2, the two arguments are gone from argument list
                                numArgs -= 2;
                            }
                        }

                        // Close file descriptor as needed
                        close(sourceFile);
                    }

                    // Only perform loop a max of 6 times.
                    // Because this loop is only checking for the &, >, and < arguments,
                    // and because they appear at the end of the argument list, we can stop after 6 iterations.
                    if(numArgs - i >= 6)
                        break;
                }

                
                // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                // Background Process Redirection
                // Description: If a background process is redirecting one of the files (or nether),
                //      this block of code will properly redirect the other, non-user set redirection
                //      to /dev/null in order that the background process dose not interfere with other
                //      foreground processes

                // If the background control flag is set and there was no previous redirection errors,
                // Proceed to set the necessary /dev/null redirections
                if(!redirectErrFlag && background_flag == 1)
                {
                    int result; 

                    // User did not specify stdin or stdout redirection
                    if(stdin_flag == 0 && stdout_flag == 0)
                    {
                        // Redirect both to /dev/null
                        result = redirectToNull();

                        // Set the redirection error flag as needed
                        if(result == -1)
                            redirectErrFlag = 1;
                    } 
                    
                    // User specified a stdin file, but no stdout file for the background process
                    else if(stdin_flag == 1 && stdout_flag == 0)
                    {
                        // Redirect the stdout to /dev/null
                        result = redirectStdout();

                        // Set the redirection error flag as needed
                        if(result == - 1)
                            redirectErrFlag = 1;
                    }

                    // User specified a stdout file, but no stdin file for the background process
                    else if(stdin_flag == 0 && stdout_flag == 1)
                    {
                        // Redirect the stdin to /dev/null
                        result = redirectStdin();

                        // Set the redirection error flag as needed
                        if(result == - 1)
                            redirectErrFlag = 1;
                    }
                }
                    

                // -------------------------
                // *** Processes forking ***
                // -------------------------

                // Check if redirection was successful (badfile name?)
                if(!redirectErrFlag)
                {
                    // Spawn a new fork of the shell and execute the provided command
                    spawnPid = fork();

                    // Perform various tasks in the parent shell and exec() the specified program in the child
                    switch(spawnPid)
                    {
                        // Forking failed!
                        case -1:
                            perror("Forking error!\n");
                            exit(301);
                            break;
                        
                        // -------------
                        // Child Process --> Becomes the new execution of the specified program
                        // -------------
                        case 0:
                            // Set the foreground and background processes to ignore the sigtstp signal
                            sigaction(SIGTSTP, &ignore_action, NULL);

                            // Background processes will ignore the sigint signal. Otherwise, foreground
                            // processes will be interupted by the sigint signal
                            if(background_flag == 0)
                                sigaction(SIGINT, &sigint_struct, NULL);
                            else
                                sigaction(SIGINT, &ignore_action, NULL);

                            // Execute the specified program (this child shell will no longer exist and further
                            // code will not execute in this child, unless there was an error in the execution)
                            execvp(argList[0], argList);

                            // There was an error in execusion: Display error message and exit dramatically. 
                            printf("%s: no such file or directory\n", argList[0]);
                            fflush(stdout);
                            exit(1);
    
                        // --------------
                        // Parent Process --> waits for foreground process and loops back around for another prompt
                        // --------------
                        default:
                            // If the child is a foreground process, then wait for it to complete
                            if(background_flag == 0)
                                waitpid(spawnPid, &STATUS, 0);
                            // Otherwise, set up the child as a background process
                            else
                            {   
                                waitpid(spawnPid, &BACK_STATUS, WNOHANG);

                                // Go through the background processes id array
                                for(i = 0; i < MAX_PS; i++)
                                {
                                    // Place the background process id into the array of PIDs upon first junk PID found
                                    if(background_ps[i] == -5)
                                    {
                                        background_ps[i] = spawnPid;
                                        numPs++;
                                        break;
                                    }

                                    // error logic in case there are now too many background processes
                                    if(i == (MAX_PS - 1))
                                    {
                                        perror("OVERFLOW! Too many background processes running");
                                        exit(501);
                                    }
                                }
                            }

                            // Reset the stdin and stdout to the saved file descriptors for the original
                            // stdin and stdout
                            dup2(saved_stdin, 0);
                            dup2(saved_stdout, 1);
                    }
                }

                // The redirection was unsucessful, so create a fork, but immediatly return
                else
                {
                    // Generate child shell process
                    spawnPid = fork();
                    switch(spawnPid)
                    {
                        // Error with forking the child
                        case -1:
                            perror("Forking error!");
                            exit(1);
                            break;
                        // Child shell - exit immediatly 
                        case 0:
                            exit(1);
                        // Parent - Wait for the child to exit
                        default:
                            waitpid(spawnPid, &STATUS, 0);

                            // Reset the stdin and stdout
                            dup2(saved_stdin, 0);
                            dup2(saved_stdout, 1);
                    }
                }
            }
        }

        if(background_msg == 1)
        {
            printf("background pid is %d\n", spawnPid);
            fflush(stdout);

            background_msg = 0;
        }

        // Clean The input buffer
        cleanBuffer(argList);

        // Reset the redirection error flag
        redirectErrFlag = 0;


        // Reset the redirection control flags
        stdin_flag =  0;
        stdout_flag = 0;

        // Loop back to the top, get another command line from user, profit. 
    }
}
