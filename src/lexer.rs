use thiserror::Error;

use crate::tokens::Token;

struct LexerState<'a> {
    input: &'a str,
    cursor: usize,
    bol: usize, // beginning of line
    line_number: usize,
}

impl<'a> LexerState<'a> {
    #[inline]
    fn new(input: &'a str) -> Self {
        Self {
            input,
            cursor: 0,
            bol: 0,
            line_number: 1,
        }
    }

    #[inline]
    fn is_at_end(&self) -> bool {
        self.cursor >= self.input.len()
    }

    #[inline]
    fn advance(&mut self) {
        if self.current() == '\n' {
            self.bol = self.cursor;
            self.line_number += 1;
        }

        self.cursor += 1;
    }

    #[inline]
    fn current(&self) -> char {
        self.input.chars().nth(self.cursor).unwrap_or('\0')
    }
}

#[derive(Debug, Error)]
pub enum LexErrorKind {
    #[error("Invalid number with multiple decimal points")]
    MultipleDecimalPoints,

    #[error("Number could not be parsed\n{0}")]
    InvalidNumber(#[from] std::num::ParseFloatError),

    #[error("Invalid escape sequence: {0}")]
    InvalidEscapeSequence(char),

    #[error("Expected ' after char")]
    ExpectedApostrophe,

    #[error("Expected \" after string")]
    ExpectedQuote,

    #[error("Unknown character")]
    UnknownCharacter,
}

#[derive(Debug, Error)]
pub struct SimpleLexError {
    pub kind: LexErrorKind,
    pub line: String,
    pub line_number: usize,
    pub position: usize,
}

impl std::fmt::Display for SimpleLexError {
    #[inline]
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let line_number_str = format!("{:>4}", self.line_number);

        writeln!(f, "Error: {}", self.kind)?;
        writeln!(f)?;
        writeln!(f, "{} | {}", line_number_str, self.line)?;
        writeln!(
            f,
            "{:>1$}^-- Here",
            " ",
            line_number_str.len() + self.position + 3
        )?;
        Ok(())
    }
}

#[derive(Debug, Error)]
pub struct LexError(pub Vec<SimpleLexError>);

impl std::fmt::Display for LexError {
    #[inline]
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        for error in &self.0 {
            writeln!(f, "{}", error)?;
        }
        Ok(())
    }
}

impl LexError {
    #[inline]
    fn multiple_decimals(state: &LexerState) -> SimpleLexError {
        Self::new(state, LexErrorKind::MultipleDecimalPoints)
    }

    #[inline]
    fn invalid_number(state: &LexerState, e: std::num::ParseFloatError) -> SimpleLexError {
        Self::new(state, LexErrorKind::InvalidNumber(e))
    }

    #[inline]
    fn invalid_escape_sequence(state: &LexerState, ch: char) -> SimpleLexError {
        Self::new(state, LexErrorKind::InvalidEscapeSequence(ch))
    }

    #[inline]
    fn expected_apostrophe(state: &LexerState) -> SimpleLexError {
        Self::new(state, LexErrorKind::ExpectedApostrophe)
    }

    #[inline]
    fn expected_quote(state: &LexerState) -> SimpleLexError {
        Self::new(state, LexErrorKind::ExpectedQuote)
    }

    #[inline]
    fn unknown_character(state: &LexerState) -> SimpleLexError {
        Self::new(state, LexErrorKind::UnknownCharacter)
    }

    fn new(state: &LexerState, kind: LexErrorKind) -> SimpleLexError {
        let next_line = state.input[state.cursor..]
            .find('\n')
            .map_or(state.input.len(), |i| i + state.cursor);

        SimpleLexError {
            kind,
            line: state.input[state.bol..next_line].to_string(),
            position: state.cursor - state.bol,
            line_number: state.line_number,
        }
    }
}

impl From<Vec<SimpleLexError>> for LexError {
    fn from(value: Vec<SimpleLexError>) -> Self {
        Self(value)
    }
}

type LexResult1 = Result<Token, SimpleLexError>;
pub type LexResult = Result<Vec<Token>, LexError>;

trait Convert<T> {
    fn convert(self) -> T;
}

impl Convert<LexResult> for Vec<LexResult1> {
    fn convert(self) -> LexResult {
        let mut tokens = Vec::new();
        let mut errors = Vec::new();

        for res in self {
            match res {
                Ok(tok) => tokens.push(tok),
                Err(err) => errors.push(err),
            }
        }

        if errors.is_empty() {
            Ok(tokens)
        } else {
            Err(LexError(errors))
        }
    }
}

pub struct Lexer;

