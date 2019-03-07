//---------------------------------------------------
// Author: Justin Luttrell			    |
// Date: 3/28/2018				    |
// File: msh.cpp contains the implementation for    |
//       the shell project                          |
//---------------------------------------------------


#include<iostream> 
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<signal.h>
#include<string>
#include<vector>
#include<fstream>
#define MAX_VAR_LENGTH 256
#define SETVAR_ARGS 3
#define SETPROMPT_ARGS 2
#define SETDIR_ARGS 2
#define DONE_ARGS 2

using namespace std;

void getTokens(string input, vector<string> & tokens, vector<string> variables, vector<string> values);

void setvar(vector<string> tokens, vector<string> & variables, vector<string> & values);

void setprompt(vector<string> tokens, string & prompt);

void setdir(vector<string> tokens); 

void showprocs(vector<int> procInfo, vector<string> procName);

void checkProcs(vector<string> tokens, vector<int> & procInfo, vector<string> & procName);

void done(vector<string> tokens);

void runFly(vector<string> tokens, vector<int> & procInfo, vector<string> & procName, vector<string> values);

void tovar(vector<string> tokens, vector<string> & variables, vector<string> & values);

int main(){


	// VARIABLES AND VALUES	
	// -------------------------------------
	// stores the variables and their values
	vector<string> variables;
	vector<string> values;

	// By default: PATH has value 
	// /bin:/usr/bin
	variables.push_back("PATH");
	values.push_back("/bin:/usr/bin");

	// By default: ShowTokens has
	// value 0.
	variables.push_back("ShowTokens");
	values.push_back("0");
	//--------------------------------------


	// stores the information of background
	// processes
	vector<int> procInfo;
	vector<string> procName;
	//vector<string> showprocsList;


	// default prompt
	string prompt = "msh > ";


	// prints prompt and gets
	// user input
	string input = "";
	cout << prompt;
	getline(cin, input);


	// runs shell until user inputs <ctl-D> or
	// exits elsewhere through done
	while (1){

		// exits program for <CTRL-D>
		if (cin.eof())
			exit(0);				


		// defines a vector to store tokens and
		// calls function to tokenize input
		vector<string> tokens;
		getTokens(input, tokens, variables, values);


		// calls to function to update all background
		// processes
		checkProcs(tokens, procInfo, procName);

		// sets the first word the user inputs
		// to be command
		string command = "";
		if (tokens.size() > 0){
			command  = tokens[0];
		}


		// calls setvar built-in command
		if (command == "setvar")
			setvar(tokens, variables, values);

		// calls setprompt built-in command
		else if (command == "setprompt")
			setprompt(tokens, prompt);

		// calls setdir built-in command
		else if (command == "setdir")
			setdir(tokens);		

		// calls showprocs built-in command
		else if (command == "showprocs")
			showprocs(procInfo, procName);

		// calls done built-in command
		else if (command == "done")
			done(tokens);

		// calls run program-control command
		else if (command == "run")
			runFly(tokens, procInfo, procName, values);

		// calls fly program-control command
		else if (command == "fly")
			runFly(tokens, procInfo, procName, values);

		// calls tovar program-control command
		else if (command == "tovar") 
			tovar(tokens,variables,values);		

		// if the user inputs nothing or spaces it does nothing
		else if (command == "");
	
		// prints error message for invalid command
		// except for empty string
		else{
			cerr << "invalid command: " << command << endl;
		}


		// reprints prompt and asks for user input again		
		cout << prompt;
		getline(cin, input);	

	}

	exit(0);
}


