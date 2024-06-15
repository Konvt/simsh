<statement> ::= <expression> <statement_extension>
              | (\d*)>{1,2} <statement_extension>
              | '(' <inner_statement> ')' <statement_extension>
              | '!' <logical_not> <statement_extension>
              | '\n'
              | EOF

<statement_extension> ::= <connector> <statement>
                     | <redirection> <statement_extension>
                     | ';' <statement>
                     | '\n'
                     | EOF

<inner_statement> ::= <expression> <inner_statement_extension>
                     | (\d*)>{1,2} <inner_statement_extension>
                     | '!' <logical_not> <inner_statement_extension>
                     | '(' <inner_statement> ')' <inner_statement_extension>

<inner_statement_extension> ::= <connector> <inner_statement>
                              | <redirection> <inner_statement_extension>
                              | ';' <inner_statement>
                              | ')' // 该符号不会在此语法解析阶段被消耗

<logical_not> ::= <expression>
                | '(' <inner_statement> ')'

<connector> ::= '&&'
              | '||'
              | '|'

<redirection> ::= (\d*)&> <expression>
                | (\d*)>{1,2} <expression>
                | '<' <expression>

<expression> ::= [A-Za-z0-9_., +-*/@:]+
