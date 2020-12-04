#include<stdlib.h>
#include <stdio.h>
#include<limits.h>
#include<errno.h>
#include <string.h>


/*

Arithmetic ops:
    add
    sub
    neg
    and
    or
    not

Logical ops (true := 1111111111111111B == -1) (false := 000...B == 0)
    gt
    lt
    eq


Memory ops:
    pop  <segment> <i>
    push <segment> <i>

    Segments:                                       Hack computer implementation
        local               
            gives Ram[LCL+<i>]                      256
        argument
            gives Ram[ARG+<i>]                      ??
        this
            gives Ram[THIS+<i>]                     set by pointer 2
        that
            gives Ram[THAT+<i>]                     set by pointer 1
        constant
            gives the constant <i>                  not "real" memory, set A reg
        static
            gives Foo.<i> allocated by assembler    16
            *Only Ram[16] - Ram[255] available
              Function checks this
        temp
            gives Ram[R5+i].                        R5
            Only 8 slots
        pointer
            Ram[R13+i]                              R13
            Only 2 slots
            accessing pointer 0 should result in accessing THIS
            accessing pointer 1 ... THAT

Branching:
    label <label>
    goto <label>
    if-goto <label>

Function commands:
    function <name> <nVars>
    call <name> <nArgs>
    return
*/

#define LONGEST_FIRST_WORD 8
#define LONGEST_INSTRUCTION 50   //* Picked arbitrary value..


typedef struct entry {
    char  key[LONGEST_FIRST_WORD];
    enum {LOGICAL, PUSHPOP} type;
    union {
        void (*lfn)(FILE *fout);
        void (*ppfn)(FILE *fout, char *segment, char *istring);
    } f;
} entry;


void add(FILE *fout) {
    fputs("// add\n"
          "@SP\n"
          "M=M-1\n" // set new stack pointer to the address one above it
          "A=M\n"    // set address to 1st val in stack (where new stack pointer is)
          "D=M\n"    // store first val in register
          "A=A-1\n"  // address 2nd val in stack (one above current address)
          "M=M+D\n",  // (2nd val in stack) = (itself) + (1st val in stack),
          fout);
}

void sub(FILE *fout) {
    fputs("// sub\n"
          "@SP\n"
          "M=M-1\n" // set new stack pointer to the address one above it
          "A=M\n"    // set address to 1st val in stack (where new stack pointer is)
          "D=M\n"    // store first val in register
          "A=A-1\n"  // address 2nd val in stack (one above current address)
          "M=M-D\n",  // (2nd val in stack) = (itself) + (1st val in stack),
          fout);
}

void neg(FILE *fout) {
    fputs("// neg\n"
          "@SP\n"
          "A=A-1\n"
          "M=-M\n",
          fout);
}

void gt(FILE *fout) {
    fputs("// gt\n"
          "@SP\n"
          "M=M-1\n" // set new stack pointer to the address one above it
          "A=M\n"   // set address to 1st val in stack (where new stack pointer is)
          "D=M\n"   // store first val in register
          "A=A-1\n" // address 2nd val in stack (one above current address)
          "D=M-D\n" // store (2nd val in stack) - (1st val in stack) in register
          "@__TRUE\n"
          "D;JGT\n" // register > 0?  If so, goto __TRUE
          "@SP\n"   // if false: set new 1st val in stack to 00...B
          "A=M-1\n"
          "M=0\n",
          fout);
}

void writeTrueGlobalCode(FILE *fout) {
    //! This might be sensitive to bugs, global var __TRUE may break it
    fputs("// Global code used for vm commands gt, eq, lt\n"
          "(__TRUE)\n"
          "@SP\n"
          "A=M-1\n"  // address first value in stack
          "M=-1\n",  // set it to 1111111111111111B
          fout);
}

void eq(FILE *fout) {
    fputs("// eq\n"
          "@SP\n"
          "M=M-1\n" // set new stack pointer to the address one above it
          "A=M\n"   // set address to 1st val in stack (where new stack pointer is)
          "D=M\n"   // store first val in register
          "A=A-1\n" // address 2nd val in stack (one above current address)
          "D=M-D\n" // store (2nd val in stack) - (1st val in stack) in register
          "@__TRUE\n"
          "D;JEQ\n" // register == 0?  If so, goto __TRUE
          "@SP\n"   // if false: set new 1st val in stack to 00...B
          "A=M-1\n"
          "M=0\n",
          fout);
}

