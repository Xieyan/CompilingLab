#include <stdio.h>

extern FILE * yydebug;

int main(int argc ,char** argv){
	if (argc<=1) return 1;
		
	FILE *f = fopen(argv[1], "r");
	if (!f){
	  perror(argv[1]);
	  return 1;	
	}
	yyrestart(f);

//  yydebug = 1;
  yyparse();
	return 0;
}