// GET TOKENS
//---------------------------------------------------------------------------------------------------
// Function to tokenize user input
//      -Performs expanion as needed
//	-Prints ShowTokens as necessary
void getTokens(string input, vector<string> & tokens, vector<string> variables, vector<string> values){
	

	// SCANNER AND PARSER
	//-------------------------------------------------------------
	// used to append each
	// leter of an indivual token
	string aToken = "";

	for (int i = 0; i < input.size(); i++){

		// tokenizes a whole set of 
		// parenthesis as one token
		if (input[i] == '\"'){
			i++;

			// appends to aToken until
			// it finds the end quote
			while (input[i] != '\"'){
				aToken += input[i];
				i++;
			}

			tokens.push_back(aToken);
			aToken = "";

		}
		
		
		// determines if there is a comment, if so it doesn't
		// run the for loop anymore, effectively ignoring the rest
		else if (input[i] == '#'){
			i = input.size();
		}


		// when the character is a space, it continues on to the next
		// character and appends the letters to aToken to get the 
		// whole word
		else{

			if(input[i] != ' '){ 
				while (input[i] != ' ' && i < input.size()){
					aToken += input[i];
					i++;
				}

				tokens.push_back(aToken);
				aToken = "";
			}

		}

	}
	//-------------------------------------------------------------------------

	// SHOWTOKENS
	//----------------------------------------------------
	// determines if ShowTokens is set to on
	bool showTokensON;
	for (int i = 0; i < variables.size(); i++){
		if (variables[i] == "ShowTokens"){
			if (values[i] == "1"){
				showTokensON = true;
			}

			else
				showTokensON = false;

		}

	}

	// prints the tokens if ShowTokens is on
	if (showTokensON == true){
		for (int i = 0; i < tokens.size(); i++){
			cout << "Token = " << tokens[i] << endl;
		}
	}
	//------------------------------------------------------



	// EXPANSION
	//---------------------------------------------------------------------------------------------
	//finds the number of carrots in the input
	int numCarrot = 0;
	for (int x = 0; x < tokens.size(); x++)
		for (int y = 0; y < tokens[x].size(); y++){
			if (tokens[x][y] == '^')
				numCarrot++;
		}


	bool expansion = false;

	// Determines if a token contains '^', if so
	// the variable name in the token is replaced with the 
	// value of the variable. Does this for the amount of
	// carrots found above.
	int carrotIndex = -1;
	string varName = "";
	for (int g = 0; g < numCarrot; g++){

		for (int i = 0; i < tokens.size(); i++){

			// finds the index of the "^"
			carrotIndex = tokens[i].find('^');

			// determines if a carrot is found
			if (carrotIndex != -1){

				// j is used to find the end 
				// of the variable name 
				int j = 1;	

				// appends a string for the variable name to check later
				while(tokens[i][carrotIndex + j] != ' ' && tokens[i][carrotIndex + j] != '\"' && (carrotIndex + j) < tokens[i].size()){

					varName += tokens[i][carrotIndex + j];
					j++;
				}

				
				// searches to see if the inputted variable matches
				// an already defined variablee
				for (int k = 0; k < variables.size(); k++){

					if (variables[k] == varName){
						
						// sets expansion to true since expansion will
						// take place
						expansion = true;

						// gets all characters before the '^'
						string expansion = tokens[i].substr(0, carrotIndex);

						// adds to the expansion the variable value
						expansion += values[k];

						
						// j is now the index of last letter of variable name
						j += carrotIndex;
						string stringEnd = "";
		
						// gets the characters after the variable name
						while (j != tokens[i].size()){

							stringEnd += tokens[i][j];
							j++;
						}	
						
						// expansion works by adding:
						// (characters before '^') + (variable value) + (characters after last character of variable name)
						expansion += stringEnd;
						tokens[i] = expansion;


					}				
				}
			}	
			varName = "";	
		}
	}
	// prints message for giving an undeclared variable
	if (carrotIndex != -1 && expansion == false)
		cout << "No such shell variable: " << varName << endl;


	// prints the expansion for showTokens 
	// if expansion occured
	if (showTokensON == true && expansion == true){
		cout << "After expansion:"  << endl;
		for (int i = 0; i < tokens.size(); i++){
			cout << "Token = " << tokens[i] << endl;;
		}

	}	
	//------------------------------------------------------------------------------------------------------------------------------------------

}


