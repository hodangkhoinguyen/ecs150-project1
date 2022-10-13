# Report

### All of the Structs

---

We decided to implement 4 structs named simply `Config`, `FD`, `Pipeline`, and
`Stack`.

`Config` is used to store our arguments and anything else pertaining to it, such
as size and output redirection. We decided to implement this way because \_\_\_.

`FD` is used to store the file descriptors for piping.

`Pipeline` is used to keep track of the **\_**.

`Stack` is used to store the current working directory and a pointer to the next
stack. We decided to implement this way because \_\_\_.

## Implementation

---

## Testing

---

Testing was mostly done using the `tester.sh` file that was given to us. Before
the test file, Christian was mostly moving the branch into the CSIF and manually
testing edge cases.

## Sources

---

In the function `parseCommand()`, there was a [stackoverflow question](https://stackoverflow.com/questions/17104953/c-strtok-split-string-into-tokens-but-keep-old-data-unaltered) that we
found extremely helpful in the creation of the function.
