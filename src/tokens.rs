#[derive(Debug, PartialEq)]
pub enum Token {
    // literals
    Number(f64),
    Char(char),
    String(String),
    Identifier(String),

    // constants
    Null,
    True,
    False,

    // keywords
    Let,
    If,
    Then,
    Else,
    End,
    For,
    Execute,
    While,
    Do,
    Until,
    Print,
    Read,
    Throw,
    Try,
    Catch,
    Function,
    Return,
    Continue,
    Break,
    Include,
    Run,

    // operators
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulo,
    Equals,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Different,
    Assignment,
    Or,
    And,

    // delimiters
    ParenLeft,
    ParenRight,
    BracketLeft,
    BracketRight,
    CurlyLeft,
    CurlyRight,
    Comma,
    Colon,
    Endline,
}
