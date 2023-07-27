/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"


/*

*/

typedef struct
{
	int				s;
	dfunction_t		*f;
} prstack_t;

#define	MAX_STACK_DEPTH		256 //32
prstack_t	pr_stack[MAX_STACK_DEPTH + 1]; // PR_StackTrace uses the last one
int		pr_depth;
static int	pr_peakdepth;

#define	LOCALSTACK_SIZE		2048
int		localstack[LOCALSTACK_SIZE];
int		localstack_used;


qboolean	pr_trace;
dfunction_t	*pr_xfunction;
int		pr_xstatement;
qboolean	pr_ExtendedTrace;



int		pr_argc;

char *pr_opnames[] =
{
"DONE",

"MUL_F",
"MUL_V",
"MUL_FV",
"MUL_VF",

"DIV",

"ADD_F",
"ADD_V",

"SUB_F",
"SUB_V",

"EQ_F",
"EQ_V",
"EQ_S",
"EQ_E",
"EQ_FNC",

"NE_F",
"NE_V",
"NE_S",
"NE_E",
"NE_FNC",

"LE",
"GE",
"LT",
"GT",

"INDIRECT",
"INDIRECT",
"INDIRECT",
"INDIRECT",
"INDIRECT",
"INDIRECT",

"ADDRESS",

"STORE_F",
"STORE_V",
"STORE_S",
"STORE_ENT",
"STORE_FLD",
"STORE_FNC",

"STOREP_F",
"STOREP_V",
"STOREP_S",
"STOREP_ENT",
"STOREP_FLD",
"STOREP_FNC",

"RETURN",

"NOT_F",
"NOT_V",
"NOT_S",
"NOT_ENT",
"NOT_FNC",

"IF",
"IFNOT",

"CALL0",
"CALL1",
"CALL2",
"CALL3",
"CALL4",
"CALL5",
"CALL6",
"CALL7",
"CALL8",

"STATE",

"GOTO",

"AND",
"OR",

"BITAND",
"BITOR"
};

char *PR_GlobalString (int ofs);
char *PR_GlobalStringNoContents (int ofs);


//=============================================================================

/*
=================
PR_PrintStatement
=================
*/
void PR_PrintStatement (dstatement_t *s)
{
	if ( (unsigned)s->op < sizeof(pr_opnames)/sizeof(pr_opnames[0]))
		Con_SafePrintf ("%-10s ", pr_opnames[s->op]);

	if (s->op == OP_IF || s->op == OP_IFNOT)
		Con_SafePrintf ("%sbranch %i", PR_GlobalString((unsigned short)s->a), s->b);
	else if (s->op == OP_GOTO)
	{
		Con_SafePrintf ("branch %i", s->a);
	}
	else if ( (unsigned)(s->op - OP_STORE_F) < 6)
	{
		Con_SafePrintf ("%s", PR_GlobalString((unsigned short)s->a));
		Con_SafePrintf ("%s", PR_GlobalStringNoContents((unsigned short)s->b));
	}
	else
	{
		if (s->a)
			Con_SafePrintf ("%s", PR_GlobalString((unsigned short)s->a));
		if (s->b)
			Con_SafePrintf ("%s", PR_GlobalString((unsigned short)s->b));
		if (s->c)
			Con_SafePrintf ("%s", PR_GlobalStringNoContents((unsigned short)s->c));
	}
	Con_SafePrintf ("\n");
}

/*
============
PR_StackTrace
============
*/
void PR_StackTrace (void)
{
	dfunction_t	*f;
	int		i;

	if (pr_depth <= 0)
	{
		Con_SafePrintf ("<NO STACK>\n");
		return;
	}

	if (pr_depth > MAX_STACK_DEPTH)
		pr_depth = MAX_STACK_DEPTH;

	pr_stack[pr_depth].s = pr_xstatement;
	pr_stack[pr_depth].f = pr_xfunction;
	for (i=pr_depth ; i>0 ; i--)
	{
		f = pr_stack[i].f;

		if (!f)
		{
			Con_SafePrintf ("<NO FUNCTION>\n");
		}
		else
			Con_SafePrintf ("%12s : %s : statement %i\n", pr_String("PR_StackTrace1", f->s_file), pr_String("PR_StackTrace2", f->s_name), pr_stack[i].s - f->first_statement);
	}
}