impl Lexer {
    pub fn tokenize(input: &str) -> LexResult {
        let mut results = Vec::new();

        let mut state = LexerState::new(input);

        while !state.is_at_end() {
            let ch = state.current();

            // ignore whitespace
            if ch != '\n' && ch.is_whitespace() {
                state.advance();
                continue;
            }

            let res = match ch {
                '0'..='9' | '.' => Lexer::match_number(&mut state),
                '\'' => Lexer::match_char(&mut state),
                '"' => Lexer::match_string(&mut state),

                // delimiters
                '(' => {
                    state.advance();
                    Ok(Token::ParenLeft)
                }
                ')' => {
                    state.advance();
                    Ok(Token::ParenRight)
                }
                '[' => {
                    state.advance();
                    Ok(Token::BracketLeft)
                }
                ']' => {
                    state.advance();
                    Ok(Token::BracketRight)
                }
                '{' => {
                    state.advance();
                    Ok(Token::CurlyLeft)
                }
                '}' => {
                    state.advance();
                    Ok(Token::CurlyRight)
                }
                ',' => {
                    state.advance();
                    Ok(Token::Comma)
                }
                ':' => {
                    state.advance();
                    Ok(Token::Colon)
                }
                '\n' => {
                    state.advance();
                    Ok(Token::Endline)
                }

                // operators
                '+' => {
                    state.advance();
                    Ok(Token::Plus)
                }
                '-' => {
                    state.advance();
                    Ok(Token::Minus)
                }
                '*' => {
                    state.advance();
                    Ok(Token::Star)
                }
                '/' => {
                    state.advance();
                    Ok(Token::Slash)
                }
                '%' => {
                    state.advance();
                    Ok(Token::Percent)
                }
                '=' => {
                    state.advance();
                    Ok(Token::Equals)
                }
                '>' => Lexer::match_greater(&mut state),
                '<' => Lexer::match_less(&mut state),

                // identifier or keyword
                _ => Lexer::match_word(&mut state),
            };

            // consume faulty character
            if res.is_err() {
                state.advance();
            }

            results.push(res);
        }

        results.convert()
    }

    fn match_number(state: &mut LexerState) -> LexResult1 {
        let start = state.cursor;
        let mut has_decimal = false;

        while !state.is_at_end() {
            let ch = state.current();

            match ch {
                '0'..='9' => {}
                '.' => {
                    if has_decimal {
                        return Err(LexError::multiple_decimals(state));
                    }
                    has_decimal = true;
                }
                _ => break,
            }

            state.advance();
        }

        let number = state.input[start..state.cursor].parse::<f64>();

        match number {
            Ok(n) => Ok(Token::Number(n)),
            Err(e) => Err(LexError::invalid_number(state, e)),
        }
    }

    fn match_char(state: &mut LexerState) -> LexResult1 {
        state.advance(); // consume the '
        let ch = state.current();

        let token = if ch == '\\' {
            state.advance();
            let ch = state.current();
            let escaped = get_escape_char(ch);

            match escaped {
                Some(c) => Ok(Token::Char(c)),
                None => Err(LexError::invalid_escape_sequence(state, ch)),
            }
        } else {
            Ok(Token::Char(ch))
        }?;

        state.advance();

        if state.current() != '\'' {
            return Err(LexError::expected_apostrophe(state));
        }
        state.advance();

        Ok(token)
    }

    fn match_string(state: &mut LexerState) -> LexResult1 {
        state.advance(); // consume the "
        let mut literal = String::new();

        while !state.is_at_end() {
            let ch = state.current();
            if ch == '"' {
                break;
            }

            if ch == '\\' {
                state.advance();
                let ch = state.current();
                let escaped = get_escape_char(ch);

                match escaped {
                    Some(c) => literal.push(c),
                    None => {
                        return Err(LexError::invalid_escape_sequence(state, ch));
                    }
                }
            } else {
                literal.push(ch);
            }
            state.advance();
        }

        if state.current() != '"' {
            return Err(LexError::expected_quote(state));
        }
        state.advance();

        Ok(Token::String(literal))
    }

    fn match_greater(state: &mut LexerState) -> LexResult1 {
        state.advance(); // consume the >

        if state.is_at_end() {
            return Ok(Token::Greater);
        }

        let ch = state.current();

        let tok = match ch {
            '=' => Some(Token::GreaterEqual),
            _ => None,
        };

        match tok {
            Some(tok) => {
                state.advance();
                Ok(tok)
            }
            None => Ok(Token::Greater),
        }
    }

