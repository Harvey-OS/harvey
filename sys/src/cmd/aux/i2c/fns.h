/* For a poor-mans function tracer (can add these with spatch) */
void __print_func_entry(const char *func, const char *file, int line);
void __print_func_exit(const char *func, const char *file, int line);
void set_printx(int mode);
#define print_func_entry() __print_func_entry(__FUNCTION__, __FILE__, __LINE__)
#define print_func_exit() __print_func_exit(__FUNCTION__, __FILE__, __LINE__)
