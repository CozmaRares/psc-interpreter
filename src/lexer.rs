use crate::tokens::Token;

struct LexerState<'a> {
    input: &'a str,
    cursor: usize,
}

impl<'a> LexerState<'a> {
    #[inline]
    fn new(input: &'a str) -> Self {
        Self { input, cursor: 0 }
    }

    #[inline]
    fn is_at_end(&self) -> bool {
        self.cursor >= self.input.len()
    }

    #[inline]
    fn advance(&mut self) {
        self.cursor += 1;
    }

    #[inline]
    fn current(&self) -> char {
        self.input.chars().nth(self.cursor).unwrap()
    }
}

pub struct Lexer;

impl Lexer {
    pub fn tokenize(input: &str) -> Vec<Token> {
        let mut tokens = Vec::new();

        let mut state = LexerState::new(input);

        while !state.is_at_end() {
            let ch = state.current();
            state.advance();

            let tok = match ch {
                _ => todo!(),
            };

            tokens.push(tok);
        }

        return tokens;
    }
}
