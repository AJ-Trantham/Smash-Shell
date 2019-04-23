#
# Makefile --- Demonstrate how to rebuild *.o files if a *.h file changes
#
# Note:  This approach uses gcc -MM and a dependency file
#
# Operation
#    1. The first dependency for target all is rules.d, and make will bring
#       this file up-to-date first.
#    2. The rule for building rules.d ensures the file will be rebuilt if any
#       C source, header or the Makefile itself changes.  Note the use of
#       gnu make's wildcard function to generate a list of names of C source
#       and header files.
#    3. The recipe for rules.d invokes the gcc compiler with its -MM option
#       causing gcc to write a list of Makefile rules to stdout rather than
#       compiling anything.  Each of these rules is in the form of:
#               something.o: something.c someHeaderFile.h anotherHeader.h
#    4. The include statement is a directive not a rule.  It causes make to
#       read the contents of rules.d, the true dependencies for each *.o
#    5. The executable program contains a dependency upon each of the *.o
#       files which make will update (using the rules included from
#       the rules.d file) before linking the objects.
#
# WARNINGS
#       The use of gcc's -MM option, gnumake's wildcard function, and gnumake's
#       include directive are significant dependencies upon the gnu toolchain.
#       Verify these features exist in your toolchain before committing your
#       project's build to them.
#
# Author(s)
# 09/05/18 Epoch............................................................jrc
# 02/18/19 AJ Trantham
#------------------------------------------------------------------------------

#-------Define names of all the object files in this project
        OBJS = smash.o

#-------Define the names of all my header files in this project
        HDRS = smash.h, history.h

#-------Define the name of the resulting released product
        EXE = smash

#-------Define options passed by make to the compiler
        CFLAGS = -Wall -std=c99

#-------Define the name of the compiler to be used
        CC = gcc

#-------Override the rule for compiling *.c files to consider *.h dependencies
#%.o: %.c $(HDRS)
#       $(CC) -c $(CFLAGS) $< -o $@

#-------Define target "all" for building the executable(s)
all:    rules.d $(EXE)

# ------Rebuild rules.d if it' out of date with any *.c or *.h file gcc -MM
rules.d: Makefile $(wildcard *.c) $(wildcard *.h)
				gcc -MM $(wildcard *.c) >rules.d

#------Incorporate the auto-generated dependencies from rules.d into this Makefile
-include rules.d

#-------Rule for linking the executable product
$(EXE): $(OBJS)
				$(CC) $(CFLAGS) $^ -o $@

#-------Rule for debugging
debug: CFLAGS += -DDEBUG -g -Og
debug: smash

#-------Target for building a debug version to be used for valgrind
valgrind: debug
				valgrind --leak-check=yes --leak-check=full  --track-origins=yes --leak-check=full --show-leak-kinds=all smash

#-------Rule for cleaning build and emacs artifacts
clean:
				rm -f $(OBJS) $(EXE) *~