    fn match_less(state: &mut LexerState) -> LexResult1 {
        state.advance(); // consume the <

        if state.is_at_end() {
            return Ok(Token::Less);
        }

        let ch = state.current();

        let tok = match ch {
            '=' => Some(Token::LessEqual),
            '>' => Some(Token::Different),
            '-' => Some(Token::Assignment),
            _ => None,
        };

        match tok {
            Some(tok) => {
                state.advance();
                Ok(tok)
            }
            None => Ok(Token::Less),
        }
    }

    fn match_word(state: &mut LexerState) -> LexResult1 {
        let start = state.cursor;
        let mut first = true;

        while !state.is_at_end() {
            let ch = state.current();
            if !is_identifier_char(ch, first) {
                break;
            }
            first = false;

            state.advance();
        }

        if start == state.cursor {
            return Err(LexError::unknown_character(state));
        }

        let word = &state.input[start..state.cursor];

        match word {
            "null" => Ok(Token::Null),
            "true" => Ok(Token::True),
            "false" => Ok(Token::False),
            "let" => Ok(Token::Let),
            "if" => Ok(Token::If),
            "then" => Ok(Token::Then),
            "else" => Ok(Token::Else),
            "end" => Ok(Token::End),
            "for" => Ok(Token::For),
            "execute" => Ok(Token::Execute),
            "while" => Ok(Token::While),
            "do" => Ok(Token::Do),
            "until" => Ok(Token::Until),
            "print" => Ok(Token::Print),
            "read" => Ok(Token::Read),
            "throw" => Ok(Token::Throw),
            "try" => Ok(Token::Try),
            "catch" => Ok(Token::Catch),
            "function" => Ok(Token::Function),
            "return" => Ok(Token::Return),
            "continue" => Ok(Token::Continue),
            "break" => Ok(Token::Break),
            "include" => Ok(Token::Include),
            "run" => Ok(Token::Run),
            "or" => Ok(Token::Or),
            "and" => Ok(Token::And),
            _ => Ok(Token::Identifier(word.to_string())),
        }
    }
}

#[inline]
fn get_escape_char(ch: char) -> Option<char> {
    match ch {
        '0' => Some('\0'),
        'n' => Some('\n'),
        'r' => Some('\r'),
        't' => Some('\t'),
        '\\' => Some('\\'),
        '\'' => Some('\''),
        '"' => Some('"'),
        _ => None,
    }
}

#[inline]
fn is_identifier_char(ch: char, first: bool) -> bool {
    let lower = ch.is_ascii_lowercase();
    let upper = ch.is_ascii_lowercase();
    let digit = ch.is_ascii_digit() && !first;

    ch == '_' || lower || upper || digit
}

#[cfg(test)]
mod tests {
    use super::*;

    mod literals {
        use super::*;

        #[test]
        fn number_literals() {
            let input = "123 45.67 0.89 .42";
            let tokens = Lexer::tokenize(input).unwrap();
            assert_eq!(
                tokens,
                vec![
                    Token::Number(123.0),
                    Token::Number(45.67),
                    Token::Number(0.89),
                    Token::Number(0.42),
                ]
            );
        }

        #[test]
        fn string_literals() {
            let input = r#""hello" "world\n" "escaped\"quote""#;
            let tokens = Lexer::tokenize(input).unwrap();
            assert_eq!(
                tokens,
                vec![
                    Token::String("hello".to_string()),
                    Token::String("world\n".to_string()),
                    Token::String("escaped\"quote".to_string()),
                ]
            );
        }

        #[test]
        fn char_literals() {
            let input = r"'a' '\n' '\\'";
            let tokens = Lexer::tokenize(input).unwrap();
            assert_eq!(
                tokens,
                vec![Token::Char('a'), Token::Char('\n'), Token::Char('\\'),]
            );
        }

        #[test]
        fn identifiers() {
            let input = "foo bar_baz _qux123";
            let tokens = Lexer::tokenize(input).unwrap();
            assert_eq!(
                tokens,
                vec![
                    Token::Identifier("foo".to_string()),
                    Token::Identifier("bar_baz".to_string()),
                    Token::Identifier("_qux123".to_string()),
                ]
            );
        }
    }

    mod constants {
        use super::*;

        #[test]
        fn constant_keywords() {
            let input = "null true false";
            let tokens = Lexer::tokenize(input).unwrap();
            assert_eq!(tokens, vec![Token::Null, Token::True, Token::False]);
        }
    }

    mod keywords {
        use super::*;