/*
============
PR_Profile_f

============
*/
void PR_Profile_f (void)
{
	dfunction_t	*f, *best;
	int		max;
	int		num;
	int		i;

	if (!progs)
	{
		Con_SafePrintf ("No progs loaded, can't profile\n");
		return;
	}
	
	num = 0;
	do
	{
		max = 0;
		best = NULL;
		for (i=0 ; i<progs->numfunctions ; i++)
		{
			f = &pr_functions[i];
			if (f->profile > max)
			{
				max = f->profile;
				best = f;
			}
		}
		if (best)
		{
			if (num < 10)
				Con_SafePrintf ("%9i %s\n", best->profile, pr_String("PR_Profile_f", best->s_name));
			num++;
			best->profile = 0;
		}
	} while (best);
}

/*
============
PR_PrintState

Prints recent statements and a trace
============
*/
void PR_PrintState (void)
{
	int i;
	
	if (pr_xfunction)
	{
		for (i = (pr_ExtendedTrace || developer.value > 2) ? -7 : 0; i <= 0; i++)
		{
			if (pr_xstatement + i >= pr_xfunction->first_statement)
				PR_PrintStatement (pr_statements + pr_xstatement + i);
		}
	}
	
	pr_ExtendedTrace = false;
	PR_StackTrace ();
}

/*
============
PR_RunError

Aborts the currently executing function
============
*/
void PR_RunError (char *error, ...)
{
	va_list	argptr;
	char	string[MAX_PRINTMSG]; //1024

	va_start (argptr,error);
	COM_vsnprintf ("PR_RunError", string, sizeof(string) - 1, error, argptr);
	va_end (argptr);

	PR_PrintState ();
	Con_SafePrintf ("%s\n", string);

	// dump the stack so host_error can shutdown functions
	PR_RunClear ();

	Host_Error ("Program error");
}

/*
============
PR_RunError2

Simpler trace, exiting with Sys_Error
============
*/
void PR_RunError2 (char *error, ...)
{
	va_list		 argptr;
	char		 string[MAX_PRINTMSG];
	static qboolean	 inerror = false;

	va_start (argptr,error);
	COM_vsnprintf ("PR_RunError2", string, sizeof(string) - 1, error, argptr);
	va_end (argptr);

	if (!inerror && pr_depth > 0)
	{
		inerror = true; // Prevent recursion
		PR_StackTrace ();
	}

	inerror = false;

	PR_RunClear ();

	Sys_Error (string);
}

/*
============
PR_RunError3

Only trace, does not exit
============
*/
void PR_RunError3 (void)
{
	static qboolean	inerror = false;

	if (!inerror && pr_depth > 0)
	{
		inerror = true; // Prevent recursion
		PR_PrintState ();
	}

	inerror = false;
}

/*
============
PR_RunClear
============
*/
void PR_RunClear (void)
{
	// Clear stack variables
	pr_depth = localstack_used = pr_xstatement = 0;
	pr_xfunction = NULL;
}

/*
============================================================================
PR_ExecuteProgram

The interpretation main loop
============================================================================
*/

/*
====================
PR_EnterFunction

Returns the new program statement counter
====================
*/
int PR_EnterFunction (dfunction_t *f)
{
	int		i, j, c, o;

	pr_stack[pr_depth].s = pr_xstatement;
	pr_stack[pr_depth].f = pr_xfunction;
	pr_depth++;
	if (pr_depth >= MAX_STACK_DEPTH)
		PR_RunError ("PR_EnterFunction: stack overflow (%d, max = %d)", pr_depth, MAX_STACK_DEPTH - 1);

	if (pr_depth > pr_peakdepth)
		pr_peakdepth = pr_depth; // Remember peak depth

// save off any locals that the new function steps on
	c = f->locals;
	if (localstack_used + c > LOCALSTACK_SIZE)
		PR_RunError ("PR_EnterFunction: locals stack overflow (%d, max = %d)", localstack_used + c, LOCALSTACK_SIZE);

	for (i=0 ; i < c ; i++)
		localstack[localstack_used+i] = ((int *)pr_globals)[pr_ChkGlobalsF("PR_EnterFunction1", f->parm_start + i)];
	localstack_used += c;

// copy parameters
	o = f->parm_start;
	for (i=0 ; i<f->numparms ; i++)
	{
		for (j=0 ; j<f->parm_size[i] ; j++)
		{
			((int *)pr_globals)[pr_ChkGlobalsF("PR_EnterFunction2", o)] = ((int *)pr_globals)[pr_ChkGlobalsF("PR_EnterFunction3", OFS_PARM0 + i * 3 + j)];
			o++;
		}
	}

	pr_xfunction = f;
	return f->first_statement - 1;	// offset the s++
}

