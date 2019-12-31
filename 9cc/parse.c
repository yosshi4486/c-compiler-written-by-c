#include "9cc.h"

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

Node *new_node_ident(Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;

    // aがASCIIでは97で管理されており、intだと文字コードで扱われる。
    // str[0]が例えばcだとするとintでは99となり、99 - a = 2となり、式全体を評価するとRBPからのオフセットが24となる。
    // 変数aのオフセットが8、変数bのオフセットが16, 変数cのオフセットが24が期待する数値なので、これは期待通りの挙動である。
    // これはASCIIコード上でアルファベットが連続しているから実現できるテクニック
    node->offset = (tok->str[0] - 'a' + 1) * 8;
    return node;
}

// 関数プロトタイプを宣言しないとエラーが出てしまう
Node *stmt();
Node *assign();
Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *primary();
Node *unary();

Node *code[100];

void program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

Node *assign() {
    Node *node = equality();
    if (consume('=')) {
        node = new_node(ND_ASSIGN, node, assign());
    } 
    return node;
}

Node *stmt() {
    Node *node = expr();
    expect(';');
    return node;
}

Node *expr() {
  return assign();
}

Node *equality() {
  Node *node = relational();

  for(;;) {
    if (consume("==")) {
      node = new_node(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_node(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

Node *relational() {
  Node *node = add();

  if (consume("<")) {
    node = new_node(ND_LT, node, add());
  } else if (consume("<=")) {
    node = new_node(ND_LE, node, add());
  } else if (consume(">")) {
    node = new_node(ND_LT, add(), node);
  } else if (consume(">=")) {
    node = new_node(ND_LE, add(), node);
  } else {
    return node;
  }
}

Node *add() {
  Node *node = mul();

  for(;;) {
    if (consume("+")) {
      node = new_node(ND_ADD, node, mul());
    } else if (consume("-")) {
      node = new_node(ND_SUB, node, mul());
    } else {
      return node;
    }
  }

}

Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*")) {
      node = new_node(ND_MUL, node, unary());
    } else if (consume("/")) {
      node = new_node(ND_DIV, node, unary());
    } else {
      return node;
    }
  }
}

Node *primary() {
  // 次のトークンが"("なら、"(" expr ")" のはず
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  } 

  Token *tok = consume_ident();
  if (tok) {
      return new_node_ident(tok);
  }

  // そうでなければ数値のはず
  return new_node_num(expect_number());
}

Node *unary() {
  if (consume("+")) {
    return primary();
  }
  if (consume("-")) {
    return new_node(ND_SUB, new_node_num(0), primary());
  }

  return primary();
}