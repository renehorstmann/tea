#ifndef RHC_ASSUME_H
#define RHC_ASSUME_H

// assert like function, that also uses formatting print to stderr.
// Calls raise(SIG_ABRT).
// If NDEBUG is defined, only msg will get displayed (without expression, file and line infos)
#define assume(EX, ...) \
(void)((EX) || (rhc_assume_impl_ (#EX, __FILE__, __LINE__, __VA_ARGS__),0))

void rhc_assume_impl_(const char *expression, const char *file, int line, const char *format, ...);

#endif //RHC_ASSUME_H
