#[derive(Debug, PartialEq)]
pub enum Ast {
    Expressions(Expressions),
    If(IfExpression),
    For(ForExpression),
    While(WhileExpression),
    DoUntil(DoUntilExpression),
    Continue,
    Break,
    TryCatch(TryCatchExpression),
    Throw(Expression),
    FunctionDefinition(FunctionDefinition),
    Return(Expression),
    Include(String),
    Run(String),
    Read(ReadExpression),
    Print(PrintExpression),
    Assignment(AssignmentExpression),
    LogicalOperation(LogicalOperationExpression),
    ComparisonOperation(ComparisonExpression),
    ArithmeticOperation(ArithmeticExpression),
    ArithmeticOperation2(ArithmeticOperation2),
    Factor(FactorExpression),
    Base(BaseExpression),
}

#[derive(Debug, PartialEq)]
pub struct Expressions(pub Vec<Expression>);

#[derive(Debug, PartialEq)]
pub struct Expression(pub Box<Ast>);

#[derive(Debug, PartialEq)]
pub struct IfExpression {
    pub condition: Expression,
    pub true_body: Expressions,
    pub false_body: Option<Expressions>,
}

#[derive(Debug, PartialEq)]
pub struct ForExpression {
    pub identifier: String,
    pub start: Expression,
    pub end: Expression,
    pub step: Option<Expression>,
    pub body: Expressions,
}

#[derive(Debug, PartialEq)]
pub struct WhileExpression {
    pub condition: Expression,
    pub body: Expressions,
}

#[derive(Debug, PartialEq)]
pub struct DoUntilExpression {
    pub condition: Expression,
    pub body: Expressions,
}

#[derive(Debug, PartialEq)]
pub struct TryCatchExpression {
    pub try_body: Expressions,
    pub catch_identifier: String,
    pub catch_body: Expressions,
}

#[derive(Debug, PartialEq)]
pub struct FunctionDefinition {
    pub identifier: String,
    pub parameters: Vec<String>,
    pub body: Expressions,
}

#[derive(Debug, PartialEq)]
pub struct ReadExpression {
    pub file: Option<String>,
    pub identifiers: Vec<String>,
}

#[derive(Debug, PartialEq)]
pub struct PrintExpression {
    pub file: Option<String>,
    pub expressions: Vec<Expression>,
}

#[derive(Debug, PartialEq)]
pub struct AssignmentExpression {
    pub identifier: String,
    pub index_access: Vec<Expression>,
    pub expression: Expression,
}

#[derive(Debug, PartialEq)]
pub enum LogicalOperator {
    And,
    Or,
}
#[derive(Debug, PartialEq)]
pub struct LogicalOperationExpression {
    pub left: Expression,
    pub right: Expression,
    pub operator: LogicalOperator,
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
pub struct ComparisonExpression {
    pub left: Expression,
    pub right: Expression,
    pub operator: ComparisonOperator,
}

#[derive(Debug, PartialEq)]
pub enum ArithmeticOperator {
    Add,
    Subtract,
}
#[derive(Debug, PartialEq)]
pub struct ArithmeticExpression {
    pub left: Expression,
    pub right: Expression,
    pub operator: ArithmeticOperator,
}

#[derive(Debug, PartialEq)]
pub enum ArithmeticOperator2 {
    Multiply,
    Divide,
    Modulo,
}
#[derive(Debug, PartialEq)]
pub struct ArithmeticOperation2 {
    pub left: Expression,
    pub right: Expression,
    pub operator: ArithmeticOperator2,
}

#[derive(Debug, PartialEq)]
pub enum FactorExpression {
    FnCall {
        base: BaseExpression,
        arguments: Vec<Expression>,
    },
    IndexAccess {
        base: BaseExpression,
        index: Vec<Expression>,
    },
}

#[derive(Debug, PartialEq)]
pub enum UnaryOperator {
    Plus,
    Minus,
}

#[derive(Debug, PartialEq)]
pub enum BaseExpression {
    Number(f64),
    Char(char),
    String(String),
    Identifier(String),
    Expression(Expression),
    Array(Vec<Expression>),
    Dictionary(Vec<(Expression, Expression)>),
    Unary {
        operator: UnaryOperator,
        base: Box<BaseExpression>,
    },
}
