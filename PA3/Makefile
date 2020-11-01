# Makefile v1 for CSCI3753-F20 PA3

CC = gcc
CFLAGS = -Wextra -Wall -g 
INCLUDES = 
LFLAGS = 
LIBS = -lpthread

MAIN = multi-lookup

# Add any additional .c files to SRCS and .h files to HDRS
SRCS = multi-lookup.c util.c
HDRS = multi-lookup.h util.h

OBJS = $(SRCS:.c=.o) 

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean
clean: 
	$(RM) *.o *~ $(MAIN)


performance: $(MAIN)
	@echo "Number for requester thread: 1"
	@echo "Number for resolver thread = 1"
	./multi-lookup 1 1 req.log res.log input/*
	@echo ""
	@echo "Number for requester thread: 1"
	@echo "Number for resolver thread = 3"
	./multi-lookup 1 3 req.log res.log input/*
	@echo ""
	@echo "Number for requester thread: 3"
	@echo "Number for resolver thread = 1"
	./multi-lookup 3 1 req.log res.log input/*
	@echo ""
	@echo "Number for requester thread: 3"
	@echo "Number for resolver thread = 3"
	./multi-lookup 3 3 req.log res.log input/*
	@echo ""
	@echo "Number for requester thread: 5"
	@echo "Number for resolver thread = 5"
	./multi-lookup 5 5 req.log res.log input/*
	@echo ""


SUBMITFILES = $(SRCS) $(HDRS) Makefile README.md performance.txt
submit: 
	@read -r -p "Enter your identikey username: " username; \
	echo; echo Bundling the following files for submission; \
	tar --transform "s|^|PA3-$$username/|" -cvf PA3-$$username.txt $(SUBMITFILES); \
	echo; echo Please upload the file PA3-$$username.txt to Canvas to complete your submission; echo
