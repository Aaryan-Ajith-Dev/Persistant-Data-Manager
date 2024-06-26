# Persistant Data Manager (PDS)
## Introduction

This project is an add-on to the existing pds lab 3b which initially contained get and store functionalities via a bst constructed from an index file. I have implemented transactions and in the process, I have maintained multiple versions for a data object (similar to multiverison concurrency control) in an attempt to support concurrency. 
*This project currently supports only one user at a time.*

## Features

- Creating a transaction
- Implemented rollback
- Implemented commit
- Implemented update
- Implemented Mulitple Versions for a given data item
- Implemented the corresponding uncommited versions of put, get, update

## Working of Transaction and Multiversion
Every record is appended with a version.

If the version is 0 (commited version), then it was appended normally (no uncommited transaction used it), if it is -1, then it is an invalid record and otherwise it refers to the transaction id of the transaction that has appended it.

This project contains different functions for put, get, etc. when a transaction operates. Whenever a transaction modifies / adds a record, it modifies its version (if the version doesn't exist, it is created).
The get_uncommited function ensures that the record it gets has a version 0 or tid (calling transactions id)

In rollback, all versions that have version = tid are invalidated by setting version = -1

In commit, all records that have a version of 0 and another records with version as tid are invalidated, and the latter record's version is set to 0.

The init_transaction just writes '0' to the transaction.dat which contains the number of transactions that are executing / have executed. This is used to generate transaction ID.


## Attempted Solutions for concurrency
- Can be achieved using OS functionalities to implement file locking and loading the bst in shared memory.
- Whenever a user calls the pds_open(repo_name), it checks whether the shared memory exists or not and accordingly creates / opens it.


## Usage

cc init_transaction.c <br/>
./a.out

cc -o pds_tester pds_tester.c pds.c bst.c contact.c<br/>
./pds_tester input_file.in

## Testing
- The public_testcase.in contains the START_TRANSACTION and ROLLBACK, COMMIT commands that can be tested by running the pds_tester with that input file.
- Every contact record that is created has initial values as "dummy_values"
- The update command updates this to "dummy_values2", this can be used to check if update has executed correctly or not
- Similary, the input file also tests rollback and commit.
