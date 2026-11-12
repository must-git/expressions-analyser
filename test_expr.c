#include "expr.h"

#include <math.h>
#include <stdio.h>

static int failures = 0;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "ECHEC %s:%d : %s\n", __FILE__, __LINE__,          \
                    #condition);                                               \
            failures++;                                                        \
        }                                                                      \
    } while (0)

static Token *read_token(Queue *queue, const TokenList *store)
{
    Item index;
    if (queue_dequeue(queue, &index) != DS_OK) {
        return NULL;
    }
    if (index < 0 || (size_t)index >= store->count) {
        return NULL;
    }
    return &store->items[(size_t)index];
}

static void expect_ok(const char *expression, double expected)
{
    ExprError error;
    double actual = 0.0;

    ExprStatus status = expr_evaluate(expression, &actual, &error);
    if (status != EXPR_OK) {
        fprintf(stderr,
                "ECHEC expression \"%s\" : statut %s (%s), position %zu\n",
                expression, expr_status_str(status), error.message,
                error.position);
        failures++;
        return;
    }

    if (fabs(actual - expected) >= 1e-9) {
        fprintf(stderr,
                "ECHEC expression \"%s\" : resultat %.17g, attendu %.17g\n",
                expression, actual, expected);
        failures++;
    }
}

static void expect_error(const char *expression, ExprStatus expected,
                         size_t expected_position)
{
    ExprError error;
    double actual = 0.0;

    ExprStatus status = expr_evaluate(expression, &actual, &error);
    if (status != expected) {
        fprintf(stderr,
                "ECHEC expression \"%s\" : statut %s, attendu %s (%s)\n",
                expression, expr_status_str(status),
                expr_status_str(expected), error.message);
        failures++;
        return;
    }

    if (error.status != expected) {
        fprintf(stderr,
                "ECHEC expression \"%s\" : error.status = %s, attendu %s\n",
                expression, expr_status_str(error.status),
                expr_status_str(expected));
        failures++;
    }

    if (error.position != expected_position) {
        fprintf(stderr,
                "ECHEC expression \"%s\" : position %zu, attendue %zu\n",
                expression, error.position, expected_position);
        failures++;
    }
}

static void expect_brackets(const char *expression, ExprStatus expected,
                            size_t expected_position)
{
    ExprError error;
    ExprStatus status = expr_check_brackets(expression, &error);

    if (status != expected) {
        fprintf(stderr,
                "ECHEC parentheses \"%s\" : statut %s, attendu %s (%s)\n",
                expression, expr_status_str(status),
                expr_status_str(expected), error.message);
        failures++;
        return;
    }

    if (status != EXPR_OK && error.position != expected_position) {
        fprintf(stderr,
                "ECHEC parentheses \"%s\" : position %zu, attendue %zu\n",
                expression, error.position, expected_position);
        failures++;
    }
}

static void test_brackets(void)
{
    expect_brackets("(3 + 4)", EXPR_OK, 0);
    expect_brackets("((3 + 4) * 2)", EXPR_OK, 0);
    expect_brackets("3 + 4", EXPR_OK, 0);
    expect_brackets("", EXPR_OK, 0);

    expect_brackets("(3 + 4", EXPR_ERR_UNBALANCED_OPEN, 0);
    expect_brackets("3 + 4)", EXPR_ERR_UNBALANCED_CLOSE, 5);
    expect_brackets("(3 + 4))", EXPR_ERR_UNBALANCED_CLOSE, 7);
    expect_brackets("((3 + 4)", EXPR_ERR_UNBALANCED_OPEN, 0);
    expect_brackets(")", EXPR_ERR_UNBALANCED_CLOSE, 0);
    expect_brackets("3 + (4 * (5 + 6)", EXPR_ERR_UNBALANCED_OPEN, 4);
}

