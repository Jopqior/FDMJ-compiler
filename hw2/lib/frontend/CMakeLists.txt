# flex和bison
find_package(FLEX)
find_package(BISON)
FLEX_TARGET(lexer lexer.lex ${PROJECT_BINARY_DIR}/include/frontend/lexer.yy.c)
BISON_TARGET(parser parser.yacc ${PROJECT_BINARY_DIR}/include/frontend/y.tab.c)
ADD_FLEX_BISON_DEPENDENCY(lexer parser)

aux_source_directory(. FRONTEND_SRCS)
add_library(frontend OBJECT 
  ${FLEX_lexer_OUTPUTS}
  ${BISON_parser_OUTPUTS}
)