/*
====================
PR_LeaveFunction
====================
*/
int PR_LeaveFunction (void)
{
	int		i, c;

	if (pr_depth <= 0)
		PR_RunError ("PR_LeaveFunction: prog stack underflow (%d)", pr_depth);

// restore locals from the stack
	c = pr_xfunction->locals;
	localstack_used -= c;
	if (localstack_used < 0)
		PR_RunError ("PR_LeaveFunction: locals stack underflow (%d)", localstack_used);

	for (i=0 ; i < c ; i++)
		((int *)pr_globals)[pr_ChkGlobalsF("PR_LeaveFunction", pr_xfunction->parm_start + i)] = localstack[localstack_used+i];

// up stack
	pr_depth--;
	pr_xfunction = pr_stack[pr_depth].f;
	return pr_stack[pr_depth].s;
}

#ifndef NOISE
#define RUNAWAY	     5000000
#else
#define RUNAWAY	     100000
#endif

#define RUNAWAY_STEP (RUNAWAY / 5)

/*
====================
PR_ExecuteProgram
====================
*/
// LordHavoc: optimized
#define OPA ((eval_t *)&pr_globals[(unsigned short) st->a])
#define OPB ((eval_t *)&pr_globals[(unsigned short) st->b])
#define OPC ((eval_t *)&pr_globals[(unsigned short) st->c])
void PR_ExecuteProgram (func_t fnum, char *fname)
{
	eval_t		*a;
	dstatement_t	*st;
	dfunction_t	*f, *newf;
	edict_t		*ed;
	int		exitdepth;
	eval_t		*ptr;
	int		profile, startprofile;

	if (!fnum || fnum < 0 || fnum >= progs->numfunctions)
	{
		if (pr_global_struct->self)
			ED_Print (PROG_TO_EDICT("PR_ExecuteProgram1", pr_global_struct->self));

		if (!fnum)
			Host_Error ("PR_ExecuteProgram1: NULL '%s' function", fname);
		else
			Host_Error ("PR_ExecuteProgram1: invalid '%s' function (%d, max = %d)", fname, fnum, progs->numfunctions);
	}

	f = &pr_functions[fnum];

	pr_trace = false;

// make a stack frame
	exitdepth = pr_depth;
	pr_peakdepth = 0;

	st = &pr_statements[PR_EnterFunction (f)];
	startprofile = profile = 0;

while (1)
{
	st++;
	pr_xstatement = st - pr_statements;

	if (++profile >= RUNAWAY) //100000
		PR_RunError ("runaway loop error %d", RUNAWAY);

	if (profile > RUNAWAY_STEP - 1 && profile % RUNAWAY_STEP == 0)
	{
		Con_Printf ("PR_ExecuteProgram: runaway loop %d\n", (RUNAWAY - profile) / RUNAWAY_STEP);
		SCR_UpdateScreen (); // Force screen update
		S_ClearBuffer ();    // Avoid looping sounds
	}

#ifndef NOISE
	if (pr_trace)
		PR_PrintStatement (st);
#endif

	switch (st->op)
	{
	case OP_ADD_F:
		OPC->_float = OPA->_float + OPB->_float;
		break;
	case OP_ADD_V:
		OPC->vector[0] = OPA->vector[0] + OPB->vector[0];
		OPC->vector[1] = OPA->vector[1] + OPB->vector[1];
		OPC->vector[2] = OPA->vector[2] + OPB->vector[2];
		break;
	case OP_SUB_F:
		OPC->_float = OPA->_float - OPB->_float;
		break;
	case OP_SUB_V:
		OPC->vector[0] = OPA->vector[0] - OPB->vector[0];
		OPC->vector[1] = OPA->vector[1] - OPB->vector[1];
		OPC->vector[2] = OPA->vector[2] - OPB->vector[2];
		break;
	case OP_MUL_F:
		OPC->_float = OPA->_float * OPB->_float;
		break;
	case OP_MUL_V:
		OPC->_float = OPA->vector[0]*OPB->vector[0] + OPA->vector[1]*OPB->vector[1] + OPA->vector[2]*OPB->vector[2];
		break;
	case OP_MUL_FV:
		OPC->vector[0] = OPA->_float * OPB->vector[0];
		OPC->vector[1] = OPA->_float * OPB->vector[1];
		OPC->vector[2] = OPA->_float * OPB->vector[2];
		break;
	case OP_MUL_VF:
		OPC->vector[0] = OPB->_float * OPA->vector[0];
		OPC->vector[1] = OPB->_float * OPA->vector[1];
		OPC->vector[2] = OPB->_float * OPA->vector[2];
		break;
	case OP_DIV_F:
		OPC->_float = OPA->_float / OPB->_float;
		break;
	case OP_BITAND:
		OPC->_float = (int)OPA->_float & (int)OPB->_float;
		break;
	case OP_BITOR:
		OPC->_float = (int)OPA->_float | (int)OPB->_float;
		break;
	case OP_GE:
		OPC->_float = OPA->_float >= OPB->_float;
		break;
	case OP_LE:
		OPC->_float = OPA->_float <= OPB->_float;
		break;
	case OP_GT:
		OPC->_float = OPA->_float > OPB->_float;
		break;
	case OP_LT:
		OPC->_float = OPA->_float < OPB->_float;
		break;
	case OP_AND:
		OPC->_float = OPA->_float && OPB->_float;
		break;
	case OP_OR:
		OPC->_float = OPA->_float || OPB->_float;
		break;
	case OP_NOT_F:
		OPC->_float = !OPA->_float;
		break;
	case OP_NOT_V:
		OPC->_float = !OPA->vector[0] && !OPA->vector[1] && !OPA->vector[2];
		break;
	case OP_NOT_S:
		OPC->_float = !OPA->string || !(*pr_String("PR_ExecuteProgram1", OPA->string));
		break;
	case OP_NOT_FNC:
		OPC->_float = !OPA->function;
		break;
	case OP_NOT_ENT:
		OPC->_float = (PROG_TO_EDICT("PR_ExecuteProgram2", OPA->edict) == sv.edicts);
		break;
	case OP_EQ_F:
		OPC->_float = OPA->_float == OPB->_float;
		break;
	case OP_EQ_V:
		OPC->_float = (OPA->vector[0] == OPB->vector[0]) && (OPA->vector[1] == OPB->vector[1]) && (OPA->vector[2] == OPB->vector[2]);
		break;
	case OP_EQ_S:
		OPC->_float = !strcmp(pr_String("PR_ExecuteProgram2", OPA->string), pr_String("PR_ExecuteProgram3", OPB->string));
		break;
	case OP_EQ_E:
		OPC->_float = OPA->_int == OPB->_int;
		break;
	case OP_EQ_FNC:
		OPC->_float = OPA->function == OPB->function;
		break;
	case OP_NE_F:
		OPC->_float = OPA->_float != OPB->_float;
		break;
	case OP_NE_V:
		OPC->_float = (OPA->vector[0] != OPB->vector[0]) || (OPA->vector[1] != OPB->vector[1]) || (OPA->vector[2] != OPB->vector[2]);
		break;
	case OP_NE_S:
		OPC->_float = strcmp(pr_String("PR_ExecuteProgram4", OPA->string), pr_String("PR_ExecuteProgram5", OPB->string));
		break;
	case OP_NE_E:
		OPC->_float = OPA->_int != OPB->_int;
		break;
	case OP_NE_FNC:
		OPC->_float = OPA->function != OPB->function;
		break;

//==================
	case OP_STORE_F:
	case OP_STORE_ENT:
	case OP_STORE_FLD:		// integers
	case OP_STORE_S:
	case OP_STORE_FNC:		// pointers
		OPB->_int = OPA->_int;
		break;
	case OP_STORE_V:
		OPB->vector[0] = OPA->vector[0];
		OPB->vector[1] = OPA->vector[1];
		OPB->vector[2] = OPA->vector[2];
		break;

	case OP_STOREP_F:
	case OP_STOREP_ENT:
	case OP_STOREP_FLD:		// integers
	case OP_STOREP_S:
	case OP_STOREP_FNC:		// pointers
		ptr = (eval_t *)((byte *)sv.edicts + pr_ChkEdict ("PR_ExecuteProgram3", OPB->_int, sizeof(int)));
		ptr->_int = OPA->_int;
		break;
	case OP_STOREP_V:
		ptr = (eval_t *)((byte *)sv.edicts + pr_ChkEdict ("PR_ExecuteProgram4", OPB->_int, sizeof(float) * 3));
		ptr->vector[0] = OPA->vector[0];
		ptr->vector[1] = OPA->vector[1];
		ptr->vector[2] = OPA->vector[2];
		break;

	case OP_ADDRESS:
		ed = PROG_TO_EDICT("PR_ExecuteProgram5", OPA->edict);
#ifdef PARANOID
		NUM_FOR_EDICT(ed);		// make sure it's in range
#endif
		if (ed == (edict_t *)sv.edicts && sv.state == ss_active)
		{
			pr_ExtendedTrace = true;
			PR_RunError ("PR_ExecuteProgram: assignment to world entity");
		}

		OPC->_int = (byte *)((int *)&ed->v + pr_ChkEField ("PR_ExecuteProgram1", OPB->_int)) - (byte *)sv.edicts;
		break;

	case OP_LOAD_F:
	case OP_LOAD_FLD:
	case OP_LOAD_ENT:
	case OP_LOAD_S:
	case OP_LOAD_FNC:
		ed = PROG_TO_EDICT("PR_ExecuteProgram6", OPA->edict);
#ifdef PARANOID
		NUM_FOR_EDICT(ed);		// make sure it's in range
#endif
		OPC->_int = ((eval_t *)((int *)&ed->v + pr_ChkEField ("PR_ExecuteProgram2", OPB->_int)))->_int;
		break;

	case OP_LOAD_V:
		ed = PROG_TO_EDICT("PR_ExecuteProgram7", OPA->edict);
#ifdef PARANOID
		NUM_FOR_EDICT(ed);		// make sure it's in range
#endif
		a = (eval_t *)((int *)&ed->v + pr_ChkEField ("PR_ExecuteProgram3", OPB->_int));
		OPC->vector[0] = a->vector[0];
		OPC->vector[1] = a->vector[1];
		OPC->vector[2] = a->vector[2];
		break;

//==================

	case OP_IFNOT:
		if (!OPA->_int)
			st += st->b - 1;	// offset the s++
		break;

	case OP_IF:
		if (OPA->_int)
			st += st->b - 1;	// offset the s++
		break;

	case OP_GOTO:
		st += st->a - 1;	// offset the s++
		break;

	case OP_CALL0:
	case OP_CALL1:
	case OP_CALL2:
	case OP_CALL3:
	case OP_CALL4:
	case OP_CALL5:
	case OP_CALL6:
	case OP_CALL7:
	case OP_CALL8:
		pr_xfunction->profile += profile - startprofile;
		startprofile = profile;
		pr_argc = st->op - OP_CALL0;
		if (!OPA->function)
		{
			if ((st - 1)->op == OP_LOAD_FNC) // OK?
				ED_Print (ed); // Print owner edict, if any
			else if (pr_global_struct->self)
				ED_Print (PROG_TO_EDICT("PR_ExecuteProgram9", pr_global_struct->self));

			pr_ExtendedTrace = true;
			PR_RunError ("PR_ExecuteProgram2: NULL function");
		}

		newf = &pr_functions[pr_ChkFunction("PR_ExecuteProgram", OPA->function)];

		if (newf->first_statement < 0)
		{	// negative statements are built in functions
			int i = -newf->first_statement;
			if (i >= pr_numbuiltins)
			{
				pr_ExtendedTrace = true;
				PR_RunError ("PR_ExecuteProgram: bad builtin call number (%d, max = %d)", i, pr_numbuiltins);
			}
			pr_builtins[i] ();
			break;
		}

		st = &pr_statements[PR_EnterFunction (newf)];
		break;

	case OP_DONE:
	case OP_RETURN:
		pr_globals[OFS_RETURN] = pr_globals[(unsigned short) st->a];
		pr_globals[OFS_RETURN+1] = pr_globals[(unsigned short) st->a+1];
		pr_globals[OFS_RETURN+2] = pr_globals[(unsigned short) st->a+2];

		st = &pr_statements[PR_LeaveFunction ()];
		if (pr_depth == exitdepth)
		{
			// Check old limit
			if (pr_peakdepth >= 32)
			{
				Con_SafePrintf ("\002PR_ExecuteProgram: ");
				Con_SafePrintf ("excessive stack (%d, normal max = %d)\n", pr_peakdepth, 32 - 1);
			}

			return;		// all done
		}
		break;

	case OP_STATE:
		ed = PROG_TO_EDICT("PR_ExecuteProgram8", pr_global_struct->self);
#ifdef FPS_20
		ed->v.nextthink = pr_global_struct->time + 0.05;
#else
		ed->v.nextthink = pr_global_struct->time + 0.1;
#endif
		ed->v.frame = OPA->_float;
		ed->v.think = OPB->function;
		break;

	default:
		pr_ExtendedTrace = true;
		PR_RunError ("PR_ExecuteProgram: bad opcode %i", st->op);
	}
}

}