void lt(FILE *fout) {
    fputs("// lt\n"
          "@SP\n"
          "M=M-1\n" // set new stack pointer to the address one above it
          "A=M\n"   // set address to 1st val in stack (where new stack pointer is)
          "D=M\n"   // store first val in register
          "A=A-1\n" // address 2nd val in stack (one above current address)
          "D=M-D\n" // store (2nd val in stack) - (1st val in stack) in register
          "@__TRUE\n"
          "D;JLT\n" // register < 0?  If so, goto __TRUE
          "@SP\n"   // if false: set new 1st val in stack to 00...B
          "A=M-1\n"
          "M=0\n",
          fout);
}

void and(FILE *fout) {
    fputs("// and\n"
          "@SP\n"
          "M=M-1\n" // set new stack pointer to the address one above it
          "A=M\n"    // set address to 1st val in stack (where new stack pointer is)
          "D=M\n"    // store first val in register
          "A=A-1\n"  // address 2nd val in stack (one above current address)
          "M=M&D\n",  // (2nd val in stack) = (itself) & (1st val in stack),
          fout);
}

void or(FILE *fout) {
    fputs("// or\n"
          "@SP\n"
          "M=M-1\n" // set new stack pointer to the address one above it
          "A=M\n"    // set address to 1st val in stack (where new stack pointer is)
          "D=M\n"    // store first val in register
          "A=A-1\n"  // address 2nd val in stack (one above current address)
          "M=M|D\n",  // (2nd val in stack) = (itself) | (1st val in stack),
          fout);
}

void not(FILE *fout) {
    fputs("// not\n"
          "@SP\n"
          "A=A-1\n"
          "M=-M\n",
          fout);
}

void printPush(FILE *fout) {
    fputs("@SP\n"
          "M=M+1\n" // set new top of stack one down
          "A=M-1\n" // point to address one above (new 1st val of stack)
          "M=D\n", // put stored value in memory
          fout);
}

void printPop(FILE *fout) {
    fputs("@SP\n"
          "M=M-1\n" // set new top of stack one up
          "A=M+1\n" // point to address one down (where popped val from stack is)
          "D=M\n", // put value in memory in register D
          fout);
}

void pushRegularSegment(FILE *fout, char *segmentSymbol, long int i) {
    if (i > 16384L) {
        perror("Argument given to push is greater than size of Ram (16KiB");
        exit(EXIT_FAILURE);
    }
    
    fprintf(fout, 
            "// push %s\n"
            "@%ld\n"
            "D=A\n"   // store local index in D
            "@%s\n"
            "A=M\n"   // point to main address
            "A=A+D\n" // point to local index
            "D=M\n"   // store value in local index
            "@SP\n"
            "M=M+1\n" // set new top of stack one down
            "A=M-1\n" // point to address one above (new 1st val of stack)
            "M=D\n", // put stored value in memory
            segmentSymbol, i, segmentSymbol);
}

void popRegularSegment(FILE *fout, char *segmentSymbol, long int i) {
    if (i > 16384L) {
        perror("Argument given to pop is greater than size of Ram (16KiB");
        exit(EXIT_FAILURE);
    }
    
    fprintf(fout, 
            "// pop %1$s\n"
            "@%2$ld\n"   // store segment+idx in segment register (e.g LCL) temporarily
            "D=A\n"
            "@%1$s\n"
            "M=M+D\n"
            "@SP\n"   // Pop from stack into D, and adjust stack pointer one up
            "M=M-1\n" 
            "A=M+1\n" 
            "D=M\n"   
            "@%1$s\n"   // point to segment register
            "M=D\n"   // store popped value into memory
            "@%2$ld\n"   // restore segment register to original value
            "D=A\n"
            "@%1$s\n"
            "M=M-D\n",
            segmentSymbol, i);  
}

