pub enum AST {
    Expressions(Expressions),
    Expression(Expression),
    IfExpression(IfExpression),
    ForExpression(ForExpression),
}

pub struct Expressions(pub Vec<Expression>);

pub struct Expression(pub Box<AST>);

pub struct IfExpression {
    pub condition: Expression,
    pub true_body: Expressions,
    pub false_body: Option<Expressions>,
}

pub struct ForExpression {
    pub identifier: String,
    pub start: Expression,
    pub end: Expression,
    pub step: Option<Expression>,
    pub body: Expressions,
}
