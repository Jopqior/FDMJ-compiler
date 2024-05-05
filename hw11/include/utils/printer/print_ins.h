/*
 * print_ins.h - functions to print out instruction list in AS_instrList in xml format *
 */

#ifndef PRINT_INS_H
#define PRINT_INS_H

#include <stdio.h>
#include "util.h"
#include "temp.h"
#include "assem.h"

void AS_print_xml(FILE *out,  AS_instrList il, Temp_map m);

#endif