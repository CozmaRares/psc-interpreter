use std::collections::VecDeque;

//use thiserror::Error;

use crate::{nodes::*, tokens::Token};

struct ParserState {
    tokens: VecDeque<Token>,
}

impl ParserState {
    #[inline]
    fn new(tokens: Vec<Token>) -> Self {
        Self {
            tokens: tokens.into(),
        }
    }

    #[inline]
    fn is_at_end(&self) -> bool {
        self.tokens.is_empty()
    }

    // TODO: remove Token::None and instead return an error
    #[inline]
    fn advance(&mut self) -> Token {
        self.tokens.pop_front().unwrap_or(Token::None)
    }

    fn current(&self) -> &Token {
        self.tokens.back().unwrap_or(&Token::None)
    }
}

//#[derive(Debug, Error)]
//pub struct ParseError {
//    expected: Token,
//    actual: Token,
//}
type ParseError = ();
// TODO: ^

//impl std::fmt::Display for ParseError {
//    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
//        writeln!(f, "Expected: {}", self.expected)?;
//        writeln!(f, "Actual: {}", self.actual)?;
//        Ok(())
//    }
//}

macro_rules! assert_token {
    ($state: ident, $expected: path) => {
        if *$state.current() != $expected {
            return Err(());
        }
        $state.advance();
    };
}

macro_rules! match_token {
    ($state: ident, $expected: path) => {
        match $state.advance() {
            $expected(ident) => ident,
            _ => return Err(()),
        }
    };
}

macro_rules! optional_token {
    ($state: ident, $expected: path, $scope: block) => {
        if *$state.current() == $expected {
            $state.advance();
            let _ret = $scope;
            Some(_ret)
        } else {
            None
        }
    };
}

macro_rules! while_token {
    ($state: ident, $expected: path, $scope: block) => {
        while *$state.current() == $expected {
            $state.advance();
            $scope
        }
    };
}

pub struct Parser;

impl Parser {
    pub fn parse(tokens: Vec<Token>) -> Result<Ast, ParseError> {
        let mut state = ParserState::new(tokens);
        Parser::z(&mut state)
    }

    fn z(state: &mut ParserState) -> Result<Ast, ParseError> {
        let expr = Parser::expression(state)?;
        let mut exprs = Parser::expressions(state)?;
        exprs.0.insert(0, expr);
        Ok(Ast::Expressions(exprs))
    }

    fn expressions(state: &mut ParserState) -> Result<Expressions, ParseError> {
        let mut exprs = Vec::new();
        while !state.is_at_end() {
            if state.current() != &Token::Endline {
                break;
            }

            let expr = Parser::expression(state)?;
            exprs.push(expr);
        }

        Ok(Expressions(exprs))
    }

    fn expression(state: &mut ParserState) -> Result<Expression, ParseError> {
        match state.current() {
            Token::If => Parser::expression_if(state),
            Token::For => Parser::expression_for(state),
            Token::While => Parser::expression_while(state),
            Token::Do => Parser::expression_do_until(state),
            Token::Continue => Parser::expression_continue(state),
            Token::Break => Parser::expression_break(state),
            Token::Try => Parser::expression_trycatch(state),
            Token::Throw => Parser::expression_throw(state),
            Token::Function => Parser::expression_function(state),
            Token::Return => Parser::expression_return(state),
            Token::Include => Parser::expression_include(state),
            Token::Run => Parser::expression_run(state),
            Token::Read => Parser::expression_read(state),
            Token::Print => Parser::expression_print(state),
            Token::Let => Parser::assignment(state),
            _ => Parser::logical_operation(state),
        }
    }

    fn expression_if(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::If);
        let condition = Parser::expression(state)?;
        assert_token!(state, Token::Then);
        let true_body = Parser::expressions(state)?;
        let false_body = optional_token!(state, Token::Else, { Parser::expressions(state)? });
        assert_token!(state, Token::End);

        let if_expr = Ast::If(IfExpression {
            condition,
            true_body,
            false_body,
        });

