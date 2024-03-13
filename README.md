## UWM CS537 project 3, WSH (Wisconsin Shell).
A simple unix shell that supports batch mode (reading input from a file) and interactive mode (typing in command lines) implemented in wsh.c
Interative mode could be run using "./wsh" and you should see<br />

  $ prompt> ./wsh<br />
  $ wsh> <br />

Batch mode will read from a batch file and no prompt will be printed, here is an example using script.wsh as input

  $ prompt> ./wsh script.wsh<br />
  $ <br />

Note that no prompts are being printed in the second line

## Pipes
Implemented using fork, exit, wait, execvp, and other system calls. Used dup2 to redirect child input.


## Environment variable and shell variable
Environment variables are declared using "environ" keyword. "getenv" and "setenv" are used to view and change the environment of the current process. <br />
Shell variables are declared using "local" keyword. "vars" prints the shell/local variables in the order they were last edited (most recent goes last).<br />
We also support variable substitution, $ followed by a variable name will be substituted in the prompt. Note that recursive variable definitions are not supported in the form "local somevar = $someothervar"<br />

## History
The wsh.c automatically stores the 5 most recent commands. You can configure the length of history using "history set <n>". Calling "history <n>" runs the nth most recent command again with the same parameter. Calling "history <n>" doesn't updates history list.<br />

## List of built-in functions
1. exit
2. cd
3. export
4. local
5. vars
6. history
