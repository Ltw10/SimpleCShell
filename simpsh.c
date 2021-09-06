/*
    CS 270 Project 4
    Luke Williams
    Simple Shell Program - implementation of a simple shell that reads commands typed by the user
    and executes them. It also allows users to set variables to be used in subsequent commands.
*/

#include "csapp.h"

#define VAR_TABLE_SIZE 100 //starting size of variable table - more space is allocated if necessary
#define MAX_INPUT_LINE 256 //given parameters that max input is 256 chars
#define MAX_TOKENS 128 //max number of tokens for an input line of 256 chars is 128 
#define MAX_VARS_IN_TOKEN 3 //max number of variables allowed by shell in one token
#define MAX_DIRECS_IN_PATH 10 //max number of directories stored in path

volatile int insideChild = 0; //used in signal handler for indicating if signal was recieved within child or within parent

/*
    dieWithError function used if shell system call fails
*/
void dieWithError(char* message){
    printf("%s\n",message);
    exit(1);
}

/*
    signal handler for SIGINT. Does not kill simpsh but outputs different messages for
    different situations
*/
void sigint_handler(int sig)
{
    if(insideChild == 0) //signal recieved in child
        write(1, "\nSIGINT signal recieved. To terminate program enter \"quit\" or ctrl-D\nPress enter to continue.", 93);
    else //signal recieved in parent
        write(1, "\nSIGINT signal recieved. Closing process and returning to simpsh.\n",66);
}

/*
    variable structure is used in varTable array to hold shell variables
    name is the name of the variables and value is the value that that variable holds
*/
struct variable{
    char* name;
    char* value;
};

