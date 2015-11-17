1,/int main(argc, argv)/s/return 0/print("PASS\\n");exits("PASS")/
/stdio.h/D
s/ printf/ print/g
s/ fprintf/ fprint/g
s/stderr/2/g
s/stdout/2/g
s/int main(argc, argv)/void main(argc, argv)/g
s/int  main/void main/g