        Ok(Expression(Box::new(if_expr)))
    }

    fn expression_for(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::For);
        let identifier = match_token!(state, Token::Identifier);
        assert_token!(state, Token::Assignment);
        let start = Parser::expression(state)?;
        assert_token!(state, Token::Comma);
        let end = Parser::expression(state)?;
        let step = optional_token!(state, Token::Comma, { Parser::expression(state)? });
        assert_token!(state, Token::Execute);
        let body = Parser::expressions(state)?;
        assert_token!(state, Token::End);

        let for_expr = Ast::For(ForExpression {
            identifier,
            start,
            end,
            step,
            body,
        });
        Ok(Expression(Box::new(for_expr)))
    }

    fn expression_while(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::While);
        let condition = Parser::expression(state)?;
        assert_token!(state, Token::Execute);
        let body = Parser::expressions(state)?;
        assert_token!(state, Token::End);

        let while_expr = Ast::While(WhileExpression { condition, body });
        Ok(Expression(Box::new(while_expr)))
    }
    fn expression_do_until(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::Do);
        let body = Parser::expressions(state)?;
        assert_token!(state, Token::Until);
        let condition = Parser::expression(state)?;
        assert_token!(state, Token::End);

        let do_until_expr = Ast::DoUntil(DoUntilExpression { condition, body });
        Ok(Expression(Box::new(do_until_expr)))
    }
    fn expression_continue(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::Continue);
        Ok(Expression(Box::new(Ast::Continue)))
    }
    fn expression_break(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::Break);
        Ok(Expression(Box::new(Ast::Break)))
    }
    fn expression_trycatch(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::Try);
        let try_body = Parser::expressions(state)?;
        assert_token!(state, Token::Catch);
        let catch_identifier = match_token!(state, Token::Identifier);
        let catch_body = Parser::expressions(state)?;
        assert_token!(state, Token::End);

        let try_catch_expr = Ast::TryCatch(TryCatchExpression {
            try_body,
            catch_identifier,
            catch_body,
        });
        Ok(Expression(Box::new(try_catch_expr)))
    }
    fn expression_throw(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::Throw);
        let expression = Parser::expression(state)?;

        let throw_expr = Ast::Throw(expression);
        Ok(Expression(Box::new(throw_expr)))
    }
    fn expression_function(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::Function);
        let identifier = match_token!(state, Token::Identifier);
        assert_token!(state, Token::ParenLeft);

        let mut parameters = Vec::new();
        parameters.push(match_token!(state, Token::Identifier));

        while_token!(state, Token::Comma, {
            parameters.push(match_token!(state, Token::Identifier));
        });

        assert_token!(state, Token::ParenRight);
        let body = Parser::expressions(state)?;
        assert_token!(state, Token::End);

        let function_def = Ast::FunctionDefinition(FunctionDefinition {
            identifier,
            parameters,
            body,
        });
        Ok(Expression(Box::new(function_def)))
    }
    fn expression_return(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::Return);
        let expression = Parser::expression(state)?;

        let return_expr = Ast::Return(expression);
        Ok(Expression(Box::new(return_expr)))
    }
    fn expression_include(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::Include);
        let string = match_token!(state, Token::String);

        let include_expr = Ast::Include(string);
        Ok(Expression(Box::new(include_expr)))
    }
    fn expression_run(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::Run);
        let string = match_token!(state, Token::String);

        let run_expr = Ast::Run(string);
        Ok(Expression(Box::new(run_expr)))
    }
    fn expression_read(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::Read);
        let file = optional_token!(state, Token::Less, {
            let file = match_token!(state, Token::Identifier);
            assert_token!(state, Token::Greater);
            file
        });
        let mut identifiers = Vec::new();
        identifiers.push(match_token!(state, Token::Identifier));

        while_token!(state, Token::Comma, {
            identifiers.push(match_token!(state, Token::Identifier));
        });

        let read_expr = Ast::Read(ReadExpression { file, identifiers });
        Ok(Expression(Box::new(read_expr)))
    }
    fn expression_print(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::Print);
        let file = optional_token!(state, Token::Less, {
            let file = match_token!(state, Token::Identifier);
            assert_token!(state, Token::Greater);
            file
        });
        let mut expressions = Vec::new();
        expressions.push(Parser::expression(state)?);

        while_token!(state, Token::Comma, {
            expressions.push(Parser::expression(state)?);
        });

        let print_expr = Ast::Print(PrintExpression { file, expressions });
        Ok(Expression(Box::new(print_expr)))
    }
    fn assignment(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::Let);
        let identifier = match_token!(state, Token::Identifier);

        let index_access =
            optional_token!(state, Token::BracketLeft, { Parser::index_access(state)? })
                .unwrap_or_default();

        assert_token!(state, Token::Assignment);
        let expression = Parser::expression(state)?;

        let assignment_expr = Ast::Assignment(AssignmentExpression {
            identifier,
            index_access,
            expression,
        });
        Ok(Expression(Box::new(assignment_expr)))
    }

    fn logical_operation(state: &mut ParserState) -> Result<Expression, ParseError> {
        let mut expr = Parser::comparison(state)?;

        while *state.current() == Token::And || *state.current() == Token::Or {
            let op = match state.advance() {
                Token::And => LogicalOperator::And,
                Token::Or => LogicalOperator::Or,
                _ => unreachable!(),
            };
            let right = Parser::comparison(state)?;

            let e = Ast::LogicalOperation(LogicalOperationExpression {
                operator: op,
                left: expr,
                right,
            });
            expr = Expression(Box::new(e));
        }
        Ok(expr)
    }

    fn comparison(state: &mut ParserState) -> Result<Expression, ParseError> {
        let mut expr = Parser::arith(state)?;

        while *state.current() == Token::Equals
            || *state.current() == Token::Less
            || *state.current() == Token::LessEqual
            || *state.current() == Token::Greater
            || *state.current() == Token::GreaterEqual
            || *state.current() == Token::Different
        {
            let op = match state.advance() {
                Token::Equals => ComparisonOperator::Equal,
                Token::Less => ComparisonOperator::Less,
                Token::LessEqual => ComparisonOperator::LessEqual,
                Token::Greater => ComparisonOperator::Greater,
                Token::GreaterEqual => ComparisonOperator::GreaterEqual,
                Token::Different => ComparisonOperator::Different,
                _ => unreachable!(),
            };
            let right = Parser::arith(state)?;

            let e = Ast::ComparisonOperation(ComparisonExpression {
                operator: op,
                left: expr,
                right,
            });
            expr = Expression(Box::new(e));
        }
        Ok(expr)
    }

    fn arith(state: &mut ParserState) -> Result<Expression, ParseError> {
        let mut expr = Parser::arith2(state)?;

        while *state.current() == Token::Plus || *state.current() == Token::Minus {
            let op = match state.advance() {
                Token::Plus => ArithmeticOperator::Add,
                Token::Minus => ArithmeticOperator::Subtract,
                _ => unreachable!(),
            };
            let right = Parser::arith2(state)?;

            let e = Ast::ArithmeticOperation(ArithmeticExpression {
                operator: op,
                left: expr,
                right,
            });
            expr = Expression(Box::new(e));
        }
        Ok(expr)
    }

    fn arith2(state: &mut ParserState) -> Result<Expression, ParseError> {
        let mut expr = Parser::factor(state)?;

        while *state.current() == Token::Star
            || *state.current() == Token::Slash
            || *state.current() == Token::Percent
        {
            let op = match state.advance() {
                Token::Star => ArithmeticOperator2::Multiply,
                Token::Slash => ArithmeticOperator2::Divide,
                Token::Percent => ArithmeticOperator2::Modulo,
                _ => unreachable!(),
            };
            let right = Parser::arith2(state)?;

            let e = Ast::ArithmeticOperation2(ArithmeticOperation2 {
                operator: op,
                left: expr,
                right,
            });
            expr = Expression(Box::new(e));
        }
        Ok(expr)
    }

    fn factor(state: &mut ParserState) -> Result<Expression, ParseError> {
        let base = Parser::base(state)?;

        match state.current() {
            Token::BracketLeft => {
                let index = Parser::index_access(state)?;
                let fact = Ast::Factor(FactorExpression::IndexAccess { base, index });
                Ok(Expression(Box::new(fact)))
            }
            Token::ParenLeft => {
                let arguments = Parser::fn_call(state)?;
                let fact = Ast::Factor(FactorExpression::FnCall { base, arguments });
                Ok(Expression(Box::new(fact)))
            }
            _ => {
                let base = Ast::Base(base);
                Ok(Expression(Box::new(base)))
            }
        }
    }

    fn fn_call(state: &mut ParserState) -> Result<Vec<Expression>, ParseError> {
        assert_token!(state, Token::ParenLeft);

        let mut arguments = Vec::new();

        if state.current() == &Token::ParenRight {
            state.advance();
            return Ok(arguments);
        }

        arguments.push(Parser::expression(state)?);

        while_token!(state, Token::Comma, {
            arguments.push(Parser::expression(state)?);
        });
        assert_token!(state, Token::ParenRight);
        Ok(arguments)
    }

    fn index_access(state: &mut ParserState) -> Result<Vec<Expression>, ParseError> {
        let mut indices = Vec::new();
        while_token!(state, Token::BracketLeft, {
            indices.push(Parser::expression(state)?);
            assert_token!(state, Token::BracketRight);
        });
        Ok(indices)
    }

    fn base(state: &mut ParserState) -> Result<BaseExpression, ParseError> {
        match state.advance() {
            Token::Number(n) => Ok(BaseExpression::Number(n)),
            Token::Char(c) => Ok(BaseExpression::Char(c)),
            Token::String(s) => Ok(BaseExpression::String(s)),
            Token::Identifier(i) => Ok(BaseExpression::Identifier(i)),
            Token::ParenLeft => {
                let expr = Parser::expression(state)?;
                assert_token!(state, Token::ParenRight);
                Ok(BaseExpression::Expression(expr))
            }
            Token::BracketLeft => Ok(Parser::array(state)?),
            Token::CurlyLeft => Ok(Parser::dictionary(state)?),
            Token::Plus | Token::Minus => Ok(Parser::unary(state)?),
            // FIX: error reporting might be wrong here
            // might have to use current instead of advance
            _ => Err(()),
        }
    }

    fn array(state: &mut ParserState) -> Result<BaseExpression, ParseError> {
        assert_token!(state, Token::BracketLeft);

        let mut elements = Vec::new();

        if state.current() == &Token::BracketRight {
            state.advance();
            return Ok(BaseExpression::Array(elements));
        }

        elements.push(Parser::expression(state)?);

        while_token!(state, Token::Comma, {
            elements.push(Parser::expression(state)?);
        });

        assert_token!(state, Token::BracketRight);

        Ok(BaseExpression::Array(elements))
    }

    fn dictionary(state: &mut ParserState) -> Result<BaseExpression, ParseError> {
        assert_token!(state, Token::CurlyLeft);

        let mut pairs = Vec::new();

        if state.current() == &Token::CurlyRight {
            state.advance();
            return Ok(BaseExpression::Dictionary(pairs));
        }

        let mut key;
        let mut value;

        key = Parser::expression(state)?;
        assert_token!(state, Token::Colon);
        value = Parser::expression(state)?;
        pairs.push((key, value));

        while_token!(state, Token::Comma, {
            key = Parser::expression(state)?;
            assert_token!(state, Token::Colon);
            value = Parser::expression(state)?;
            pairs.push((key, value));
        });

        assert_token!(state, Token::CurlyRight);

        Ok(BaseExpression::Dictionary(pairs))
    }

    fn unary(state: &mut ParserState) -> Result<BaseExpression, ParseError> {
        match state.advance() {
            Token::Plus => Ok(BaseExpression::Unary {
                operator: UnaryOperator::Plus,
                base: Box::new(Parser::base(state)?),
            }),
            Token::Minus => Ok(BaseExpression::Unary {
                operator: UnaryOperator::Minus,
                base: Box::new(Parser::base(state)?),
            }),
            // FIX: same thing as in base
            _ => Err(()),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::lexer::Lexer;

    macro_rules! parse {
        ($input: expr) => {{
            let _tokens = Lexer::tokenize($input).unwrap();
            Parser::parse(_tokens)
        }};
    }

    mod r#if {
        use super::*;

        #[test]
        fn simple() {
            let parsed = parse!(
                r#"if x then
                    print "not zero"
                end"#
            )
            .unwrap();
            todo!()
        }

        // TODO: change grammar to allow this
        #[test]
        fn one_line() {
            let parsed = parse!("if x then x end").unwrap();
            todo!()
        }

        #[test]
        fn with_else() {
            let parsed = parse!(
                r#"if x then
                    print "not zero"
                else
                    print "zero"
                end"#
            );
            todo!()
        }

        #[test]
        fn else_if() {
            let parsed = parse!(
                r#"if x then
                    print "x not zero"
                else
                    if y then
                        print "x zero, y not zero"
                    else
                        print "both zero"
                    end
                end"#
            );
            todo!()
        }

        #[test]
        fn if_if() {
            let parsed = parse!(
                r#"if x then
                    if y then
                        print "both not zero"
                    else
                        print "x not zero, y zero"
                    end
                else
                    print "x zero"
                end"#
            );
            todo!()
        }

        #[test]
        fn missing_then() {
            let parsed = parse!(
                r#"if x
                    print "positive"
                end"#
            );
            todo!()
        }

        #[test]
        fn missing_end() {
            let parsed = parse!(
                r#"if x then
                    print "positive"
                "#
            );
            todo!()
        }

        #[test]
        fn expression_after_end() {
            let parsed = parse!(
                r#"if x
                    print "positive"
                end 1"#
            );
            todo!()
        }
    }

    mod r#for {
        use super::*;

        #[test]
        fn without_skip() {
            let parsed = parse!(
                r#"for i <- 1, 5 execute
                    print i
                end"#
            );
            todo!()
        }

        #[test]
        fn with_skip() {
            let parsed = parse!(
                r#"for i <- 0, 10, 2 execute
                    print i
                end"#
            );
            todo!()
        }

        #[test]
        fn missing_execute() {
            let parsed = parse!(
                r#"for i <- 0, 10, 2
                    print i
                end"#
            );
            todo!()
        }

        #[test]
        fn missing_start() {
            let parsed = parse!(
                r#"for i <- , 10 execute
                    print i
                end"#
            );
            todo!()
        }

        #[test]
        fn empty_body() {
            let parsed = parse!(
                r#"for i <- 0, 10 execute
                end"#
            );
            todo!()
        }
    }

    mod r#while {
        use super::*;

        #[test]
        fn simple() {
            let parsed = parse!(
                r#"while x execute
                    x
                end"#
            );
            todo!()
        }

        // TODO: change grammar to allow this
        #[test]
        fn one_line() {
            let parsed = parse!("while x execute x end");
            todo!()
        }

        #[test]
        fn missing_execute() {
            let parsed = parse!(
                r#"while x
                    x
                end"#
            );
            todo!()
        }
    }

    #[test]
    fn r#do_until() {
        let parsed = parse!(
            r#"do
                x
            until x"#
        );
        todo!()
    }

    #[test]
    fn r#continue() {
        let parsed = parse!("continue");
        todo!()
    }

    #[test]
    fn r#break() {
        let parsed = parse!("break");
        todo!()
    }

    mod r#trycatch {
        use super::*;

        #[test]
        fn simple() {
            let parsed = parse!(
                r#"try
                    "trying"
                catch err
                    err
                end"#
            );
            todo!()
        }

        #[test]
        fn empty_catch() {
            let parsed = parse!(
                r#"try
                    "trying"
                catch err
                end"#
            );
            todo!()
        }

        #[test]
        fn missing_catch_identifier() {
            let parsed = parse!(
                r#"try
                    print "trying"
                catch
                    1
                end"#
            );
            todo!()
        }
    }

    mod throw {
        use super::*;

        #[test]
        fn string() {
            let parsed = parse!(r#"throw "error""#).unwrap();
            todo!()
        }

        #[test]
        fn number() {
            let parsed = parse!("throw 1").unwrap();
            todo!()
        }

        #[test]
        fn expression() {
            let parsed = parse!("throw 1 + 2").unwrap();
            todo!()
        }
    }

    mod function {
        use super::*;

        #[test]
        fn no_params() {
            let parsed = parse!(
                r#"function f()
                    print "hello"
                end"#
            );
            todo!()
        }

        #[test]
        fn with_params() {
            let parsed = parse!(
                r#"function f(x, y)
                    print x, y
                end"#
            );
            todo!()
        }

        #[test]
        fn empty_body() {
            let parsed = parse!(
                r#"function f()
                end"#
            );
            todo!()
        }
    }

    mod r#return {
        use super::*;

        #[test]
        fn string() {
            let parsed = parse!(r#"return "error""#).unwrap();
            todo!()
        }

        #[test]
        fn number() {
            let parsed = parse!("return 1").unwrap();
            todo!()
        }

        #[test]
        fn expression() {
            let parsed = parse!("return 1 + 2").unwrap();
            todo!()
        }
    }

    mod include {
        use super::*;

        #[test]
        fn simple() {
            let parsed = parse!(r#"include "file.txt""#).unwrap();
            todo!()
        }

        #[test]
        fn include_number() {
            let parsed = parse!("include 1");
            todo!()
        }
    }

    mod run {
        use super::*;

        #[test]
        fn simple() {
            let parsed = parse!(r#"run "file.txt""#).unwrap();
            todo!()
        }

        #[test]
        fn run_number() {
            let parsed = parse!("run 1");
            todo!()
        }
    }

    mod read {
        use super::*;

        #[test]
        fn without_file() {
            let parsed = parse!("read var1, var2, var3");
            todo!()
        }

        #[test]
        fn with_file() {
            let parsed = parse!("read <f> var1, var2");
            todo!()
        }

        #[test]
        fn missing_first_identifier() {
            let parsed = parse!("read , var2");
            todo!()
        }

        #[test]
        fn trailing_comma() {
            let parsed = parse!("read var1, var2,");
            todo!()
        }
    }

    mod print {
        use super::*;

        #[test]
        fn without_file() {
            let parsed = parse!("print 123, 456");
            todo!()
        }

        #[test]
        fn with_file() {
            let parsed = parse!("print <stdout> 123, 456");
            todo!()
        }

        #[test]
        fn missing_expressions() {
            let parsed = parse!("print <stdout>");
            todo!()
        }

        #[test]
        fn trailing_comma() {
            let parsed = parse!("print 123, ");
            todo!()
        }
    }

    mod assignment {
        use super::*;

        #[test]
        fn to_variable() {
            let parsed = parse!("let x <- 42");
            todo!()
        }

        #[test]
        fn to_index() {
            let parsed = parse!("let arr[3] <- 99");
            todo!()
        }

        #[test]
        fn missing_let() {
            let parsed = parse!("x <- 42");
            todo!()
        }

        #[test]
        fn missing_arrow() {
            let parsed = parse!("let x 42");
            todo!()
        }

        #[test]
        fn missing_expression() {
            let parsed = parse!("let x <- ");
            todo!()
        }
    }

    mod logical_operation {
        use super::*;

        #[test]
        fn comparison() {
            let parsed = parse!("a < b");
            todo!()
        }

        mod operators {
            use super::*;

            #[test]
            fn and() {
                let parsed = parse!("a < b and c < d");
                todo!()
            }

            #[test]
            fn or() {
                let parsed = parse!("a < b or c < d");
                todo!()
            }
        }

        #[test]
        fn mixed_operators() {
            let parsed = parse!("a < b or c < d and e < f");
            todo!()
        }

        #[test]
        fn trailing_operator() {
            let parsed = parse!("a < b and");
            todo!()
        }

        #[test]
        fn missing_operator() {
            let parsed = parse!("a < b c < d");
            todo!()
        }
    }

    mod comparison {
        use super::*;

        #[test]
        fn arithmetic() {
            let parsed = parse!("a + b");
            todo!()
        }

        mod operators {
            use super::*;

            #[test]
            fn equality() {
                let parsed = parse!("a = b");
                todo!()
            }

            #[test]
            fn less() {
                let parsed = parse!("a < b");
                todo!()
            }

            #[test]
            fn less_equal() {
                let parsed = parse!("a <= b");
                todo!()
            }

            #[test]
            fn greater() {
                let parsed = parse!("a > b");
                todo!()
            }

            #[test]
            fn greater_equal() {
                let parsed = parse!("a >= b");
                todo!()
            }

            #[test]
            fn different() {
                let parsed = parse!("a <> b");
                todo!()
            }
        }

        #[test]
        fn multiple_comparisons() {
            let parsed = parse!("x <= y = z");
            todo!()
        }

        #[test]
        fn trailing_operator() {
            let parsed = parse!("a <");
            todo!()
        }

        #[test]
        fn missing_operator() {
            let parsed = parse!("a b");
            todo!()
        }
    }

    mod arith1 {
        use super::*;

        #[test]
        fn arith2() {
            let parsed = parse!("a * 2");
            todo!()
        }

        mod operators {
            use super::*;

            #[test]
            fn addition() {
                let parsed = parse!("a + b");
                todo!()
            }

            #[test]
            fn subtraction() {
                let parsed = parse!("a - b");
                todo!()
            }
        }

        #[test]
        fn multiple_operations() {
            let parsed = parse!("a - b + c");
            todo!()
        }

        #[test]
        fn trailing_operator() {
            let parsed = parse!("a +");
            todo!()
        }

        #[test]
        fn missing_operator() {
            let parsed = parse!("a b");
            todo!()
        }
    }

    mod arith2 {
        use super::*;

        #[test]
        fn single_factor() {
            let parsed = parse!("a");
            todo!()
        }

        mod operators {
            use super::*;

            #[test]
            fn multiplication() {
                let parsed = parse!("a * b");
                todo!()
            }

            #[test]
            fn division() {
                let parsed = parse!("a / b");
                todo!()
            }

            #[test]
            fn modulo() {
                let parsed = parse!("a % b");
                todo!()
            }
        }

        #[test]
        fn mixed_operations() {
            let parsed = parse!("a / b * c");
            todo!()
        }

        #[test]
        fn trailing_operator() {
            let parsed = parse!("a *");
            todo!()
        }

        #[test]
        fn missing_operator() {
            let parsed = parse!("a b");
            todo!()
        }
    }

    mod factor {
        use super::*;

        mod fn_call {
            use super::*;

            #[test]
            fn fn_zero_args() {
                let parsed = parse!("foo()");
                todo!()
            }

            #[test]
            fn fn_one_arg() {
                let parsed = parse!("foo(42)");
                todo!()
            }

            #[test]
            fn fn_multiple_args() {
                let parsed = parse!("foo(1, 2, 3)");
                todo!()
            }

            #[test]
            fn fn_missing_closing_paren() {
                let parsed = parse!("foo(42");
                todo!()
            }

            #[test]
            fn extra_comma() {
                let parsed = parse!("foo(1, 2,)");
                todo!()
            }
        }

        mod index {
            use super::*;

            #[test]
            fn index_single() {
                let parsed = parse!("arr[5]");
                todo!()
            }

            #[test]
            fn index_double() {
                let parsed = parse!("matrix[2][3]");
                todo!()
            }

            #[test]
            fn missing_closing_bracket() {
                let parsed = parse!("arr[5");
                todo!()
            }
        }

        #[test]
        fn base() {
            let parsed = parse!("variable");
            todo!()
        }

        #[test]
        fn adjacent_suffixes() {
            let parsed = parse!("foo()[bar]");
            assert!(parsed.is_err());
        }
    }

    mod base {
        use super::*;

        #[test]
        fn number() {
            let parsed = parse!("12345");
            todo!();
        }

        #[test]
        fn char() {
            let parsed = parse!("'a'");
            todo!();
        }

        #[test]
        fn string() {
            let parsed = parse!(r#""hello""#);
            todo!();
        }

        #[test]
        fn base_identifier() {
            let parsed = parse!("variableName");
            todo!();
        }

        #[test]
        fn base_parenthesized_expression() {
            let parsed = parse!("(a + b)");
            todo!();
        }

        #[test]
        fn base_array() {
            let parsed = parse!("[1, 2, 3]");
            todo!();
        }

        #[test]
        fn base_dictionary() {
            let parsed = parse!(r#"{"key": "value", "foo": 42}"#);
            todo!();
        }

        mod unary {
            use super::*;

            #[test]
            fn plus() {
                let parsed = parse!("+1");
                todo!()
            }

            #[test]
            fn minus() {
                let parsed = parse!("-1");
                todo!()
            }
        }

        #[test]
        fn unclosed_parenthesized_expression() {
            let parsed = parse!("(a + b");
            assert!(parsed.is_err());
        }

        #[test]
        fn invalid_array_syntax() {
            let parsed = parse!("[1, 2, 3");
            assert!(parsed.is_err());
        }

        #[test]
        fn invalid_dictionary_syntax() {
            let parsed = parse!("{\"key\": \"value\", \"foo\" 42}");
            assert!(parsed.is_err());
        }

        #[test]
        fn invalid_unary_expression() {
            let parsed = parse!("+");
            assert!(parsed.is_err());
        }
    }
}