// SET VARIABLES
// ------------------------------------------------------------------------------------
// Function to set the variables given the setvar command
void setvar(vector<string> tokens, vector<string> & variables, vector<string> & values){


	// prints error message 
	// checks if the token[0] is setvar in case of
	// calling it in tovar later
	if(tokens.size() != SETVAR_ARGS && tokens[0] == "setvar"){
		cerr << "expected " << SETVAR_ARGS << " tokens, got " 
			<< tokens.size() <<" tokens." << endl;
	}


	else{

		// Used to tell if a variable
		// is already defined
		bool varPresent = false;

		for (int i = 0; i < variables.size(); i++){

			// determines if the variable is
			// is already defined. If so it
			// changes the value
			if (tokens[1] == variables[i]){
				varPresent = true;
				values[i] = tokens[2];
			}

		}	

		// determines if the variable isn't
		// already defined. If not it defines
		// the new variable	
		if (varPresent == false){
			variables.push_back(tokens[1]);
			values.push_back(tokens[2]);
		}	
	}
}


// SET PROMPT
//-----------------------------------------------------
// Function to allow user to change the prompt
void setprompt(vector<string> tokens, string & prompt){
	
	// Determines if user entered the wrong amount of arguments
	if(tokens.size() != SETPROMPT_ARGS){
		cerr << "expected " << SETPROMPT_ARGS << " tokens, got " 
			<< tokens.size() <<" tokens." << endl;
	}
	
	// sets prompt
	else{

		prompt = tokens[1];
	}


}

// SET DIRECTORY
//---------------------------------
// Function to change the directory
void setdir(vector<string> tokens){

	// prints error message for wrong amount of arguments
	if(tokens.size() != SETDIR_ARGS){
		cerr << "expected " << SETDIR_ARGS << " tokens, got " 
			<< tokens.size() <<" tokens." << endl;
	}


	else{

		// gets the current working directory
		char buffer[MAX_VAR_LENGTH];
		string path = getcwd(buffer, MAX_VAR_LENGTH);
		string currentDir;
		currentDir = path;
		int test;

		
		string desiredDir = tokens[1];
		
		// determines if the input is absolute
		if (desiredDir[0] == '/')
			test = chdir(desiredDir.c_str());


		// works for inputs that start with "./" or no "./" 
		// but not with inputs that start with /
		else if (desiredDir[0] == '.' && desiredDir[1] == '/'){

			desiredDir = currentDir + desiredDir.substr(1, desiredDir.size() - 1); 
			test = chdir(desiredDir.c_str());

		}


		// changes the directory for inputs that are relative
		else if (desiredDir[0] != '.' || desiredDir[0] != '/'){
			string slashDir = "/";
			slashDir += desiredDir;
			desiredDir = currentDir + slashDir;
			test = chdir(desiredDir.c_str());

		}
	
		// moves the directory up a level
		else if (tokens[1] == ".."){
			for (int i = currentDir.size()-1; currentDir[i] != '/'; i--){
				if ( currentDir[i] == '/')
					currentDir = currentDir.substr(0, i - 1);
			}

		}

		// determines if an invalid directory was given
		if (test < 0){

			cerr << tokens[1] << ": No such file or directory" << endl;	

		}

		//char buffer2[MAX_VAR_LENGTH];
		//char * path2 = getcwd(buffer2, MAX_VAR_LENGTH);
		//currentDir = path2;



	}
}


// SHOW PROCESSES
//-----------------------------------------------------------
// Function to show the current background processes
void showprocs(vector<int> procInfo, vector<string> procName){

	// prints the list of background directories if 
	// if there are any
	if(procName.size() > 0){	

		cout << "Background processes: " << endl;	

		for(int i = 0; i < procName.size(); i++){
			cout << "\t" << i + 1 << ":   " << procName[i] << endl;

		}
	}

	else
		cout << "No background processes." << endl;


}



// CHECK PROCESSES
//---------------------------------------------------------------------------------------
// Function to check the status of all background process just after each user input
void checkProcs(vector<string> tokens, vector<int> & procInfo, vector<string> & procName){

	if(procName.size() > 0){
		for(int i = 0; i < procName.size(); i++){

			int status;
			int result;

			// gets updated status of child process
			result  = waitpid(procInfo[i], &status, WNOHANG);

			// checks the child status to see
			// if the process has completed
			if(result != 0 && result != -1){
				
				cout << "Completed: " << procName[i] << endl;
				
				// remembers index of completed process for removal
				int removeIndex = i;
			
				// if there is only one background process
				// it just removes them from the vector
				if (procName.size() == 1){

					procName.pop_back();
					procInfo.pop_back();
				}


				else{

					// removes the completed process from the vector if there is
					// more than one vackgroudn process
					for (int j = removeIndex; j < procName.size() - 1; j++){
						procName[i] = procName[i+1];
						procInfo[i] = procInfo[i+1];
					}

					procName.pop_back();
					procInfo.pop_back();
				}
			}
		}
	}
}


