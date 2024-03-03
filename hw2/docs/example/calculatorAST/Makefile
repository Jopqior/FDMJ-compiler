calc: lex.yy.o y.tab.o calc.o util.o printast.o
	cc -o calc calc.o printast.o util.o y.tab.o lex.yy.o

calc.o: calc.c calc.h util.h
	cc -g -c calc.c

printast.o: printast.c calc.h
	cc -g -c printast.c

lex.yy.c: calc.lex y.tab.h y.tab.c
	lex calc.lex
	
y.tab.c: calc.yacc calc.h 
	yacc -d calc.yacc
	
y.tab.h: calc.yacc
	yacc -d calc.yacc
	

util.o: util.c util.h
	cc -g -c util.c

lex.yy.o: lex.yy.c y.tab.h
	cc -g -c lex.yy.c
	
y.tab.o: y.tab.c
	cc -g -c y.tab.c

clean: 
	rm -f calc lex.yy.o lex.yy.c y.tab.o y.tab.c y.tab.h calc.o util.o printast.o
