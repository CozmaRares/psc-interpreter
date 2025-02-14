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
        if self.current().unwrap() == '\n' {
            self.bol = self.cursor;
            self.line_number += 1;
        }

        self.cursor += 1;
    }

    #[inline]
    fn current(&self) -> Result<char, LexError> {
        match self.input.chars().nth(self.cursor) {
            Some(c) => Ok(c),
            None => Err(LexError::new(self, "Unexpected end of input".to_string())),
        }
    }
}

#[derive(Debug, Error)]
pub struct LexError {
    message: String,
    line: String,
    line_number: usize,
    position: usize,
}

impl std::fmt::Display for LexError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        writeln!(f, "Error: {}", self.message)?;
        writeln!(f)?;
        writeln!(f, "{} | {}", self.line_number, self.line)?;
        writeln!(f, "{:>1$}^-- Here", " ", self.position)?;
        Ok(())
    }
}

impl LexError {
    fn new(state: &LexerState, message: String) -> Self {
        let next_line = state.input[state.cursor..]
            .find('\n')
            .unwrap_or(state.input.len());

        Self {
            message,
            line: state.input[state.bol..next_line].to_string(),
            position: state.cursor - state.bol,
            line_number: state.line_number,
        }
    }
}

type LexResult1 = Result<Token, LexError>;
pub type LexResult = Result<Vec<Token>, Vec<LexError>>;

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
            Err(errors)
        }
    }
}

pub struct Lexer;

impl Lexer {
    pub fn tokenize(input: &str) -> LexResult {
        let mut results = Vec::new();

        let mut state = LexerState::new(input);

        while !state.is_at_end() {
            let ch = state.current().unwrap(); // first char is always valid

            // ignore whitespace
            if ch.is_whitespace() {
                state.advance();
                continue;
            }

            let res = match ch {
                '0'..='9' => Lexer::match_number(&mut state),
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
                    Ok(Token::Multiply)
                }
                '/' => {
                    state.advance();
                    Ok(Token::Divide)
                }
                '%' => {
                    state.advance();
                    Ok(Token::Modulo)
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

            results.push(res);
        }

        results.convert()
    }

    fn match_number(state: &mut LexerState) -> LexResult1 {
        let start = state.cursor;

        while !state.is_at_end() && state.current()?.is_ascii_digit() {
            state.advance();
        }

        if state.is_at_end() {
            return parse_number(state, start);
        }

        if state.current()? == '.' {
            state.advance();
        }

        while state.current()?.is_ascii_digit() {
            state.advance();
        }

        parse_number(state, start)
    }

    fn match_char(state: &mut LexerState) -> LexResult1 {
        state.advance(); // consume the '
        let ch = state.current()?;

        let token = if ch == '\\' {
            state.advance();
            let ch = state.current()?;
            let escaped = get_escape_char(ch);

            match escaped {
                Some(c) => Ok(Token::Char(c)),
                None => Err(LexError::new(state, "Invalid escape sequence".to_string())),
            }
        } else {
            Ok(Token::Char(ch))
        }?;

        state.advance();

        if state.current()? != '\'' {
            return Err(LexError::new(state, "Expected ' after char".to_string()));
        }
        state.advance();

        Ok(token)
    }

    fn match_string(state: &mut LexerState) -> LexResult1 {
        state.advance(); // consume the "
        let mut literal = String::new();

        while !state.is_at_end() {
            let ch = state.current()?;
            if ch == '"' {
                break;
            }

            if ch == '\\' {
                state.advance();
                let ch = state.current()?;
                let escaped = get_escape_char(ch);

                match escaped {
                    Some(c) => literal.push(c),
                    None => {
                        return Err(LexError::new(state, "Invalid escape sequence".to_string()))
                    }
                }
            } else {
                literal.push(ch);
            }
            state.advance();
        }

        if state.current()? != '"' {
            return Err(LexError::new(state, "Expected \" after string".to_string()));
        }
        state.advance();

        Ok(Token::String(literal))
    }

    fn match_greater(state: &mut LexerState) -> LexResult1 {
        state.advance(); // consume the >

        if state.is_at_end() {
            return Ok(Token::Greater);
        }

        let ch = state.current()?;

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

        let ch = state.current()?;

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
            let ch = state.current()?;
            if !is_identifier_char(ch, first) {
                break;
            }
            first = false;

            state.advance();
        }

        if start == state.cursor {
            return Err(LexError::new(state, "Unknown character".to_string()));
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

fn get_escape_char(ch: char) -> Option<char> {
    match ch {
        'n' => Some('\n'),
        'r' => Some('\r'),
        't' => Some('\t'),
        '\\' => Some('\\'),
        '\'' => Some('\''),
        '"' => Some('"'),
        _ => None,
    }
}

fn is_identifier_char(ch: char, first: bool) -> bool {
    let lower = ch.is_ascii_lowercase();
    let upper = ch.is_ascii_lowercase();
    let digit = ch.is_ascii_digit() && !first;

    ch == '_' || lower || upper || digit
}

fn parse_number(state: &mut LexerState, start: usize) -> LexResult1 {
    let number = state.input[start..state.cursor].parse::<f64>();

    match number {
        Ok(n) => Ok(Token::Number(n)),
        Err(_) => Err(LexError::new(state, "Invalid number".to_string())),
    }
}
