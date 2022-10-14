# Report

## Summary
This program `sshell` can take command from users with pipeline supported, then 
execute those actions from the kernel. It is also able to display some errors 
if needed, but the program won't die until the users enter `exit`.


## Implementation

The implementation of this program follows the steps:
1. Parsing the command into pipelines, and each pipelines to arguments.
2. If there is only one pipeline, it will execute seperately because it needs
to see if that command is built-in.
3. If there are 2 pipelines or more, then it will use pipe to assign read and
write endings for each forked child.

# Parsing the command

The struct `Pipeline` stores pipeSize, list of pipelines, list of status (if
fork is executed), and list of struct `Config`.

The struct `Config` represents for one single Pipeline process, which has 
argument size, list of arguments, and check if there is output redirection.

The struct `FD` holds 2 R/W endings of each pipe. So, when we execute pipelines
of size `n`, then there should be `n-1` `FD`'s.

The whole input command will be splitted by `|` sign into `Pipeline`. After
obtaining the command for each pipeline, it's used to parse into each `Config`.
At here, for each phase of parsing, we always check some restrictions (e.g.
empty, too many argumments) before moving to the next one.

# Builtin commands

These commands are executed if we see there is only 1 pipeline after parsing.
Then, the `Config` will be executed and see if the command is builtin. No
fork needed for this process.

# Piping

If the pipe size is 2 or more, let's call size of `n`, there must be `n-1` pipes
required. We open `n-1` pipes to used. For each forked process (both children),
we make sure to close them before exiting. We use `otherCommand` function to
execute these pipes, because they are not builtin commands. Then, the parents
will wait for their children to finish executing concurrently and printout
the status for all child processes.

However, for struct `Pipeline`, we strictly assign the arrays `status` and
`Config` to the size of 4, so it only supports 4 pipelines maximum.

# Output redirection

Because each `Config` has the property `isOutputRedirection`, we just need to open the if it's true and vice versa, using `dup2` for `STDOUT`. Then continue
executing the rest as usual.

## Testing

---

Testing was mostly done using the `tester.sh` file that was given to us. Before
the test file, Christian was mostly moving the branch into the CSIF and manually
testing edge cases.

## Sources

In the function `parseCommand()`, there was a [stackoverflow question](https://stackoverflow.com/questions/17104953/c-strtok-split-string-into-tokens-but-keep-old-data-unaltered) that we
found extremely helpful in the creation of the function.
// https://stackoverflow.com/questions/17104953/c-strtok-split-string-into-tokens-but-keep-old-data-unaltered

https://stackoverflow.com/questions/1919975/creating-a-stack-of-strings-in-c

professor notes and slides
