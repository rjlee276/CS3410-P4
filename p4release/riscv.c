#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "linkedlist.h"
#include "hashtable.h"
#include "riscv.h"

/************** BEGIN HELPER FUNCTIONS PROVIDED FOR CONVENIENCE ***************/
const int R_TYPE = 0;
const int I_TYPE = 1;
const int MEM_TYPE = 2;
const int U_TYPE = 3;
const int UNKNOWN_TYPE = 4;

/**
 * Return the type of instruction for the given operation
 * Available options are R_TYPE, I_TYPE, MEM_TYPE, UNKNOWN_TYPE
 */
static int get_op_type(char *op)
{
    const char *r_type_op[] = {"add", "sub", "and", "or", "xor", "slt", "sll", "sra"};
    const char *i_type_op[] = {"addi", "andi", "ori", "xori", "slti"};
    const char *mem_type_op[] = {"lw", "lb", "sw", "sb"};
    const char *u_type_op[] = {"lui"};
    for (int i = 0; i < (int)(sizeof(r_type_op) / sizeof(char *)); i++)
    {
        if (strcmp(r_type_op[i], op) == 0)
        {
            return R_TYPE;
        }
    }
    for (int i = 0; i < (int)(sizeof(i_type_op) / sizeof(char *)); i++)
    {
        if (strcmp(i_type_op[i], op) == 0)
        {
            return I_TYPE;
        }
    }
    for (int i = 0; i < (int)(sizeof(mem_type_op) / sizeof(char *)); i++)
    {
        if (strcmp(mem_type_op[i], op) == 0)
        {
            return MEM_TYPE;
        }
    }
    for (int i = 0; i < (int)(sizeof(u_type_op) / sizeof(char *)); i++)
    {
        if (strcmp(u_type_op[i], op) == 0)
        {
            return U_TYPE;
        }
    }
    return UNKNOWN_TYPE;
}
/*************** END HELPER FUNCTIONS PROVIDED FOR CONVENIENCE ****************/

registers_t *registers;
hashtable_t *memhash;

// TODO: create any additional variables to store the state of the interpreter

void init(registers_t *starting_registers)
{
    registers = starting_registers;
    memhash = ht_init(400000);
    // TODO: initialize any additional variables needed for state
}

void end()
{
    // TODO: Free everything from memory
    free(registers);
    ht_free(memhash);
}

// TODO: create any necessary helper functions

// logic necessary to execute r-type instructions
void execute_r(char *op, int rd, int rs1, int rs2)
{
    if (rd == 0)
    {
        return;
    }

    const char *r_type_op[] = {"add", "sub", "and", "or", "xor", "slt", "sll", "sra"};

    if (strcmp(op, r_type_op[0]) == 0)
    {
        registers->r[rd] = registers->r[rs1] + registers->r[rs2];
    }
    else if (strcmp(op, r_type_op[1]) == 0)
    {
        registers->r[rd] = registers->r[rs1] - registers->r[rs2];
    }
    else if (strcmp(op, r_type_op[2]) == 0)
    {
        registers->r[rd] = registers->r[rs1] & registers->r[rs2];
    }
    else if (strcmp(op, r_type_op[3]) == 0)
    {
        registers->r[rd] = registers->r[rs1] | registers->r[rs2];
    }
    else if (strcmp(op, r_type_op[4]) == 0)
    {
        registers->r[rd] = registers->r[rs1] ^ registers->r[rs2];
    }
    else if (strcmp(op, r_type_op[5]) == 0)
    {
        registers->r[rd] = (registers->r[rs1] < registers->r[rs2]) ? 1 : 0;
    }
    else if (strcmp(op, r_type_op[6]) == 0)
    {
        registers->r[rd] = registers->r[rs1] << registers->r[rs2];
    }
    else if (strcmp(op, r_type_op[7]) == 0)
    {
        registers->r[rd] = registers->r[rs1] >> registers->r[rs2];
    }
}

int i_process(char *imm)
{
    if (strchr(imm, 'x') != NULL)
    {
        return (int)strtol(imm, NULL, 16);
    }
    return atoi(imm);
}

// logic necessary to execute i-type instructions
void execute_i(char *op, int rd, int rs1, int imm)
{

    if (rd == 0)
    {
        return;
    }

    if (imm & (1 << 11))
    {
        imm |= 0xFFFFF000;
    }

    if (imm <= 1048576 && imm >= 4096)
    {
        imm = imm % 4096;
    }

    const char *i_type_op[] = {"addi", "andi", "ori", "xori", "slti"};

    if (strcmp(op, i_type_op[0]) == 0)
    {
        registers->r[rd] = registers->r[rs1] + imm;
    }
    else if (strcmp(op, i_type_op[1]) == 0)
    {
        registers->r[rd] = registers->r[rs1] & imm;
    }
    else if (strcmp(op, i_type_op[2]) == 0)
    {
        registers->r[rd] = registers->r[rs1] | imm;
    }
    else if (strcmp(op, i_type_op[3]) == 0)
    {
        registers->r[rd] = registers->r[rs1] ^ imm;
    }
    else if (strcmp(op, i_type_op[4]) == 0)
    {
        registers->r[rd] = (registers->r[rs1] < imm) ? 1 : 0;
    }
}