static void test_tokenization(void)
{
    ExprError error;
    TokenList store;
    Queue *tokens = queue_create();
    CHECK(tokens != NULL);
    if (tokens == NULL) {
        return;
    }

    ExprStatus status = expr_tokenize("-3 + 4", tokens, &store, &error);
    CHECK(status == EXPR_OK);
    if (status == EXPR_OK) {
        CHECK(store.count == 4);

        Token *token = read_token(tokens, &store);
        CHECK(token != NULL);
        if (token != NULL) {
            CHECK(token->type == TOK_UMINUS);
            CHECK(token->position == 0);
        }

        token = read_token(tokens, &store);
        CHECK(token != NULL);
        if (token != NULL) {
            CHECK(token->type == TOK_NUMBER);
            CHECK(token->position == 1);
            CHECK(token->value.number == 3.0);
        }

        token = read_token(tokens, &store);
        CHECK(token != NULL);
        if (token != NULL) {
            CHECK(token->type == TOK_OPERATOR);
            CHECK(token->value.op == '+');
            CHECK(token->position == 3);
        }

        token = read_token(tokens, &store);
        CHECK(token != NULL);
        if (token != NULL) {
            CHECK(token->type == TOK_NUMBER);
            CHECK(token->position == 5);
            CHECK(token->value.number == 4.0);
        }

        CHECK(queue_is_empty(tokens));
    }

    token_list_free(&store);
    queue_destroy(tokens);

    tokens = queue_create();
    CHECK(tokens != NULL);
    if (tokens == NULL) {
        return;
    }

    status = expr_tokenize("12.5 + 3", tokens, &store, &error);
    CHECK(status == EXPR_OK);
    if (status == EXPR_OK) {
        CHECK(store.count == 3);

        Token *token = read_token(tokens, &store);
        CHECK(token != NULL);
        if (token != NULL) {
            CHECK(token->type == TOK_NUMBER);
            CHECK(token->position == 0);
            CHECK(token->value.number == 12.5);
        }

        token = read_token(tokens, &store);
        CHECK(token != NULL);
        if (token != NULL) {
            CHECK(token->type == TOK_OPERATOR);
            CHECK(token->value.op == '+');
            CHECK(token->position == 5);
        }

        token = read_token(tokens, &store);
        CHECK(token != NULL);
        if (token != NULL) {
            CHECK(token->type == TOK_NUMBER);
            CHECK(token->position == 7);
            CHECK(token->value.number == 3.0);
        }
    }

    token_list_free(&store);
    queue_destroy(tokens);

    tokens = queue_create();
    CHECK(tokens != NULL);
    if (tokens == NULL) {
        return;
    }

    status = expr_tokenize("2 * -(1 + 1)", tokens, &store, &error);
    CHECK(status == EXPR_OK);
    if (status == EXPR_OK) {
        const TokenType expected_types[] = {
            TOK_NUMBER, TOK_OPERATOR, TOK_UMINUS, TOK_LPAREN,
            TOK_NUMBER, TOK_OPERATOR, TOK_NUMBER, TOK_RPAREN};
        const size_t expected_positions[] = {0, 2, 4, 5, 6, 8, 10, 11};

        CHECK(store.count == 8);
        for (size_t i = 0; i < store.count && i < 8; i++) {
            Token *token = read_token(tokens, &store);
            CHECK(token != NULL);
            if (token != NULL) {
                CHECK(token->type == expected_types[i]);
                CHECK(token->position == expected_positions[i]);
            }
        }
        CHECK(queue_is_empty(tokens));
    }

    token_list_free(&store);
    queue_destroy(tokens);

    tokens = queue_create();
    CHECK(tokens != NULL);
    if (tokens == NULL) {
        return;
    }

    status = expr_tokenize("3 $ 4", tokens, &store, &error);
    CHECK(status == EXPR_ERR_LEXICAL);
    CHECK(error.position == 2);
    CHECK(store.items == NULL);
    CHECK(store.count == 0);
    CHECK(queue_is_empty(tokens));
    queue_destroy(tokens);

    tokens = queue_create();
    CHECK(tokens != NULL);
    if (tokens == NULL) {
        return;
    }

    status = expr_tokenize("1.2.3", tokens, &store, &error);
    CHECK(status == EXPR_ERR_LEXICAL);
    CHECK(error.position == 3);
    queue_destroy(tokens);
}

static void test_postfix_conversion(void)
{
    ExprError error;
    TokenList store;
    Queue *tokens = queue_create();
    Queue *postfix = queue_create();
    CHECK(tokens != NULL);
    CHECK(postfix != NULL);
    if (tokens == NULL || postfix == NULL) {
        queue_destroy(tokens);
        queue_destroy(postfix);
        return;
    }

    ExprStatus status = expr_tokenize("3 + 4 * 2", tokens, &store, &error);
    CHECK(status == EXPR_OK);
    if (status == EXPR_OK) {
        status = expr_to_postfix(tokens, postfix, &store, &error);
        CHECK(status == EXPR_OK);

        if (status == EXPR_OK) {
            const TokenType expected_types[] = {TOK_NUMBER, TOK_NUMBER,
                                                TOK_NUMBER, TOK_OPERATOR,
                                                TOK_OPERATOR};
            const char expected_ops[] = {0, 0, 0, '*', '+'};

            for (size_t i = 0; i < 5; i++) {
                Token *token = read_token(postfix, &store);
                CHECK(token != NULL);
                if (token != NULL) {
                    CHECK(token->type == expected_types[i]);
                    if (token->type == TOK_OPERATOR) {
                        CHECK(token->value.op == expected_ops[i]);
                    }
                }
            }
            CHECK(queue_is_empty(postfix));
        }
    }

    token_list_free(&store);
    queue_destroy(tokens);
    queue_destroy(postfix);

    tokens = queue_create();
    postfix = queue_create();
    CHECK(tokens != NULL);
    CHECK(postfix != NULL);
    if (tokens == NULL || postfix == NULL) {
        queue_destroy(tokens);
        queue_destroy(postfix);
        return;
    }

    status = expr_tokenize("2 * -(1 + 1)", tokens, &store, &error);
    CHECK(status == EXPR_OK);
    if (status == EXPR_OK) {
        status = expr_to_postfix(tokens, postfix, &store, &error);
        CHECK(status == EXPR_OK);

        if (status == EXPR_OK) {
            const TokenType expected_types[] = {
                TOK_NUMBER, TOK_NUMBER, TOK_NUMBER, TOK_OPERATOR,
                TOK_UMINUS, TOK_OPERATOR};

            for (size_t i = 0; i < 6; i++) {
                Token *token = read_token(postfix, &store);
                CHECK(token != NULL);
                if (token != NULL) {
                    CHECK(token->type == expected_types[i]);
                }
            }
            CHECK(queue_is_empty(postfix));
        }
    }

    token_list_free(&store);
    queue_destroy(tokens);
    queue_destroy(postfix);
}

