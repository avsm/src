/* This is tested by i386gas.h.  */
#define YES_UNDERSCORES

#include <i386/gstabs.h>

/* Remove when the math-emulator is fixed.  */
/* XXX i386 TARGET_DEFAULT/TARGET_CPU_DEFAULT is weird...
   This must be defined as TARGET_DEFAULT, and after including gstabs.h
	to override unix.h TARGET_DEFAULT definition.
	In effect, this adds MASK_NO_FANCY_MATH_387 to the usual definition.  */
#undef TARGET_DEFAULT
#define TARGET_DEFAULT \
	(MASK_NO_FANCY_MATH_387 | MASK_80387 | MASK_IEEE_FP | MASK_FLOAT_RETURNS)

/* Get perform_* macros to build libgcc.a.  */
#include <i386/perform.h>

#define OBSD_OLD_GAS
/* Get generic OpenBSD definitions.  */
#include <openbsd.h>

#define CPP_PREDEFINES "-Dunix -Di386 -D__OpenBSD__ -Asystem(unix) -Asystem(OpenBSD) -Acpu(i386) -Amachine(i386)"

#undef SIZE_TYPE
#define SIZE_TYPE "unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"

#undef WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef WCHAR_UNSIGNED
#define WCHAR_UNSIGNED 0

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 32

/*	$OpenBSD: openbsd.h,v 1.7 1999/04/01 21:24:41 espie Exp $	*/

/* There are conflicting reports about whether this system uses
   a different assembler syntax.  wilson@cygnus.com says # is right.  */
#undef COMMENT_BEGIN
#define COMMENT_BEGIN "#"

#undef ASM_APP_ON
#define ASM_APP_ON "#APP\n"

#undef ASM_APP_OFF
#define ASM_APP_OFF "#NO_APP\n"

/* The following macros are stolen from i386v4.h */
/* These have to be defined to get PIC code correct */

/* This is how to output an element of a case-vector that is relative.
   This is only used for PIC code.  See comments by the `casesi' insn in
   i386.md for an explanation of the expression this outputs. */

#undef ASM_OUTPUT_ADDR_DIFF_ELT
#define ASM_OUTPUT_ADDR_DIFF_ELT(FILE, VALUE, REL) \
  fprintf (FILE, "\t.long _GLOBAL_OFFSET_TABLE_+[.-%s%d]\n", LPREFIX, VALUE)

/* Indicate that jump tables go in the text section.  This is
   necessary when compiling PIC code.  */

#define JUMP_TABLES_IN_TEXT_SECTION

/* Don't default to pcc-struct-return, because gcc is the only compiler, and
   we want to retain compatibility with older gcc versions.  */
#define DEFAULT_PCC_STRUCT_RETURN 0

/* Profiling routines, partially copied from i386/osfrose.h.  */

/* Redefine this to use %eax instead of %edx.  */
#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER(FILE, LABELNO)  \
{									\
  if (flag_pic)								\
    {									\
      fprintf (FILE, "\tcall mcount@PLT\n");				\
    }									\
  else									\
    {									\
      fprintf (FILE, "\tcall mcount\n");				\
    }									\
}