// DONE
//-------------------------------------------------------------
// Function to exit program with desired exit code
void done(vector<string> tokens){

	// determines if there are too many arguments
	if (tokens.size() > DONE_ARGS){
		cerr << "too many parameters to done" << endl;
	}

	
	else{

		int exit_status = 0;

		// if the parameter is absent
		// exits program with 0
		if (tokens.size() == 1)
			exit(exit_status);


		// if parameter is present, it converts
		// string element to int	
		exit_status = atoi(tokens.at(1).c_str());		
	
		if (exit_status < 0)
			cerr << "Perameter to done must be a non-negative integer" << endl;

	
		if (exit_status >= 0){
			// exits with parameter
			if (tokens.size() == DONE_ARGS)	
				exit(exit_status);
		}

	}

}


// TO VARIABLE
//------------------------------------------------------------------------------------
// Function to store the output of a command to a variable
void tovar(vector<string> tokens, vector<string> & variables, vector<string> & values){


	// finds the index of a variable if it is already defined
	bool varPresent = false;
	int varIndex;	
	for (int i = 0; i < variables.size(); i++){

		if (tokens[1] == variables[i]){
			varPresent = true;
			varIndex = i;
		}
	}

	// creates the variable if it isn't already 
	// defined by calling setvar
	if (varPresent == false){
		setvar(tokens, variables, values);
		varIndex = variables.size() - 1;
	}


	// casts the tokens so that they can be used in excec
	char *argv[tokens.size()];
	for (int i = 0; i < tokens.size() - 2; i++){
		argv[i] = const_cast<char *>(tokens[i+2].c_str());
	}
	argv[tokens.size()-2] = NULL;

	// forks
	pid_t pid;
	int child_status;
	pid = fork();

	// gets the current working directory
	char buffer[MAX_VAR_LENGTH];
	string currentDir = getcwd(buffer, MAX_VAR_LENGTH);
	string desiredProgram;

	bool failed = false;
	
	// closes standard output to write 
	// to a temporary file
	int save_stdout = dup(STDOUT_FILENO);	
	freopen("tmp.txt", "w+", stdout);



	// child process
	if (pid == 0){
		
		// calls exec for command that is absolute
		if (tokens[2][0] == '/'){

			if (execv(argv[0], argv) == -1)	
				failed = true;
		}

		// calls exec for command that is in the current
		// working directory
		else if (tokens[2][0] == '.' && tokens[2][1] == '/'){

			desiredProgram =  currentDir + "/";
			desiredProgram += tokens[1].substr(2, tokens[1].size()-1);

			argv[0] = const_cast<char *>(desiredProgram.c_str());

			if(execv(argv[0], argv) == -1)
				failed = true;


		}

		// calls exec for commands that are just given as the name
		// by searching through a list of directories in PATH variable
		else {


			// NOTE*** values[0] is the PATH
			vector<string> paths;
			string pathList = values[0];
			string aPath = "";

			// tokenizes the list of paths by the ":" 
			for (int i = 0; i < pathList.size(); i++){

				while (pathList[i] != ':' && i < pathList.size()){
					aPath += pathList[i];
					i++;
				}

				aPath += "/" ;
				aPath += tokens[2];
				paths.push_back(aPath);
				aPath = "";
			}
	
			
			// casts to the appropriate type and calls exec
			int numFail = 0;
			for (int j = 0; j < paths.size(); j++){

				argv[0] = const_cast<char *>(paths[j].c_str());					

				if (execv(argv[0], argv) == -1)
					numFail++;

			}
			
			// determines if the command was not found in
			// any of the directories by comparing the 
			// number of fails to the number of paths
			if (numFail == paths.size())
				failed = true;
		}	

		// prints error for non-existing files
		if (failed == true){
			cerr << tokens[2] << ": No such file or directory" << endl;
			return;
		}
	}

	// parent
	else{
		
		waitpid(pid, &child_status,0);

		// brings stdout back to screen
		dup2(save_stdout, STDOUT_FILENO);
		close(save_stdout);

	}


	// gets size of file
	//---------------------------------------
	FILE *f;
	f =  fopen("tmp.txt","r");

	int size;
	fseek(f,0,SEEK_END);
	size = ftell(f);

	fclose(f);	
	//----------------------------------------	


	// reads the contents of the output of exec
	//----------------------------------------
	ifstream file;
	file.open("tmp.txt");	
	char * output = new char [size];

	file.read(output, size);

	file.close();
	//----------------------------------------


	// sets the variable to the output
	// of the command
	string varValue(output);
	values[varIndex] = varValue;


	// removes the temporary file
	remove("tmp.txt");
}



