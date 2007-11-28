int  print_license(const lame_global_flags* gfp, FILE* const fp, const char* ProgramName);
int  usage(const lame_global_flags* gfp, FILE* const fp, const char* ProgramName);
int  short_help(const lame_global_flags* gfp, FILE* const fp, const char* ProgramName);
int  long_help(const lame_global_flags* gfp, FILE* const fp, const char* ProgramName, int lessmode);
int  display_bitrates(FILE* const fp);

int  parse_args(lame_global_flags* gfp, int argc, char** argv);
void print_config(lame_global_flags* gfp);
