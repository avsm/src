/*	$OpenBSD: perl_extern.h,v 1.4 2001/01/29 01:58:47 niklas Exp $	*/

int perl_end __P((GS *));
int perl_init __P((SCR *));
int perl_screen_end __P((SCR*));
int perl_ex_perl __P((SCR*, CHAR_T *, size_t, recno_t, recno_t));
int perl_ex_perldo __P((SCR*, CHAR_T *, size_t, recno_t, recno_t));
#ifdef USE_SFIO
Sfdisc_t* sfdcnewnvi __P((SCR*));
#endif
