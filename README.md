# MyShell
Building a Basic Shell in C.
Continuously prompts user to enter the comand until the user types quit.

Builtin Commands:
cd - changes to the mentioned directory.
pwd- shows the path of the current directory.
quit - quits the process.

Executables:
Takes in an executable file and runs it as a foreground process.
Stops the foreground process on Ctrl-C without the stopping the entire shell.
Background Process:
Runs a file in Background without stopping the shell.

Job Control:
Stops a foreground job.
Brings a stopped foregorund job to foreground again.
Brings a Background job to foreground.
Kills a job.
Sends a stopped job to background.

I/O Redirection:
Sends an output the specified file.
Takes in an input from a file.