void pushStaticSegment(FILE *fout, long int i) {
    if (i > 240L) {
        perror("In Hack implementation of vm code, code overflows the static \n"
               "segment into the stack (static segment only holds 240 spots).");
        exit(EXIT_FAILURE);
    }

    char *filebasename = "File";  //TODO: Add function to determine this

    fprintf(fout,
        "// push static\n"
        "@%s.%ld\n"
        "D=M\n"
        "@SP\n"
        "M=M+1\n"
        "A=M-1\n"
        "M=D\n",
        filebasename,i);
}

void popStaticSegment(FILE *fout, long int i) {
    if (i > 240L) {
        perror("In Hack implementation of vm code, code overflows the static \n"
               "segment into the stack (static segment only holds 240 spots).");
        exit(EXIT_FAILURE);
    }

    char *filebasename = "File";  //TODO: Add function to determine this

    fprintf(fout,
        "// push static\n"
        "@SP\n"     // Pop from stack into D, and adjust stack pointer one up
        "M=M-1\n"
        "A=M+1\n"
        "M=D\n"
        "@%s.%ld\n" // Store popped value into static memory address
        "M=D\n",
        filebasename, i);
}

void push(FILE *fout, char *segment, char *istring) {
    char *str_end = NULL;
    long int i = strtol(istring, &str_end, 10);

    if (str_end == istring) {
        perror("No argument given to push");
        exit(EXIT_FAILURE);
    }
    
    if (!strcmp(segment, "local"))
        pushRegularSegment(fout, "LCL", i);
    else if (!strcmp(segment, "argument"))
        pushRegularSegment(fout, "ARG", i);
    else if (!strcmp(segment, "this"))
        pushRegularSegment(fout, "THIS", i);
    else if (!strcmp(segment, "that"))
        pushRegularSegment(fout, "THAT", i);
    else if (!strcmp(segment, "constant"))
        fprintf(fout,
            "// push constant\n"
            "@%ld\n"
            "D=A\n"  // store constant value
            "@SP\n"
            "M=M+1\n" // move stack pointer one down
            "A=M-1\n" // address new first value in stack
            "M=D\n",  // store constant at new first value
            i);
    else if (!strcmp(segment, "static"))
        pushStaticSegment(fout, i);
    else if (!strcmp(segment, "temp"))
        fprintf(fout,
            "// push temp\n"
            "@%ld\n"
            "D=M\n"
            "@SP\n"
            "M=M+1\n"
            "A=M-1\n"
            "M=D\n"
            ,i+5L);
    else if (!strcmp(segment, "pointer"))
        fprintf(fout,
            "// push pointer\n"
            "@%ld\n"
            "D=M\n"
            "@SP\n"
            "M=M+1\n"
            "A=M-1\n"
            "M=D\n"
            ,i+13L);
    else {
        printf("'%s'", segment);
        perror(" was found as an invalid segment specifier");
        exit(EXIT_FAILURE);
    }
    
}

void pop(FILE *fout, char *segment, char *istring) {
    char **str_end = NULL;
    long int i = strtol(istring, str_end, 10);

    if (*str_end == istring) {
        perror("No natural number argument given to push");
        exit(EXIT_FAILURE);
    }
    
    if (!strcmp(segment, "local"))
        popRegularSegment(fout, "LCL", i);
    else if (!strcmp(segment, "argument"))
        popRegularSegment(fout, "ARG", i);
    else if (!strcmp(segment, "this"))
        popRegularSegment(fout, "THIS", i);
    else if (!strcmp(segment, "that"))
        popRegularSegment(fout, "THAT", i);
    else if (!strcmp(segment, "constant")) {
        perror("Doesn't make sense to store in constant segment");
        exit(EXIT_FAILURE);
    } else if (!strcmp(segment, "static"))
        popStaticSegment(fout, i);
    else if (!strcmp(segment, "temp")) // TODO: bound check i
        fprintf(fout,
            "// pop temp\n"
            "@SP\n"
            "M=M-1\n"
            "A=M+1\n"
            "D=M\n"   // store popped value into D
            "@%ld\n"  // address the temp slot referenced
            "M=D\n"   // put popped value into memory
            ,i+5L);
    else if (!strcmp(segment, "pointer")) // TODO: bound check i
        fprintf(fout,
            "// pop pointer\n"
            "@SP\n"
            "M=M-1\n"
            "A=M+1\n"
            "D=M\n"
            "@%ld\n"
            "M=D\n"
            ,i+13L);
    else {
        printf("'%s'", segment);
        perror(" was found as an invalid segment specifier");
        exit(EXIT_FAILURE);
    }
}

