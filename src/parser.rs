use std::collections::VecDeque;

use thiserror::Error;

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

    #[inline]
    fn advance(&mut self) -> Token {
        let tok = self.tokens.pop_front().unwrap();
        return tok;
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
    ($state:ident, $expected:expr) => {
        if *$state.current() != $expected {
            return Err(());
        }
        $state.advance();
    };
}

macro_rules! match_token {
    ($state:ident, $expected:path) => {
        match $state.advance() {
            $expected(ident) => ident,
            _ => return Err(()),
        }
    };
}

pub struct Parser;

impl Parser {
    pub fn parse(tokens: Vec<Token>) -> Result<AST, ParseError> {
        let mut state = ParserState::new(tokens);
        Parser::z(&mut state)
    }

    fn z(state: &mut ParserState) -> Result<AST, ParseError> {
        let expr = Parser::expression(state)?;
        let mut exprs = Parser::expressions(state)?;
        exprs.0.insert(0, expr);
        Ok(AST::Expressions(exprs))
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
            _ => Parser::operation(state),
        }
    }

    fn expression_if(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::If);

        let condition = Parser::expression(state)?;

        assert_token!(state, Token::Then);

        let true_body = Parser::expressions(state)?;
        let false_body = if state.current() == &Token::Else {
            state.advance();
            Some(Parser::expressions(state)?)
        } else {
            None
        };

        assert_token!(state, Token::End);

        let if_expr = AST::IfExpression(IfExpression {
            condition,
            true_body,
            false_body,
        });

        Ok(Expression(Box::new(if_expr)))
    }

    fn expression_for(state: &mut ParserState) -> Result<Expression, ParseError> {
        assert_token!(state, Token::For);
        state.advance();

        let identifier = match_token!(state, Token::Identifier);

        assert_token!(state, Token::Assignment);

        let start = Parser::expression(state)?;

        assert_token!(state, Token::Comma);

        let end = Parser::expression(state)?;

        let step = if state.current() == &Token::Comma {
            state.advance();
            Some(Parser::expression(state)?)
        } else {
            None
        };

        assert_token!(state, Token::Execute);

        let body = Parser::expressions(state)?;

        assert_token!(state, Token::End);

        let for_expr = AST::ForExpression(ForExpression {
            identifier,
            start,
            end,
            step,
            body,
        });

        Ok(Expression(Box::new(for_expr)))
    }

    fn expression_while(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
    fn expression_do_until(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
    fn expression_continue(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
    fn expression_break(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
    fn expression_trycatch(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
    fn expression_throw(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
    fn expression_function(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
    fn expression_return(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
    fn expression_include(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
    fn expression_run(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
    fn expression_read(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
    fn expression_print(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
    fn assignment(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
    fn operation(state: &mut ParserState) -> Result<Expression, ParseError> {
        todo!()
    }
}
