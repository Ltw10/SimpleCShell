README for CS270 Project 4 Simple Shell Assignment
Luke Williams (Solo)

Files included in submission: csapp.h, csapp.c, simpsh.c, Makefile, and README.txt

For my simple shell I decided to step through each process that the shell would run iteratively. This project took me a while and my code is 
not the cleanest, but it does the job. I started by figuring out how I would read input and separate it into tokens. Once I got that figured
out I implemented some of the easier commands like the comment(#) and the "lv" command. Inside my loop that created my tokens I had to figure out
how to make separate tokens surrounded by " " into one token, replace variables when proceeded by "$", and ignore anything after the "#". 
These were come of the challenges I had. Another challenge I had was figuring out the "!" command and the assignment operator (=). At the 
end of the day my code depends on a lot of local variables, but that is okay.

There are no special features of my shell and the only limitation is that it can only process tokens with 3 or less variable substitutions.
So my shell could not process something like VAR5 = VAR1:VAR2:VAR3:VAR4 if VAR1-4 were all valid variables. I could have made this more but 
I figured that three was good enough. The other limitation is that I only allow variables to be split by the symbols ":" and "/". These were the
only cases you usually see variable values being combined. As an example a statement like PATH = $PATH:$CWD. 

One thing to note is that if the simp shell is ran with "make run" or "make" then the SIGINT handlers will not work as expected because it will
use the default make system settings. So please run the program using "./simpsh" on the command line.

Hope you enjoy my simpshell.