#define FNIDX_LENGTH 17
// Hopefully initializes uninitialized entries to 0
entry fnidx[FNIDX_LENGTH] = {
    {"add", LOGICAL, {&add}},
    {"sub", LOGICAL, {&sub}},
    {"neg", LOGICAL, {&neg}},
    {"gt",  LOGICAL, {&gt}},
    {"lt",  LOGICAL, {&lt}},
    {"eq",  LOGICAL, {&eq}},
    {"and", LOGICAL, {&and}},
    {"or",  LOGICAL, {&or}},
    {"not", LOGICAL, {&not}},
    {"push", PUSHPOP, {.ppfn=&push}},
    {"pop",  PUSHPOP, {.ppfn=&pop}}
};


int main(int argc, char *argv[]) {
    // int status = 0;
    
    FILE *fin = fopen("SimpleAdd/SimpleAdd.vm", "r");
    if (fin == NULL) {
        puts(strerror(errno));
    }
    FILE *fout = fopen("out.vm", "w");
    //char parsestr[] = "push constant 3";
    char strbuf[LONGEST_INSTRUCTION+1] = {'\0'};
    // buffer for strtok() to modify when it tokenizes
    // char tokbuf[LONGEST_INSTRUCTION+1] = {'\0'};

    while (!feof(fin) && !ferror(fin)) {
        //* Edge case: not at end of file in while test, but immediately
        //*   hit EOF in fgets().  fgets() then doesn't update strbuf bc
        //*   it read no characters.
        // Fix by setting first character to \0, so below will see an
        //   empty string
        strbuf[0] = '\0';
        fgets(strbuf, LONGEST_INSTRUCTION+1, fin);

        if (strbuf == NULL) {
            perror("fgets() in main() had an error");
            exit(EXIT_FAILURE);
        }

        char *toks[4] = {NULL, NULL, NULL, NULL};
        toks[0] = strtok(strbuf, " \r\n");
        toks[1] = strtok(NULL, " \r\n");
        toks[2] = strtok(NULL, " \r\n");
        toks[3] = strtok(NULL, " \r\n");

        // If no tokens found, continue
        if (toks[0] == NULL)
            continue;

        for (int i=0; i < 4; ++i) {
            // If command is longer than 50 characters
            if ((toks[i] != NULL) && 
                ((toks[i] - toks[0]) > (LONGEST_INSTRUCTION - 1))) {
                perror("Instruction retrieved is longer than 50 chars");
                exit(EXIT_FAILURE);
            }
        }

        for (int i=0; i < FNIDX_LENGTH; ++i) {
            if (!strncmp(toks[0], fnidx[i].key, LONGEST_FIRST_WORD)) {
                if (fnidx[i].type == LOGICAL)
                    (*fnidx[i].f.lfn)(fout);
                else if (fnidx[i].type == PUSHPOP)
                    (*fnidx[i].f.ppfn)(fout, toks[1], toks[2]);
                else {
                    printf("%d ", fnidx[i].type);
                    perror("number of arguments not supported/valid");
                    exit(EXIT_FAILURE);
                }
            }
        }

        // Go forward to next new line character.  fgets() will stop
        //   getting chars from fin automatically at \n :)
        char *eolp = memchr(strbuf, '\n', LONGEST_INSTRUCTION);
        while (eolp == NULL) {
            fgets(strbuf, LONGEST_INSTRUCTION, fin);
            eolp = memchr(strbuf, '\n', LONGEST_INSTRUCTION);
        }
        // Now fin is ready to give next line
    }

    fclose(fout);

    fclose(fin);

    printf("Finsihed successfully\n");


    return EXIT_SUCCESS;
    
    
}