static void test_valid_expressions(void)
{
    expect_ok("3 + 4", 7.0);
    expect_ok("3 + 4 * 2", 11.0);
    expect_ok("(3 + 4) * 2", 14.0);
    expect_ok("12.5 + 3", 15.5);
    expect_ok("10 / 2", 5.0);
    expect_ok("10 - 3 - 2", 5.0);
    expect_ok("-3 + 4", 1.0);
    expect_ok("-3", -3.0);
    expect_ok("+3", 3.0);
    expect_ok("2 * -(1 + 1)", -4.0);
    expect_ok("+3 + 4", 7.0);
    expect_ok("2 * -3", -6.0);
    expect_ok("3 - -2", 5.0);
    expect_ok("-(-3)", 3.0);
    expect_ok("  7  ", 7.0);
    expect_ok("1 / 2", 0.5);
    expect_ok("1e2 + 1", 101.0);
    expect_ok("0.5 * 4", 2.0);
    expect_ok("1+1+1+1+1+1+1+1+1+1", 10.0);
    expect_ok("((((1 + 2))))", 3.0);

    /* Extension : puissance, associative à droite. */
    expect_ok("2 ^ 3 ^ 2", 512.0);
    expect_ok("2 ^ -3", 0.125);
    expect_ok("-2 ^ 2", -4.0);
}

static void test_invalid_expressions(void)
{
    expect_error("", EXPR_ERR_EMPTY, 0);
    expect_error("   ", EXPR_ERR_EMPTY, 0);
    expect_error("3 +", EXPR_ERR_SYNTAX, 3);
    expect_error("* 3", EXPR_ERR_SYNTAX, 0);
    expect_error("3 + * 4", EXPR_ERR_SYNTAX, 4);
    expect_error("()", EXPR_ERR_SYNTAX, 1);
    expect_error("(3 + 4", EXPR_ERR_UNBALANCED_OPEN, 0);
    expect_error("3 + 4)", EXPR_ERR_UNBALANCED_CLOSE, 5);
    expect_error("(3 + 4))", EXPR_ERR_UNBALANCED_CLOSE, 7);
    expect_error("3 + (4", EXPR_ERR_UNBALANCED_OPEN, 4);
    expect_error("3 / 0", EXPR_ERR_DIVISION_BY_ZERO, 2);
    expect_error("3 4", EXPR_ERR_SYNTAX, 2);
    expect_error("3 $ 4", EXPR_ERR_LEXICAL, 2);
    expect_error("3 $)", EXPR_ERR_LEXICAL, 2);
    expect_error("0x10", EXPR_ERR_LEXICAL, 1);
    expect_error("$", EXPR_ERR_LEXICAL, 0);
    expect_error("1.2.3", EXPR_ERR_LEXICAL, 3);
    expect_error("3 + ()", EXPR_ERR_SYNTAX, 5);
    expect_error("1e999", EXPR_ERR_NONFINITE, 0);
    expect_error("1e308 * 10", EXPR_ERR_NONFINITE, 6);
    expect_error(")", EXPR_ERR_UNBALANCED_CLOSE, 0);
}

static void test_null_arguments(void)
{
    ExprError error;
    double result = 0.0;

    CHECK(expr_evaluate(NULL, &result, &error) == EXPR_ERR_NULL);
    CHECK(expr_evaluate("3 + 4", NULL, &error) == EXPR_ERR_NULL);
    CHECK(expr_check_brackets(NULL, &error) == EXPR_ERR_NULL);

    Queue *tokens = queue_create();
    CHECK(tokens != NULL);
    if (tokens != NULL) {
        TokenList store;
        CHECK(expr_tokenize(NULL, tokens, &store, &error) == EXPR_ERR_NULL);
        queue_destroy(tokens);
    }

    token_list_free(NULL);
    expr_error_clear(NULL);
}

int main(void)
{
    test_brackets();
    test_tokenization();
    test_postfix_conversion();
    test_valid_expressions();
    test_invalid_expressions();
    test_null_arguments();

    if (failures == 0) {
        printf("test_expr : tous les tests sont passes\n");
        return 0;
    }

    fprintf(stderr, "test_expr : %d echec(s)\n", failures);
    return 1;
}
