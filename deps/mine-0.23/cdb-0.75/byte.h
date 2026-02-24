#ifndef BYTE_H
#define BYTE_H


extern unsigned int byte_chr(const char *, unsigned int, int);
extern unsigned int byte_rchr(const char *, unsigned int, int);
extern void byte_zero(char *, unsigned int);
extern void byte_copyr(char *, unsigned int, char *);
extern void byte_copy(char *, unsigned int, char *);
extern int byte_diff(char *, unsigned int, char *);

#define byte_equal(s,n,t) (!byte_diff((s),(n),(t)))

#endif