        #[test]
        fn all_keywords() {
            let input = "let if then else end for execute while do until print read throw try catch function return continue break include run";
            let tokens = Lexer::tokenize(input).unwrap();
            assert_eq!(
                tokens,
                vec![
                    Token::Let,
                    Token::If,
                    Token::Then,
                    Token::Else,
                    Token::End,
                    Token::For,
                    Token::Execute,
                    Token::While,
                    Token::Do,
                    Token::Until,
                    Token::Print,
                    Token::Read,
                    Token::Throw,
                    Token::Try,
                    Token::Catch,
                    Token::Function,
                    Token::Return,
                    Token::Continue,
                    Token::Break,
                    Token::Include,
                    Token::Run,
                ]
            );
        }
    }

    mod operators {
        use super::*;

        #[test]
        fn all_operators() {
            let input = "+ - * / % = < <= > >= <> <- or and";
            let tokens = Lexer::tokenize(input).unwrap();
            assert_eq!(
                tokens,
                vec![
                    Token::Plus,
                    Token::Minus,
                    Token::Star,
                    Token::Slash,
                    Token::Percent,
                    Token::Equals,
                    Token::Less,
                    Token::LessEqual,
                    Token::Greater,
                    Token::GreaterEqual,
                    Token::Different,
                    Token::Assignment,
                    Token::Or,
                    Token::And,
                ]
            );
        }

        #[test]
        fn ending_less_greater() {
            let input = "<";
            let tokens = Lexer::tokenize(input).unwrap();
            assert_eq!(tokens, vec![Token::Less]);

            let input = ">";
            let tokens = Lexer::tokenize(input).unwrap();
            assert_eq!(tokens, vec![Token::Greater]);
        }
    }

    mod delimiters {
        use super::*;

        #[test]
        fn all_delimiters() {
            let input = "()[]{} , : \n";

            let tokens = Lexer::tokenize(input).unwrap();
            assert_eq!(
                tokens,
                vec![
                    Token::ParenLeft,
                    Token::ParenRight,
                    Token::BracketLeft,
                    Token::BracketRight,
                    Token::CurlyLeft,
                    Token::CurlyRight,
                    Token::Comma,
                    Token::Colon,
                    Token::Endline,
                ]
            );
        }
    }

    mod edge_cases {
        use super::*;

        #[test]
        fn empty_input() {
            let input = "";
            let tokens = Lexer::tokenize(input).unwrap();
            assert!(tokens.is_empty());
        }

        #[test]
        fn whitespace_handling() {
            let input = "  let\tx\n<-5  ";
            let tokens = Lexer::tokenize(input).unwrap();
            assert_eq!(
                tokens,
                vec![
                    Token::Let,
                    Token::Identifier("x".to_string()),
                    Token::Endline,
                    Token::Assignment,
                    Token::Number(5.0),
                ]
            );
        }

        #[test]
        fn identifier_vs_keyword() {
            let input = "nullx true123 falsey";
            let tokens = Lexer::tokenize(input).unwrap();
            assert_eq!(
                tokens,
                vec![
                    Token::Identifier("nullx".to_string()),
                    Token::Identifier("true123".to_string()),
                    Token::Identifier("falsey".to_string()),
                ]
            );
        }
    }

    mod error_handling {
        use super::*;

        #[test]
        fn invalid_char() {
            let input = "~";
            let result = Lexer::tokenize(input);
            assert!(result.is_err());
        }

        #[test]
        fn unterminated_string() {
            let input = "\"hello";
            let result = Lexer::tokenize(input);
            assert!(result.is_err());
        }

        #[test]
        fn unterminated_char() {
            let input = "'a";
            let result = Lexer::tokenize(input);
            assert!(result.is_err());
        }

        #[test]
        fn invalid_number() {
            let input = "12a34";
            let result = Lexer::tokenize(input);
            assert!(result.is_err());
        }

        #[test]
        fn identifier_after_number() {
            let input = "123if";
            let result = Lexer::tokenize(input);
            assert!(result.is_err());
        }

        #[test]
        fn multiple_decimals() {
            let input = "123.45.67";
            let result = Lexer::tokenize(input);
            assert!(result.is_err());
        }
    }

    mod integration {
        use super::*;

        #[test]
        fn sample_program() {
            let input = r#"
                let x <- 5
                if x = 5 then
                    print "hello"
                end
            "#;

            let tokens = Lexer::tokenize(input).unwrap();
            assert_eq!(
                tokens,
                vec![
                    Token::Endline,
                    Token::Let,
                    Token::Identifier("x".to_string()),
                    Token::Assignment,
                    Token::Number(5.0),
                    Token::Endline,
                    Token::If,
                    Token::Identifier("x".to_string()),
                    Token::Equals,
                    Token::Number(5.0),
                    Token::Then,
                    Token::Endline,
                    Token::Print,
                    Token::String("hello".to_string()),
                    Token::Endline,
                    Token::End,
                    Token::Endline,
                ]
            );
        }
    }
}
