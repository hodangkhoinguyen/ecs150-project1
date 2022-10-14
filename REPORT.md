# Report

## Summary

This program `sshell` can take command from users with pipeline supported, then 
execute those actions from the kernel. It is also able to display some errors 
if needed, but the program won't die until the users enter `exit`.

## Introduction

Every time the program launches, the terminal displays a prompt `sshell$` and
waits for users to enter their command. After the users enter the command, it
starts to first parse the command and check if there's any error. If not, the
program proceeds to executes the command. For the execution phase, once it's
done with execution, the program will notify `+ completed ...` followed by the
status for each pipe (e.g. `0` means success).

## Implementation

The implementation of this program follows the steps:
1. Parsing the command into pipelines, and each pipelines to arguments.
2. If there is only one pipeline, it will execute seperately because it needs
to see if that command is built-in.
3. If there are 2 pipelines or more, then it will use pipe to assign read and
write endings for each forked child.

### Parsing the command

At the beginning of each execution, the program will create a struct `Pipeline`
which stores pipeSize, list of pipelines, list of status (if fork is executed), 
and list of struct `Config`.

The struct `Config` represents for one single Pipeline process, which has argument
size, list of arguments, and check if there is output redirection.

The struct `FD` holds 2 R/W endings of each pipe. So, when we execute pipelines
of size `n`, then there should be `n-1` `FD`'s.

The whole input command will be splitted by `|` sign into `Pipeline`. After
obtaining the command for each pipeline, it's used to parse into each `Config`.
At here, for each phase of parsing, we always check some restrictions (e.g.
empty, too many argumments) before moving to the next one.

### Builtin commands

These commands are executed if we see there is only 1 pipeline after parsing.
Then, the `Config` will be executed and see if the command is builtin. No
fork needed for this process.

### Piping

If the pipe size is 2 or more, let's call size of `n`, there must be `n-1` pipes
required. We open `n-1` pipes to used. For each forked process (both children),
we make sure to close them before exiting. We use `otherCommand` function to
execute these pipes, because they are not builtin commands. Then, the parents
will wait for their children to finish executing concurrently and printout
the status for all child processes.

However, for struct `Pipeline`, we strictly assign the arrays `status` and
`Config` to the size of 4, so it only supports 4 pipelines maximum.

### Output redirection

Because each `Config` has the property `isOutputRedirection`, we just need to open the if it's true and vice versa, using `dup2` for `STDOUT`. Then continue
executing the rest as usual. If output redirection needed, it also checks if the
pipeline is the last one. If not, it causes an error.

### Input redirection

Similar with output redirection, `Config` has input redirection property. If it's
true, then we redirect the `STDIN` to the file using `dup2`. Also, we needs to
check if the pipeline is the first one, otherwise it causes an error.

### Directory stack

There are 2 structures building the stack. One is `StackNode` that holds the
current directory and the next node. And the main `Stack` holds the top node and
the size of the stack. We can consider this data struct as a single linked list.
There are 3 methods for stack: `popd`, `pushd`, `dirs`. `pushd` and `popd` 
behaves similarly with `cd`, but the stack will be updated.
If the top `StackNode` has `next = NULL`, then the stack cannot be poped.

## Testing

Testing was mostly done using the `tester.sh` file that was given to us. Before
the test file, we use some cases on the project prompt to verify our progress.

## Some notes

In the program, we have used many built-in libraries and define some constants.
Besides, we take advantage of `EXIT_FAILURE` and `EXIT_SUCCESS` not only for the
`exit()` function, but also for return value of some functions that we want to
see if those functions can run correctly. For example, the `parseCommand` and
`parsePipeline` use those constants to see if the input command has any error.

## References

When using split function for parsing, it may change the original string. Thus,
we make a copy of the string and pass it to the split function. We refer to
this link: 
https://stackoverflow.com/questions/17104953/c-strtok-split-string-into-tokens-but-keep-old-data-unaltered

For the stack struct, we refer the link below and implement our own:
https://stackoverflow.com/questions/1919975/creating-a-stack-of-strings-in-c

Besides, we refer many examples that are given during lectures, slides, and
discussions in ECS 150 of Professor Joel Porquet-Lupine.

## Experience and Lesson

This project seems complicated at first and it is truly time-consuming. However,
it is good that we start early, especially we have rewrite the parsing phase
many times to finalize our program. The most difficult part about this project
is we are not sure if we go on the correct track, because there are times later
on we realize that we should have implemented some of the functions or structs
differently to adapt the new phase. But it gives us real experience because
these things actually happen in programming work everyday.

We get a chance to use GitHub a lot, although both of us have experience with 
it before. We also learn to apply good code styles and editor config to keep
the code appearance consistently.

One of the drawbacks is some functions should be optimally divided into smaller 
tasks. We also should have planned better to reduce rewriting code many times.
Because both of us don't have much experience with C, so it takes so much time
to research and learn many new functions. For example, the way `strtok` logic
is pretty weird.

Overall, this project is interesting and a good start for the course.
