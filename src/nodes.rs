#[derive(Debug, PartialEq)]
pub enum LogicalOperator {
    And,
    Or,
}

#[derive(Debug, PartialEq)]
pub enum ComparisonOperator {
    Equal,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Different,
}

#[derive(Debug, PartialEq)]
pub enum ArithmeticOperator {
    Add,
    Subtract,
}

#[derive(Debug, PartialEq)]
pub enum ArithmeticOperator2 {
    Multiply,
    Divide,
    Modulo,
}

#[derive(Debug, PartialEq)]
pub enum UnaryOperator {
    Plus,
    Minus,
}

#[derive(Debug, PartialEq)]
pub enum Ast {
    Expressions(Expressions),
    If {
        condition: Box<Ast>,
        true_body: Expressions,
        false_body: Option<Expressions>,
    },
    For {
        identifier: String,
        start: Box<Ast>,
        end: Box<Ast>,
        step: Option<Box<Ast>>,
        body: Expressions,
    },
    While {
        condition: Box<Ast>,
        body: Expressions,
    },
    DoUntil {
        condition: Box<Ast>,
        body: Expressions,
    },
    Continue,
    Break,
    TryCatch {
        try_body: Expressions,
        catch_identifier: String,
        catch_body: Expressions,
    },
    Throw(Box<Ast>),
    FunctionDefinition {
        identifier: String,
        parameters: Vec<String>,
        body: Expressions,
    },
    Return(Box<Ast>),
    Include(String),
    Run(String),
    Read {
        file: Option<String>,
        identifiers: Vec<String>,
    },
    Print {
        file: Option<String>,
        expressions: Vec<Ast>,
    },
    Assignment {
        identifier: String,
        index_access: Vec<Ast>,
        expression: Box<Ast>,
    },
    LogicalOperation {
        left: Box<Ast>,
        right: Box<Ast>,
        operator: LogicalOperator,
    },
    ComparisonOperation {
        left: Box<Ast>,
        right: Box<Ast>,
        operator: ComparisonOperator,
    },
    ArithmeticOperation {
        left: Box<Ast>,
        right: Box<Ast>,
        operator: ArithmeticOperator,
    },
    ArithmeticOperation2 {
        left: Box<Ast>,
        right: Box<Ast>,
        operator: ArithmeticOperator2,
    },

    FnCall {
        expr: Box<Ast>,
        arguments: Vec<Ast>,
    },
    IndexAccess {
        expr: Box<Ast>,
        index: Box<Ast>,
    },

    Number(f64),
    Char(char),
    String(String),
    Identifier(String),
    Array(Vec<Ast>),
    Dictionary(Vec<(Ast, Ast)>),
    Unary {
        operator: UnaryOperator,
        expr: Box<Ast>,
    },
}

#[derive(Debug, PartialEq)]
pub struct Expressions(pub Vec<Ast>);