// RUN AND FLY
//--------------------------------------------------------------------------------------------------------
// Funtion to run the specified program.
// This function takes input as:
// "run cmd [param...]" Where run corresponds
// to tokens[0] and cmd is tokens[1] and params are
// tokens[2...n] respectivley
void runFly(vector<string> tokens, vector<int> & procInfo, vector<string> & procName, vector<string> values){

	// casts the tokens to the appropriate type 
	char *argv[tokens.size()];
	for (int i = 0; i < tokens.size() - 1; i++){
		argv[i] = const_cast<char *>(tokens[i+1].c_str());
	}
	argv[tokens.size()-1] = NULL;


	// forks
	pid_t pid;
	int child_status;
	pid = fork();

	// gets the current working direcory
	char buffer[MAX_VAR_LENGTH];
	string currentDir = getcwd(buffer, MAX_VAR_LENGTH);
	string desiredProgram;


	bool failed = false;

	// child process
	if (pid == 0){
		
		// calls exec for commands that are absolute
		if (tokens[1][0] == '/'){

			if (execv(argv[0], argv) == -1)	
				failed = true;
		}

		// calls exec for commands that are in the current 
		// working directory
		else if (tokens[1][0] == '.' && tokens[1][1] == '/'){

			desiredProgram =  currentDir + "/";
			desiredProgram += tokens[1].substr(2, tokens[1].size()-1);

			argv[0] = const_cast<char *>(desiredProgram.c_str());

			if(execv(argv[0], argv) == -1)
				failed = true;


		}
		

		// calls exec for commands that are just given as the name
		// by searching through a list of directories in PATH variable
		else {


			// NOTE*** values[1] is the PATH
			vector<string> paths;
			string pathList = values[0];
			string aPath = "";
			
			// tokenizes the list of paths based on the ":"
			for (int i = 0; i < pathList.size(); i++){

				while (pathList[i] != ':' && i < pathList.size()){
					aPath += pathList[i];
					i++;
				}

				aPath += "/" ;
				aPath += tokens[1];
				paths.push_back(aPath);
				aPath = "";
			}

			

			int numFail = 0;

			// casts to the appropriate type and calls exec
			for (int j = 0; j < paths.size(); j++){

				argv[0] = const_cast<char *>(paths[j].c_str());					

				if (execv(argv[0], argv) == -1)
					numFail++;

			}


			// determines if the command was not found in
			// any of the directories by comparing the 
			// number of fails to the number of paths	
			if (numFail == paths.size())
				failed = true;
		}	

		// determines if user entered non-existing file
		if (failed == true){
			cerr << tokens[1] << ": No such file or directory" << endl;
		}
			
	}

	// parent
	else{
		int result;
		result = waitpid(pid, &child_status, WNOHANG);

		// RUN
		//------------------------------------
		// determines if this is a run or fly
		// command. If it is a run command it
		// waits until the process has completed
		// to issue prompt.
		if (tokens[0] == "run")
			waitpid(pid, &child_status,0);
		//--------------------------------------


		// FLY
		//-----------------------------------------
		// pushes the background process info back
		// for the fly command
		else if (result == 0){
			
			procName.push_back(argv[0]);
			procInfo.push_back(pid);

		}

	}
}