int main(){

    //establishes SIGINT signal handler
    struct sigaction sigintAction, oldSigIntAction; 
    sigintAction.sa_handler = sigint_handler;
    sigintAction.sa_flags = SA_RESTART;
    if(sigaction(SIGINT, &sigintAction, &oldSigIntAction) < 0)
        dieWithError("System Error: sigaction error when settingn up sigint handler.\n");

    int varCount = 0; //counter for variables stored in program
    char* delim = " \t\n\r"; //these charecters seperate tokens in the input line

    //allocates space for 100 variables and values to be stored in varTable
    struct variable* varTable = malloc(VAR_TABLE_SIZE * sizeof(struct variable)); 
    struct variable newVar; //temporarily holds the variable that will be inserted into varTable

    newVar.name = "PATH"; //PATH variable name
    newVar.value = "/bin:/usr/bin"; //PATH variable value
    varTable[varCount] = newVar; //insert PATH variable in varTable
    varCount++;

    char CWD[PATH_MAX];
    getcwd(CWD, sizeof(CWD)); //inserts current working directory into variable string CWD
    if(CWD == NULL) //getcwd function fails
    {
        dieWithError("System Error: getwd function returned with error.\n"); //system error
    }
    newVar.name = "CWD"; //CWD variable name
    newVar.value = &CWD[0]; //CWD variable value
    varTable[varCount] = newVar; //insert CWD variable in varTable
    varCount++;

    newVar.name = "PS"; //PS variable name
    newVar.value = "simpsh>"; //PS variable value
    varTable[varCount] = newVar; //insert PS variable in varTable
    varCount++;

    char inbuf[MAX_INPUT_LINE]; //buffer to store shell input
    char* tokens[MAX_TOKENS]; //array of strings to hold input tokens
    char* token; //holds temporary value of most recently read in token
    
    for(;;)
    {
        printf("%s:%s",varTable[1].value,varTable[2].value); //command line

        char inChar; //temporary char to hold input
        int iterChar = 0; //iterator to count number of chars
        while((inChar = fgetc(stdin)) != '\n') //reads in input one charecter at a time
        {
            if(inChar == EOF) //format for ctrl d input
            {
                printf("\n");
                exit(0); //exits simpsh
            }
            if(iterChar != MAX_INPUT_LINE) //while input length is not max
            {
                inbuf[iterChar] = inChar; //add char to input
                iterChar++; //increase iterator
            }
            else //if input line exceeds max
            {
                break; //cut off the rest of the input - it is not accepted
            }
        }

        int tokenNum = 0; //iterator for token array 
        int tokenStartingQuotes = -1, tokenEndingQuotes = -1; //used find starting and ending quotes and turn everything between into one token
        int varSyntaxError = 0; //variable is set to 1 if there is a variable syntax error such as $ followed by an invalid variable name
        int commentSyntaxError = 0; //variable is set to 1 if there is a comment syntax error such as a "#" not in the first token
        int ignore = 0; //variable used for signaling that the input is a comment
        char* input = strndup(inbuf, iterChar); //duplicated the buffer to a new string called input
        while((token = strsep(&input,delim)) != NULL) //seperates the input into tokens
        {
            //makes sure no tokens are read in as blank - this occurs when there are repeating spaces in the input
            if(strcmp(token,"") != 0)
            {
                if((*token == '#') && (tokenNum == 0)) //token signifiest a comment
                {
                    ignore = 1;
                }
                if((*token == '#') && (tokenNum != 0))
                {
                    commentSyntaxError = 1;
                }
                if((*token == '$') && (ignore == 0)) //signifies that there is a variable to be replaced within the token
                {
                    //all of the work down below is for finding the variables that need replaced within the token (multivariable tokens)
                    char* interiorVar = malloc(strlen(token)); //temporary value to hold token within token
                    int varNums[MAX_VARS_IN_TOKEN] = {-1, -1, -1}; //holds the corresponding index in varTable for the variable replacement
                    int interiorVarLengths[MAX_VARS_IN_TOKEN] = {0}; //holds the length of the interior tokens
                    int varNum = 0; //iterator for the arrays
                    char* fakeToken = strdup(token); //fakeToken is used for strsep so we still have the original token untouched
                    while((interiorVar = strsep(&fakeToken, ":/")) != NULL) //seperates the token into smaller tokens delimited by either ":" or "/"
                    {
                        varSyntaxError = 0; //no syntax error if there is no var to replace
                        interiorVarLengths[varNum] = strlen(interiorVar); //stores length of current token
                        if(*interiorVar == '$') //if token is a variable that needs replaced
                        {
                            varSyntaxError = 1; //set to 0 if there is a matching variable stored
                            memmove(interiorVar,interiorVar+1, strlen(interiorVar)); //get rid of dollar sign
                            for(int j = 0; j < varCount; j++) //loops through varTable
                            {
                                if(strcmp(interiorVar, varTable[j].name) == 0) //comapares token to all variable names in varTable
                                {
                                    varSyntaxError = 0; //finds a match so sets to 0
                                    varNums[varNum] = j; //saves index of matching variable in varTable
                                    varNum++;
                                }   
                            }
                            if(varSyntaxError == 1) //if varSyntaxError is still 1 after the loop then we have a syntax error because there is not matching variable
                                break; //break the while loop
                        }
                        else //token is not a variable replacement
                        {
                            varNums[varNum] = -2; //store all non variables in the arrays as -2
                            varNum++;
                        }
                    }
                    char* newToken = malloc(MAX_TOKENS); //malloc a newToken with MAX_TOKENS size
                    int tokenCharIndex = 0; //keeps track of where we are in the original token
                    for(int i = 0; i < varNum; i++) //loops through varNums and interiorVarLengths
                    {
                        if(varNums[i] != -2) //if this part of the token needs variable replacement
                        {
                            strcat(newToken, varTable[varNums[i]].value); //insert variable value into newToken
                            if(varNums[i+1] != -1) //if there is another part to the token
                            {
                                strncat(newToken, &token[tokenCharIndex+interiorVarLengths[i]], 1); //add the delimiter back in
                                tokenCharIndex++;
                            }
                        }
                        else //no need for variable replacement
                        {
                            strncat(newToken, &token[tokenCharIndex], interiorVarLengths[i]); //keep the original part of the token
                        }
                        tokenCharIndex += interiorVarLengths[i]; //update variable to keep track of where we are inside token
                    }
                    strcpy(token, newToken); //replaces token with newToken
                } //end of variable replacement
                if((*token == '"') && (ignore == 0)) //if we see front quotes
                {
                    memmove(token, token+1, strlen(token)); //remove the front quotes from this token
                    tokenStartingQuotes = tokenNum; //marks what token had the front quotes
                }
                if((*(token+(strlen(token)-1)) == '"') && (ignore == 0)) //if we see back quotes
                {
                    token[strlen(token)-1] = '\0'; //removes the back quotes
                    tokenEndingQuotes = tokenNum; //marks where the back quotes from this token
                }
                tokens[tokenNum] = token; //insert the token into the array
                tokenNum++;
                if((tokenStartingQuotes != -1) && (tokenEndingQuotes != -1)) //if we have found a complete set of quotes
                {
                    for(int i = 1; i <= (tokenEndingQuotes - tokenStartingQuotes); i++) //loop through tokens that were inside quotes
                    {
                        strcat(tokens[tokenStartingQuotes], " "); //add a space behind the token
                        strncat(tokens[tokenStartingQuotes],tokens[tokenStartingQuotes+i], strlen(tokens[tokenStartingQuotes+i])); //add the next token to the token
                    }
                    tokenNum = tokenStartingQuotes + 1; //new tokens now go after the quote tokens
                    //reset quote token values
                    tokenStartingQuotes = -1;
                    tokenEndingQuotes = -1;
                } //end of quote pair
            } //end of valid token
        } //end of token reading

        for(int i = tokenNum; i < MAX_TOKENS; i++) //loops through array after all valid tokens
        {
            tokens[i] = NULL; //replaces these values with NULL
        }
    
        if(varSyntaxError == 1) //if we have a variable syntax error
        {
            printf("Syntax Error: Variable name not found.\n"); //print sytnax error
        }
        else if(commentSyntaxError == 1) //if we have a comment syntax error
        {
            printf("Syntax Error: \"#\" not allowed in input if not specifying a comment.\n");
        }
        else if(tokens[0] != NULL) //else if there is a token
        {
            if((*tokens[0] == '#') && (strlen(tokens[0]) == 1)) //comment so we do nothing
            {
                //comment is taken and simpsh does nothing
            } // # command

            else if((*tokens[0] == '!') && (strlen(tokens[0]) == 1)) //fork and run given program if it is legal
            {
                char* envir[VAR_TABLE_SIZE] = {NULL}; //creates array of strings to store environment variables
                for(int i = 0; i < varCount; i++) //loops through varTable
                {
                    envir[i] = malloc(strlen(varTable[i].name) + strlen(varTable[i].value) + 1); //malloc appropriate space
                    //turns envir[i] into variable name=value
                    strcat(envir[i], varTable[i].name);
                    strcat(envir[i], "=");
                    strcat(envir[i], varTable[i].value);
                }
                envir[varCount] = NULL; //NULL terminates the environment array
                int i = 2; //iterator for tokens. Starts at 2 because 0 is "!" and 1 is cmd
                int argCount = 1; //iterator for argv. Starts at 1 because argv[0] is cmd
                int cmdFound = -1; //signals whether cmd is valid
                int status; //holds status of child process
                struct stat stat1; //used within the stat() function
                char* cmd; //holds cmd
                char* directory; //temporary value of directory
                char* argv[VAR_TABLE_SIZE] = {NULL}; //holds program arguments
                char* inputRedir = NULL; //holds file name for input redirection
                char* outputRedir = NULL; //holds file name for output redirection
                int inputRedir_fd, outputRedir_fd; //files descriptors for input and output redirection
                while(tokens[i] != NULL) //loops through all valid tokens starting at 2
                {
                    if(strcmp(tokens[i], "infrom:") == 0) //signifies input redirection
                    {
                        if(tokens[i+1] != NULL) //given name for file
                        {
                            inputRedir = tokens[i+1]; //file for input redirection
                            i+=2;
                        }
                        else //error
                            printf("Syntax Error: Specified input redirection with no given filename.\n");
                    }
                    else if(strcmp(tokens[i], "outto:") == 0) //signifies output redirection
                    {
                        if(tokens[i+1] != NULL) //given name for file
                        {
                            outputRedir = tokens[i+1]; //file for output redirection
                            i+=2;
                        }
                        else //error
                            printf("Error: Specified output redirection with no given filename.\n");
                    }
                    else //command line argument
                    {
                        argv[argCount] = tokens[i]; //adds token to argv
                        argCount++;
                        i++;
                    }
                }
                argv[argCount] = NULL; //NULL terminates argv
                char* directories[MAX_DIRECS_IN_PATH]; //stories valid directory paths for cmd
                int numDirectories = 0; //iterator for directories array
                if(*tokens[1] == '/') //absolute path
                {
                    directories[numDirectories] = tokens[1]; //add absolute path to directories
                    numDirectories++;
                }
                else if((*tokens[1] == '.') && (*(tokens[1]+1) == '/')) //current directory
                {
                    memmove(tokens[1], tokens[1]+1, strlen(tokens[1])); //remove the "."
                    memmove(tokens[1], tokens[1]+1, strlen(tokens[1])); //remove the "/"
                    cmd = tokens[1]; //save cmd
                    directory = strndup(CWD, strlen(CWD)); //set directory to CWD
                    strcat(directory, "/"); //add a / to directory
                    strcat(directory, cmd); //add cmd to directory
                    directories[numDirectories] = directory; //add path to directories array
                    numDirectories++;
                }
                else //use PATH variable
                {
                    char* pathConvert = strdup(varTable[0].value); //saves PATH variable value as new variable
                    cmd = tokens[1]; //saves cmd
                    while((directory = strsep(&pathConvert, ": \n\t\r")) != NULL) //seperates path value into tokens
                    {
                        directories[numDirectories] = malloc(strlen(directory) + strlen(cmd) + 1); //malloc space for path
                        strcat(directories[numDirectories], directory); //adds directory to directories array
                        strcat(directories[numDirectories], "/"); //add a / to directory
                        strcat(directories[numDirectories], cmd); //add cmd to directory
                        numDirectories++;
                    }
                }

                for(int i = 0; i < numDirectories; i++) //loop through directories array
                {
                    if((stat(directories[i], &stat1)) == 0) //uses stat to see if element is a valid cmd and path
                        cmdFound = i; //marks location of valid cmd
                }

                if(cmdFound != -1) //valid cmd
                {
                    argv[0] = directories[cmdFound]; //sets argv[0] to path and cmd
                    insideChild = 1;
                    if(fork() == 0) //forks child process
                    {
                        if(inputRedir != NULL) //if input redirection needs to occur
                        {
                            inputRedir_fd = open(inputRedir, O_RDONLY); //opens correct file
                            dup2(inputRedir_fd, 0); //duplicates stdin and file descriptor
                            close(inputRedir_fd); //closes file
                        }
                        if(outputRedir != NULL) //if output redirection needs to occur
                        {
                            outputRedir_fd = open(outputRedir, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU); //opens correct file
                            dup2(outputRedir_fd, 1); //duplicates stdout and file descriptor
                            close(outputRedir_fd); //closes file
                        }
                        if(execve(directories[cmdFound], argv, envir) == -1) //child process runs cmd
                        {
                            printf("System Error: Execve error when running cmd %s.\n",cmd);
                            dieWithError(""); //execve error
                        }
                    }
                    else //parent process
                    {
                        if(waitpid(0, &status, 0) < 0) //parent waits to reap child
                            dieWithError("System Error: Child process was interrupted and was not reaped.\n"); //waitpid error
                        insideChild = 0;
                    }
                }
                else //error for invalid command
                {
                    printf("Error: cmd can not be found in any directories listed in PATH\n");
                }
            } // ! command
            
            else if((strcmp(tokens[0],"cd") == 0) && (strlen(tokens[0]) == 2) && (tokens[2] == NULL)) //change directory command
            {
                if(tokens[1] == NULL) //error: no parameters
                {
                    printf("Syntax Error: Simpsh not built to deal with cd command without parameters.\n");
                }
                else if (strcmp(tokens[1], "..") == 0) //go to parent directory or go back one directory
                {
                    char* newDir = NULL; //string to hold new directory name
                    int chop; //holds index of where to chop off CWD
                    for(int i = 0; i < strlen(CWD); i++) //loops through CWD
                    {
                        if(CWD[i] == '/') //finds last occurence of /
                            chop = i;
                    }
                    if(chop == 0) //if we have gone all the way to the root just keep the /
                    {
                        newDir = malloc(1);
                        chop = 1;
                    }
                    else //newDir has size of chop
                    {
                        newDir = malloc(chop);
                    }                    
                    strncpy(newDir, CWD, chop); //copies directory into newDir
                    strcpy(CWD, newDir); //copies newDir value into CWD
                    varTable[1].value = newDir; //sets new CWD value in varTable
                    chdir(CWD); //changes directory to newDir
                }
                else if(*tokens[1] == '/') //absolute path
                {
                    if(chdir(tokens[1]) == 0) //absolute path is a valid directory
                    {
                        strcpy(CWD, tokens[1]); //change CWD to new directory value
                        varTable[1].value = tokens[1]; //sets new CWD value in varTable
                    }
                    else //absolute path is not valid
                        printf("Error: Directory not found for absolute pathname in cd function\n");
                }
                else //looks for the directory within the CWD
                {
                    char* newDir = malloc(strlen(CWD) + strlen(tokens[1]) + 1); //malloc newDir
                    strcpy(newDir, CWD); //copy CWD to newDir
                    strcat(newDir, "/"); //add a /
                    strcat(newDir, tokens[1]); //add the token name to newDir
                    if(chdir(newDir) == 0) //valid directory
                    {
                        strcpy(CWD, newDir); //change value of CWD
                        varTable[1].value = newDir; //change value of CWD in varTable
                    }
                    else //invalid directory
                        printf("Error: Directory not found within current directory for cd function\n");
                }
            }
            else if((strcmp(tokens[0],"lv") == 0) && (strlen(tokens[0]) == 2) && (tokens[1] == NULL)) //lv command (list variables)
            {
                for(int i = 0; i < varCount; i++) //loops through varTable
                {
                    if(*varTable[i].name != '\0') //unset variables are changed to NULL so have to skip those array indicies
                        printf("%s = %s\n", varTable[i].name, varTable[i].value); //prints variable names and values
                }
            }
            else if((strcmp(tokens[0],"unset") == 0) && (strlen(tokens[0]) == 5) && tokens[2] == NULL) //unset variable command
            {
                int foundVar = 0; //signifies if variable name is a valid variable
                for(int i = 0; i < varCount; i++) //loops through varTable
                {
                    if(strcmp(tokens[1],varTable[i].name) == 0 && i <= 2) //checks if user is trying to unset any built-in variables
                    {
                        printf("Error: Can not change any built-in variable using unset\n");
                        foundVar = 1;
                        break; //break loop
                    }
                    if(strcmp(tokens[1], varTable[i].name) == 0) //compares variable token to varTable variable names
                    {
                        //changes the array element to NULL
                        *varTable[i].name = '\0';
                        *varTable[i].value = '\0';
                        foundVar = 1;
                    }
                }
                if(foundVar == 0) //error invalid variable name
                {
                    printf("Error: Variable name \"%s\" not a saved variable\n", tokens[1]);  
                }
            }
            else if((strcmp(tokens[0],"quit") == 0) && (strlen(tokens[0]) == 4)) //quit command
            {
                exit(0); //exit normally
            }
            else if(tokens[1] != NULL) //looking for valid commands in second token
            {
                if((*tokens[1] == '=') && (strlen(tokens[1]) == 1)) //assignment operator
                {
                    int syntaxError = 0;
                    if(tokens[3] != NULL)
                    {
                        syntaxError = 1;
                        printf("Syntax Error: Assignment operator must follow the pattern name = value. Input contained too many tokens.\n");
                    }
                    if(syntaxError == 0)
                    {
                        int valid, preexisiting = 0; //stores values for valid and preexisiting variable names
                        char* varName = tokens[0]; //variable name token
                        for(int i = 0; i < varCount; i++) //loops through varTable
                        {
                            if(strcmp(varTable[i].name, varName) == 0) //compares token variable name to varTable variables names
                            {
                                preexisiting = 1;
                                if(i == 1) //trying to change CWD using assignment
                                {
                                    printf("Error: Can not change built-in variables CWD and PS using assignment operator\n");
                                }
                                else //changes exisiting variable value in varTable
                                {
                                    varTable[i].value = tokens[2];
                                }
                            }
                        }
                        if(preexisiting == 0) //not a preexisiting variable
                        {
                            //checks if the first charecter of the variable name is an upper or lower case letter
                            if(((*varName >= 65) && (*varName <= 90)) || ((*varName >= 97) && (*varName <= 122)))
                            {
                                valid = 1;
                            }
                            else //not valid variable name
                            {
                                valid = 0;
                                printf("Error: Invalid variable name for assign command. Variable name must start with letters only.\n");
                            }
                            if(valid == 1) //valid variable name so far
                            {
                                int length = strlen(varName);
                                for(int i = 0; i < length; i++) //loops through varName
                                {
                                    //checks to make sure variable name consists of letters and numbers only
                                    if(((*(varName + i) >= 48) && (*(varName + i) <= 57)) || ((*(varName + i) >= 65) && (*(varName + i) <= 90)) || ((*(varName + i) >= 97) && (*(varName + i) <= 122)))
                                    {
                                        //valid stays at 1
                                    }
                                    else //invalid
                                    {
                                        valid = 0;
                                    }
                                }
                                if(valid == 1) //still a valid variable name
                                {
                                    if(varCount == VAR_TABLE_SIZE) //if we ran out of room in our varTable
                                    {
                                        varTable = (struct variable*)realloc(varTable, VAR_TABLE_SIZE * sizeof(struct variable) * 2); //realloc with double the space
                                    }
                                    //set new variable in varTable
                                    varTable[varCount].name = tokens[0];
                                    varTable[varCount].value = tokens[2];
                                    varCount++;
                                }
                                else //invalid variable name
                                {
                                    printf("Error: Invalid variable name for assign command. Variable name must contain only alphanumeric charecters.\n"); 
                                }
                            }
                        }
                    }
                } //assignment operator
                else //not a valid command
                {
                    printf("Error: Invalid command. Please try again\n");
                }
            }
            else //not  valid command
            {
                printf("Error: Invalid command. Please try again\n");
            }
            for(int i = 0; i < MAX_INPUT; i++) //loops through input buffer and tokens array
            {
                if(i < MAX_TOKENS)
                    tokens[i] = NULL; //clears out tokens array
                inbuf[i] = '\0'; //clears out input buffer array
            }
        }
        else //no command given
        {
            printf("Error: No command given. Please try again\n");
        }
    }
    //never reached
}