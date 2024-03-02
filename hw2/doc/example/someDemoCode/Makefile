try0: try0.lex
	lex -o try0.c try0.lex
	cc -o try0 try0.c

util.o: util.c util.h
	cc -g -c util.c

t: t.lex t.yacc
	yacc t.yacc
	lex -o t.c t.lex
	cc -o t t.c y.tab.c 

clean: 
	rm -f try0 try0.c t t.c y.tab.*

