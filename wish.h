/***********************
 * Author: John McBride
 * Email: mcbridej@oregonstate.edu
 * CS 344 - Operating Systems
 * Date: May 27th 2018
 *
 * Description: The macros used for the lengths of buffers and the various
 *      function headers from buffer_io.c and utility.c
 * **********************/

// Program length macros
#define LINE_SIZE 2048
#define ARG_SIZE  512
#define MAX_PS    256

// Functions found in buffer_io.c
void getCommandLine(int *inNum, char **argList);
void cleanBuffer(char **argList);

// Functions found in utility.c
void builtIn_cd(char *path);
void cleanShell(char **argList, pid_t *background_ps, int numPs);
void expandProcessID(char **argList, int argString, int argChar);

int redirectToNull();
int redirectStdin();
int redirectStdout();