// logic necessary to execute m-type instructions
void execute_m(char *op, int rd, int rs1, int imm)
{

    if (imm >= 4096)
    {
        imm = imm % 4096;
    }

    if (imm & (1 << 11))
    {
        imm |= 0xFFFFF000;
    }

    if (strcmp(op, "lw") == 0)
    {
        if (rd == 0)
        {
            return;
        }
        int b1 = ht_get(memhash, registers->r[rs1] + imm);
        int b2 = ht_get(memhash, registers->r[rs1] + imm + 1);
        int b3 = ht_get(memhash, registers->r[rs1] + imm + 2);
        int b4 = ht_get(memhash, registers->r[rs1] + imm + 3);

        registers->r[rd] = (b4 << 24) + (b3 << 16) + (b2 << 8) + b1;
    }
    else if (strcmp(op, "lb") == 0)
    {
        if (rd == 0)
        {
            return;
        }

        int lb = ht_get(memhash, registers->r[rs1] + imm);

        if (lb & (1 << 7))
        {
            lb |= 0xFFFFFF00;
        }

        registers->r[rd] = lb;
    }
    else if (strcmp(op, "sw") == 0)
    {
        int b1 = registers->r[rd] & 0xFF;
        int b2 = (registers->r[rd] >> 8) & 0xFF;
        int b3 = (registers->r[rd] >> 16) & 0xFF;
        int b4 = (registers->r[rd] >> 24) & 0xFF;

        ht_add(memhash, registers->r[rs1] + imm, b1);
        ht_add(memhash, registers->r[rs1] + imm + 1, b2);
        ht_add(memhash, registers->r[rs1] + imm + 2, b3);
        ht_add(memhash, registers->r[rs1] + imm + 3, b4);
    }
    else if (strcmp(op, "sb") == 0)
    {
        int reg = registers->r[rs1] + imm;
        ht_add(memhash, reg, registers->r[rd]);
    }
}

// logic necessary to handle u-type instructions
void execute_u(char *op, int rd, int imm)
{
    if (rd == 0)
    {
        return;
    }

    if (strcmp(op, "lui") == 0)
    {
        registers->r[rd] = imm << 12;
    }
}

// helper function space used for determining if character is space or newline
int space(int i)
{
    // if i equals 10 or 32 then return 1 else 0
    if (i == 10 || i == 32)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// trim whitespaces so as to make parsing manageable
char *trim(char *c)
{
    // while loop
    while (space(*c))
        c++;

    return c;
}

// TODO: write logic for evaluating instruction on current interpreter state
// step function deals with parsing instructions and then calling corresponding helper functions to execute instructions
void step(char *instruction)
{
    // Extracts and returns the substring before the first space character,
    // by replacing the space character with a null-terminator.
    // `instruction` now points to the next character after the space
    // See `man strsep` for how this library function works
    char *op = strsep(&instruction, " ");
    // Uses the provided helper function to determine the type of instruction
    int op_type = get_op_type(op);
    // Skip this instruction if it is not in our supported set of instructions
    if (op_type == UNKNOWN_TYPE)
    {
        return;
    }

    // parse necessary parts of string depending on value of op_type
    // if op_type == 0 (i.e. is a r-type instruction), parse string accordingly
    if (op_type == 0)
    {
        strsep(&instruction, "x");
        char *crd = strsep(&instruction, ",");
        strsep(&instruction, "x");
        char *crs1 = strsep(&instruction, ",");
        strsep(&instruction, "x");
        char *crs2 = strsep(&instruction, ",");
        int rd = atoi(crd);
        int rs1 = atoi(crs1);
        int rs2 = atoi(crs2);
        execute_r(op, rd, rs1, rs2);
    }
    // if op_type == 1 (i.e. is an i-type instruction), parse string accordingly
    else if (op_type == 1)
    {
        strsep(&instruction, "x");
        char *crd = strsep(&instruction, ",");
        strsep(&instruction, "x");
        char *crs1 = strsep(&instruction, ",");
        char *t = trim(instruction);
        char *cimm = strsep(&t, " ");
        int rd = atoi(crd);
        int rs1 = atoi(crs1);
        int imm = i_process(cimm);
        execute_i(op, rd, rs1, imm);
    }
    // if op_type == 2 (i.e. is a m-type instruction), parse string accordingly
    else if (op_type == 2)
    {
        strsep(&instruction, "x");
        char *crd = strsep(&instruction, ",");
        char *cimm = strsep(&instruction, "(");
        int imm = i_process(cimm);
        strsep(&instruction, "x");
        char *crs1 = strsep(&instruction, ")");
        int rd = atoi(crd);
        int rs1 = atoi(crs1);
        execute_m(op, rd, rs1, imm);
    }
    // if op_type == 3 (i.e. is a u-type instruction), parse string accordingly
    else if (op_type == 3)
    {
        strsep(&instruction, "x");
        char *crd = strsep(&instruction, ",");
        char *t = trim(instruction);
        char *cimm = strsep(&t, " ");
        int imm = i_process(cimm);
        int rd = atoi(crd);
        execute_u(op, rd, imm);
    }
}
