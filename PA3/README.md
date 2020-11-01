# Programming Assignment 3

Author: Earl Potters


## Description

In this assignment you will develop a multi-threaded application, written in C, that resolves domain names to IP addresses.  This is similar to the operation performed each time you access a new website in your web browser.  The application will utilize two types of worker threads: requesters and resolvers.  These threads communicate with each other using a shared array, also referred to as a bounded buffer. 

## Building

To compile, run ``make``.

And `touch res.log req.log`

## Running

To run:

```
./multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ...]
```
If you compiled with make, the executable is compiled.
You can run with something like this example:
```
./multi-lookup 3 3 req.log res.log input/*
```

### Test program
To run the test program run:
```
make performance > performance.txt
```
and inspect ``performance.txt``
