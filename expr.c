#include "expr.h"

#include "stack.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Messages et conversions de codes                                          */
/* ------------------------------------------------------------------------- */

const char *expr_status_str(ExprStatus status)
{
    switch (status) {
    case EXPR_OK:
        return "EXPR_OK";
    case EXPR_ERR_NULL:
        return "EXPR_ERR_NULL";
    case EXPR_ERR_EMPTY:
        return "EXPR_ERR_EMPTY";
    case EXPR_ERR_LEXICAL:
        return "EXPR_ERR_LEXICAL";
    case EXPR_ERR_UNBALANCED_OPEN:
        return "EXPR_ERR_UNBALANCED_OPEN";
    case EXPR_ERR_UNBALANCED_CLOSE:
        return "EXPR_ERR_UNBALANCED_CLOSE";
    case EXPR_ERR_SYNTAX:
        return "EXPR_ERR_SYNTAX";
    case EXPR_ERR_DIVISION_BY_ZERO:
        return "EXPR_ERR_DIVISION_BY_ZERO";
    case EXPR_ERR_NONFINITE:
        return "EXPR_ERR_NONFINITE";
    case EXPR_ERR_NOMEM:
        return "EXPR_ERR_NOMEM";
    case EXPR_ERR_INTERNAL:
        return "EXPR_ERR_INTERNAL";
    default:
        return "EXPR_ERR_UNKNOWN";
    }
}

const char *token_type_str(TokenType type)
{
    switch (type) {
    case TOK_NUMBER:
        return "TOK_NUMBER";
    case TOK_OPERATOR:
        return "TOK_OPERATOR";
    case TOK_UMINUS:
        return "TOK_UMINUS";
    case TOK_UPLUS:
        return "TOK_UPLUS";
    case TOK_LPAREN:
        return "TOK_LPAREN";
    case TOK_RPAREN:
        return "TOK_RPAREN";
    default:
        return "TOK_UNKNOWN";
    }
}

void expr_error_clear(ExprError *error)
{
    if (error == NULL) {
        return;
    }
    error->status = EXPR_OK;
    error->position = 0;
    error->message[0] = '\0';
}

static void set_error(ExprError *error, ExprStatus status, size_t position,
                      const char *format, ...)
{
    if (error == NULL) {
        return;
    }

    error->status = status;
    error->position = position;

    va_list args;
    va_start(args, format);
    (void)vsnprintf(error->message, sizeof error->message, format, args);
    va_end(args);
}

/* ------------------------------------------------------------------------- */
/* Petits utilitaires internes                                               */
/* ------------------------------------------------------------------------- */

/*
 * Pile interne de doubles : le TAD Stack ne stocke que des Item (int), donc il
 * ne peut pas contenir directement les opérandes réels de l'évaluation.
 */
typedef struct DoubleStack {
    double *data;
    size_t size;
    size_t capacity;
} DoubleStack;

static void double_stack_destroy(DoubleStack *stack)
{
    free(stack->data);
    stack->data = NULL;
    stack->size = 0;
    stack->capacity = 0;
}

static bool double_stack_push(DoubleStack *stack, double value)
{
    if (stack->size == stack->capacity) {
        size_t new_capacity;

        if (stack->capacity == 0) {
            new_capacity = 8;
        } else {
            if (stack->capacity > SIZE_MAX / 2) {
                return false;
            }
            new_capacity = stack->capacity * 2;
        }

        if (new_capacity > SIZE_MAX / sizeof *stack->data) {
            return false;
        }

        double *new_data =
            realloc(stack->data, new_capacity * sizeof *new_data);
        if (new_data == NULL) {
            return false;
        }

        stack->data = new_data;
        stack->capacity = new_capacity;
    }

    stack->data[stack->size] = value;
    stack->size++;
    return true;
}

static bool double_stack_pop(DoubleStack *stack, double *out)
{
    if (stack->size == 0) {
        return false;
    }

    stack->size--;
    *out = stack->data[stack->size];
    return true;
}

static Token *token_at(const TokenList *store, Item index)
{
    if (store == NULL || store->items == NULL || index < 0) {
        return NULL;
    }

    size_t position = (size_t)index;
    if (position >= store->count) {
        return NULL;
    }

    return &store->items[position];
}

static ExprStatus enqueue_index(Queue *queue, size_t index)
{
    if (index > (size_t)INT_MAX) {
        return EXPR_ERR_NOMEM;
    }

    DsStatus status = queue_enqueue(queue, (Item)index);
    if (status == DS_OK) {
        return EXPR_OK;
    }
    return status == DS_NOMEM ? EXPR_ERR_NOMEM : EXPR_ERR_INTERNAL;
}

static ExprStatus push_index(Stack *stack, size_t index)
{
    if (index > (size_t)INT_MAX) {
        return EXPR_ERR_NOMEM;
    }

    DsStatus status = stack_push(stack, (Item)index);
    if (status == DS_OK) {
        return EXPR_OK;
    }
    return status == DS_NOMEM ? EXPR_ERR_NOMEM : EXPR_ERR_INTERNAL;
}

/* Vide une file sans toucher aux objets référencés (utile en cas d'erreur). */
static void discard_queue(Queue *queue)
{
    Item item;
    while (queue_dequeue(queue, &item) == DS_OK) {
        /* rien à libérer : les jetons appartiennent à TokenList */
    }
}

static int token_precedence(const Token *token)
{
    if (token == NULL) {
        return 0;
    }

    switch (token->type) {
    case TOK_UMINUS:
    case TOK_UPLUS:
        return 3;
    case TOK_OPERATOR:
        switch (token->value.op) {
        case '^':
            return 4;
        case '*':
        case '/':
            return 2;
        case '+':
        case '-':
            return 1;
        default:
            return 0;
        }
    default:
        return 0;
    }
}

static bool token_is_right_associative(const Token *token)
{
    if (token == NULL) {
        return false;
    }
    if (token->type == TOK_UMINUS || token->type == TOK_UPLUS) {
        return true;
    }
    return token->type == TOK_OPERATOR && token->value.op == '^';
}

/* ------------------------------------------------------------------------- */
/* Partie 3 : vérification du parenthésage                                   */
/* ------------------------------------------------------------------------- */

ExprStatus expr_check_brackets(const char *input, ExprError *error)
{
    expr_error_clear(error);
    if (input == NULL) {
        set_error(error, EXPR_ERR_NULL, 0, "expression NULL");
        return EXPR_ERR_NULL;
    }

    Stack *positions = stack_create();
    if (positions == NULL) {
        set_error(error, EXPR_ERR_NOMEM, 0, "allocation mémoire impossible");
        return EXPR_ERR_NOMEM;
    }

    ExprStatus result = EXPR_OK;

    for (size_t i = 0; input[i] != '\0'; i++) {
        if (input[i] == '(') {
            if (i > (size_t)INT_MAX) {
                set_error(error, EXPR_ERR_NOMEM, i,
                          "expression trop longue pour un indice Item");
                result = EXPR_ERR_NOMEM;
                break;
            }

            DsStatus status = stack_push(positions, (Item)i);
            if (status != DS_OK) {
                set_error(error,
                          status == DS_NOMEM ? EXPR_ERR_NOMEM
                                             : EXPR_ERR_INTERNAL,
                          i, "allocation mémoire impossible");
                result =
                    status == DS_NOMEM ? EXPR_ERR_NOMEM : EXPR_ERR_INTERNAL;
                break;
            }
        } else if (input[i] == ')') {
            Item position;
            DsStatus status = stack_pop(positions, &position);

            if (status == DS_EMPTY) {
                set_error(error, EXPR_ERR_UNBALANCED_CLOSE, i,
                          "parenthèse fermante ')' sans ouvrante");
                result = EXPR_ERR_UNBALANCED_CLOSE;
                break;
            }
            if (status != DS_OK) {
                set_error(error, EXPR_ERR_INTERNAL, i,
                          "erreur interne de pile");
                result = EXPR_ERR_INTERNAL;
                break;
            }
        }
    }

    if (result == EXPR_OK && !stack_is_empty(positions)) {
        /*
         * Il reste des '(' non fermées. En dépilant, les positions sont
         * parcourues de la plus récente à la plus ancienne : la dernière
         * position dépilée est donc la première '(' restée ouverte en lisant
         * de gauche à droite.
         */
        size_t leftmost = 0;
        bool found = false;
        Item position;

        while (stack_pop(positions, &position) == DS_OK) {
            leftmost = (size_t)position;
            found = true;
        }

        if (found) {
            set_error(error, EXPR_ERR_UNBALANCED_OPEN, leftmost,
                      "parenthèse ouvrante '(' non fermée");
            result = EXPR_ERR_UNBALANCED_OPEN;
        }
    }

    stack_destroy(positions);
    return result;
}

/* ------------------------------------------------------------------------- */
/* Partie 4 : analyse lexicale                                               */
/* ------------------------------------------------------------------------- */

ExprStatus expr_tokenize(const char *input, Queue *tokens, TokenList *store,
                         ExprError *error)
{
    expr_error_clear(error);

    if (store != NULL) {
        store->items = NULL;
        store->count = 0;
    }

    if (input == NULL || tokens == NULL || store == NULL) {
        set_error(error, EXPR_ERR_NULL, 0, "argument NULL dans expr_tokenize");
        return EXPR_ERR_NULL;
    }
    if (!queue_is_empty(tokens)) {
        set_error(error, EXPR_ERR_INTERNAL, 0,
                  "la file de jetons doit être vide au départ");
        return EXPR_ERR_INTERNAL;
    }

    size_t length = strlen(input);

    if (length > (size_t)INT_MAX) {
        set_error(error, EXPR_ERR_NOMEM, 0,
                  "expression trop longue pour des indices Item");
        return EXPR_ERR_NOMEM;
    }

    /*
     * Une chaîne de longueur n ne peut pas produire plus de n jetons. On
     * alloue n + 1 pour simplifier, en vérifiant le dépassement de capacité.
     */
    if (length > SIZE_MAX / sizeof(Token) - 1) {
        set_error(error, EXPR_ERR_NOMEM, 0, "expression trop longue");
        return EXPR_ERR_NOMEM;
    }

    Token *items = malloc((length + 1) * sizeof *items);
    if (items == NULL) {
        set_error(error, EXPR_ERR_NOMEM, 0, "allocation mémoire impossible");
        return EXPR_ERR_NOMEM;
    }

    ExprStatus result = EXPR_OK;
    size_t count = 0;
    size_t i = 0;
    bool expect_operand = true;

    while (i < length) {
        unsigned char c = (unsigned char)input[i];

        if (isspace(c)) {
            i++;
            continue;
        }

        /*
         * Un nombre commence par un chiffre, ou par un point suivi d'un
         * chiffre. On ne laisse donc pas strtod accepter "nan" ou "inf".
         */
        if (isdigit(c) ||
            (c == '.' && i + 1 < length &&
             isdigit((unsigned char)input[i + 1]))) {
            /*
             * strtod accepte aussi les littéraux hexadécimaux (0x...). Pour ce
             * TP, seuls les entiers et décimaux décimaux sont autorisés.
             */
            if (c == '0' && i + 1 < length &&
                (input[i + 1] == 'x' || input[i + 1] == 'X')) {
                set_error(error, EXPR_ERR_LEXICAL, i + 1,
                          "caractère invalide '%c'", input[i + 1]);
                result = EXPR_ERR_LEXICAL;
                break;
            }

            errno = 0;
            char *end = NULL;
            double number = strtod(input + i, &end);

            if (end == input + i) {
                set_error(error, EXPR_ERR_LEXICAL, i,
                          "nombre réel invalide");
                result = EXPR_ERR_LEXICAL;
                break;
            }
            if (errno == ERANGE && !isfinite(number)) {
                set_error(error, EXPR_ERR_NONFINITE, i,
                          "nombre réel hors des limites du type double");
                result = EXPR_ERR_NONFINITE;
                break;
            }
            if (*end == '.') {
                set_error(error, EXPR_ERR_LEXICAL,
                          (size_t)(end - input), "nombre mal formé");
                result = EXPR_ERR_LEXICAL;
                break;
            }

            Token *token = &items[count];
            token->type = TOK_NUMBER;
            token->position = i;
            token->value.number = number;

            result = enqueue_index(tokens, count);
            if (result != EXPR_OK) {
                set_error(error, result, i,
                          "allocation mémoire impossible");
                break;
            }

            count++;
            i = (size_t)(end - input);
            expect_operand = false;
            continue;
        }

        if (c == '(') {
            Token *token = &items[count];
            token->type = TOK_LPAREN;
            token->position = i;

            result = enqueue_index(tokens, count);
            if (result != EXPR_OK) {
                set_error(error, result, i,
                          "allocation mémoire impossible");
                break;
            }

            count++;
            i++;
            expect_operand = true;
            continue;
        }

        if (c == ')') {
            Token *token = &items[count];
            token->type = TOK_RPAREN;
            token->position = i;

            result = enqueue_index(tokens, count);
            if (result != EXPR_OK) {
                set_error(error, result, i,
                          "allocation mémoire impossible");
                break;
            }

            count++;
            i++;
            expect_operand = false;
            continue;
        }

        if (c == '+' || c == '-') {
            Token *token = &items[count];

            /*
             * Convention unaire/binaire :
             *   - si un opérande est attendu, le signe est unaire ;
             *   - sinon c'est l'opérateur binaire d'addition/soustraction.
             */
            if (expect_operand) {
                token->type = (c == '-') ? TOK_UMINUS : TOK_UPLUS;
            } else {
                token->type = TOK_OPERATOR;
            }
            token->position = i;
            token->value.op = (char)c;

            result = enqueue_index(tokens, count);
            if (result != EXPR_OK) {
                set_error(error, result, i,
                          "allocation mémoire impossible");
                break;
            }

            count++;
            i++;
            expect_operand = true;
            continue;
        }

        if (c == '*' || c == '/' || c == '^') {
            Token *token = &items[count];
            token->type = TOK_OPERATOR;
            token->position = i;
            token->value.op = (char)c;

            result = enqueue_index(tokens, count);
            if (result != EXPR_OK) {
                set_error(error, result, i,
                          "allocation mémoire impossible");
                break;
            }

            count++;
            i++;
            expect_operand = true;
            continue;
        }

        set_error(error, EXPR_ERR_LEXICAL, i, "caractère invalide '%c'",
                  (char)c);
        result = EXPR_ERR_LEXICAL;
        break;
    }

    if (result != EXPR_OK) {
        /* Les indices partiellement enfilés pointeraient vers items libéré. */
        discard_queue(tokens);
        free(items);
        return result;
    }

    store->items = items;
    store->count = count;
    return EXPR_OK;
}

/* ------------------------------------------------------------------------- */
/* Partie 5 : conversion infixée -> postfixée (shunting-yard)                */
/* ------------------------------------------------------------------------- */

ExprStatus expr_to_postfix(Queue *tokens, Queue *postfix,
                           const TokenList *store, ExprError *error)
{
    expr_error_clear(error);

    if (tokens == NULL || postfix == NULL || store == NULL) {
        set_error(error, EXPR_ERR_NULL, 0, "argument NULL dans expr_to_postfix");
        return EXPR_ERR_NULL;
    }
    if (store->count > 0 && store->items == NULL) {
        set_error(error, EXPR_ERR_INTERNAL, 0,
                  "TokenList incohérente dans expr_to_postfix");
        return EXPR_ERR_INTERNAL;
    }
    if (!queue_is_empty(postfix)) {
        set_error(error, EXPR_ERR_INTERNAL, 0,
                  "la file de sortie doit être vide au départ");
        return EXPR_ERR_INTERNAL;
    }

    Stack *operators = stack_create();
    if (operators == NULL) {
        set_error(error, EXPR_ERR_NOMEM, 0, "allocation mémoire impossible");
        return EXPR_ERR_NOMEM;
    }

    ExprStatus result = EXPR_OK;
    bool expect_operand = true;
    bool have_last = false;
    size_t last_position = 0;
    Item item;
    DsStatus dequeue_status;

    while ((dequeue_status = queue_dequeue(tokens, &item)) == DS_OK) {
        Token *token = token_at(store, item);
        if (token == NULL) {
            set_error(error, EXPR_ERR_INTERNAL, 0, "indice de jeton invalide");
            result = EXPR_ERR_INTERNAL;
            break;
        }

        last_position = token->position;
        have_last = true;

        switch (token->type) {
        case TOK_NUMBER:
            if (!expect_operand) {
                set_error(error, EXPR_ERR_SYNTAX, token->position,
                          "deux opérandes consécutifs : opérateur manquant");
                result = EXPR_ERR_SYNTAX;
                break;
            }

            result = enqueue_index(postfix, (size_t)item);
            if (result != EXPR_OK) {
                set_error(error, result, token->position,
                          "allocation mémoire impossible");
                break;
            }
            expect_operand = false;
            break;

        case TOK_UMINUS:
        case TOK_UPLUS:
            if (!expect_operand) {
                set_error(error, EXPR_ERR_SYNTAX, token->position,
                          "opérateur unaire mal placé");
                result = EXPR_ERR_SYNTAX;
                break;
            }

            /*
             * Un opérateur unaire s'applique immédiatement à l'opérande qui
             * suit : il ne faut donc dépiler aucun opérateur de priorité
             * supérieure. Il sera dépilé plus tard, avant l'opérateur binaire
             * qui le précède logiquement.
             */
            result = push_index(operators, (size_t)item);
            if (result != EXPR_OK) {
                set_error(error, result, token->position,
                          "allocation mémoire impossible");
            }
            break;

        case TOK_OPERATOR: {
            if (expect_operand) {
                set_error(error, EXPR_ERR_SYNTAX, token->position,
                          "opérateur binaire sans opérande à gauche");
                result = EXPR_ERR_SYNTAX;
                break;
            }

            int precedence = token_precedence(token);
            bool right_associative = token_is_right_associative(token);

            while (!stack_is_empty(operators)) {
                Item top_index;
                DsStatus status = stack_top(operators, &top_index);
                if (status != DS_OK) {
                    set_error(error, EXPR_ERR_INTERNAL, token->position,
                              "erreur interne de pile");
                    result = EXPR_ERR_INTERNAL;
                    break;
                }

                Token *top = token_at(store, top_index);
                if (top == NULL) {
                    set_error(error, EXPR_ERR_INTERNAL, token->position,
                              "opérateur invalide dans la pile");
                    result = EXPR_ERR_INTERNAL;
                    break;
                }
                if (top->type == TOK_LPAREN) {
                    break;
                }

                int top_precedence = token_precedence(top);
                bool must_pop =
                    top_precedence > precedence ||
                    (top_precedence == precedence && !right_associative);

                if (!must_pop) {
                    break;
                }

                status = stack_pop(operators, &top_index);
                if (status != DS_OK) {
                    set_error(error, EXPR_ERR_INTERNAL, token->position,
                              "erreur interne de pile");
                    result = EXPR_ERR_INTERNAL;
                    break;
                }

                result = enqueue_index(postfix, (size_t)top_index);
                if (result != EXPR_OK) {
                    set_error(error, result, token->position,
                              "allocation mémoire impossible");
                    break;
                }
            }

            if (result != EXPR_OK) {
                break;
            }

            result = push_index(operators, (size_t)item);
            if (result != EXPR_OK) {
                set_error(error, result, token->position,
                          "allocation mémoire impossible");
                break;
            }

            expect_operand = true;
            break;
        }

        case TOK_LPAREN:
            if (!expect_operand) {
                set_error(error, EXPR_ERR_SYNTAX, token->position,
                          "opérateur manquant avant '('");
                result = EXPR_ERR_SYNTAX;
                break;
            }

            result = push_index(operators, (size_t)item);
            if (result != EXPR_OK) {
                set_error(error, result, token->position,
                          "allocation mémoire impossible");
                break;
            }

            expect_operand = true;
            break;

        case TOK_RPAREN: {
            /*
             * On dépile les opérateurs jusqu'à la '(' correspondante. Si on
             * n'en trouve pas, c'est une fermante orpheline.
             */
            bool found_open = false;

            while (!stack_is_empty(operators)) {
                Item top_index;
                DsStatus status = stack_top(operators, &top_index);
                if (status != DS_OK) {
                    set_error(error, EXPR_ERR_INTERNAL, token->position,
                              "erreur interne de pile");
                    result = EXPR_ERR_INTERNAL;
                    break;
                }

                Token *top = token_at(store, top_index);
                if (top == NULL) {
                    set_error(error, EXPR_ERR_INTERNAL, token->position,
                              "opérateur invalide dans la pile");
                    result = EXPR_ERR_INTERNAL;
                    break;
                }
                if (top->type == TOK_LPAREN) {
                    found_open = true;
                    break;
                }

                status = stack_pop(operators, &top_index);
                if (status != DS_OK) {
                    set_error(error, EXPR_ERR_INTERNAL, token->position,
                              "erreur interne de pile");
                    result = EXPR_ERR_INTERNAL;
                    break;
                }

                result = enqueue_index(postfix, (size_t)top_index);
                if (result != EXPR_OK) {
                    set_error(error, result, token->position,
                              "allocation mémoire impossible");
                    break;
                }
            }

            if (result != EXPR_OK) {
                break;
            }

            if (!found_open) {
                set_error(error, EXPR_ERR_UNBALANCED_CLOSE, token->position,
                          "parenthèse fermante ')' sans ouvrante");
                result = EXPR_ERR_UNBALANCED_CLOSE;
                break;
            }

            if (expect_operand) {
                set_error(error, EXPR_ERR_SYNTAX, token->position,
                          "opérande manquant avant ')'");
                result = EXPR_ERR_SYNTAX;
                break;
            }

            {
                Item open_index;
                if (stack_pop(operators, &open_index) != DS_OK) {
                    set_error(error, EXPR_ERR_INTERNAL, token->position,
                              "erreur interne de pile");
                    result = EXPR_ERR_INTERNAL;
                    break;
                }
            }

            expect_operand = false;
            break;
        }

        default:
            set_error(error, EXPR_ERR_INTERNAL, token->position,
                      "type de jeton inconnu");
            result = EXPR_ERR_INTERNAL;
            break;
        }

        if (result != EXPR_OK) {
            break;
        }
    }

    if (result == EXPR_OK && dequeue_status != DS_EMPTY) {
        set_error(error, EXPR_ERR_INTERNAL, last_position,
                  "erreur interne de file");
        result = EXPR_ERR_INTERNAL;
    }

    if (result == EXPR_OK) {
        if (!have_last) {
            set_error(error, EXPR_ERR_EMPTY, 0, "expression vide");
            result = EXPR_ERR_EMPTY;
        } else {
            /*
             * On vide la pile d'opérateurs. Les '(' restantes signalent des
             * parenthèses non fermées. En cas de plusieurs '(' orphelines, la
             * dernière dépilée est la plus à gauche : on rapporte celle-là.
             */
            bool unbalanced_open = false;
            size_t open_position = 0;

            while (!stack_is_empty(operators)) {
                Item top_index;
                DsStatus status = stack_pop(operators, &top_index);
                if (status != DS_OK) {
                    set_error(error, EXPR_ERR_INTERNAL, last_position,
                              "erreur interne de pile");
                    result = EXPR_ERR_INTERNAL;
                    break;
                }

                Token *top = token_at(store, top_index);
                if (top == NULL) {
                    set_error(error, EXPR_ERR_INTERNAL, last_position,
                              "opérateur invalide dans la pile");
                    result = EXPR_ERR_INTERNAL;
                    break;
                }

                if (top->type == TOK_LPAREN) {
                    unbalanced_open = true;
                    open_position = top->position;
                } else {
                    result = enqueue_index(postfix, (size_t)top_index);
                    if (result != EXPR_OK) {
                        set_error(error, result, last_position,
                                  "allocation mémoire impossible");
                        break;
                    }
                }
            }

            if (result == EXPR_OK) {
                if (unbalanced_open) {
                    set_error(error, EXPR_ERR_UNBALANCED_OPEN, open_position,
                              "parenthèse ouvrante '(' non fermée");
                    result = EXPR_ERR_UNBALANCED_OPEN;
                } else if (expect_operand) {
                    size_t position = last_position;
                    if (position != SIZE_MAX) {
                        position++;
                    }
                    set_error(error, EXPR_ERR_SYNTAX, position,
                              "opérande manquant en fin d'expression");
                    result = EXPR_ERR_SYNTAX;
                }
            }
        }
    }

    stack_destroy(operators);
    return result;
}

/* ------------------------------------------------------------------------- */
/* Partie 6 : évaluation postfixée                                           */
/* ------------------------------------------------------------------------- */

ExprStatus expr_eval_postfix(Queue *postfix, const TokenList *store,
                             double *result, ExprError *error)
{
    expr_error_clear(error);

    if (postfix == NULL || store == NULL || result == NULL) {
        set_error(error, EXPR_ERR_NULL, 0,
                  "argument NULL dans expr_eval_postfix");
        return EXPR_ERR_NULL;
    }
    if (store->count > 0 && store->items == NULL) {
        set_error(error, EXPR_ERR_INTERNAL, 0,
                  "TokenList incohérente dans expr_eval_postfix");
        return EXPR_ERR_INTERNAL;
    }

    DoubleStack values = {NULL, 0, 0};
    ExprStatus status = EXPR_OK;
    bool have_token = false;
    size_t last_position = 0;
    Item item;
    DsStatus dequeue_status;

    while ((dequeue_status = queue_dequeue(postfix, &item)) == DS_OK) {
        Token *token = token_at(store, item);
        if (token == NULL) {
            set_error(error, EXPR_ERR_INTERNAL, 0, "indice de jeton invalide");
            status = EXPR_ERR_INTERNAL;
            break;
        }

        have_token = true;
        last_position = token->position;

        switch (token->type) {
        case TOK_NUMBER:
            if (!double_stack_push(&values, token->value.number)) {
                set_error(error, EXPR_ERR_NOMEM, token->position,
                          "allocation mémoire impossible");
                status = EXPR_ERR_NOMEM;
            }
            break;

        case TOK_UMINUS:
        case TOK_UPLUS: {
            double operand;
            if (!double_stack_pop(&values, &operand)) {
                set_error(error, EXPR_ERR_SYNTAX, token->position,
                          "opérande manquant pour un opérateur unaire");
                status = EXPR_ERR_SYNTAX;
                break;
            }

            double computed =
                token->type == TOK_UMINUS ? -operand : operand;

            if (!double_stack_push(&values, computed)) {
                set_error(error, EXPR_ERR_NOMEM, token->position,
                          "allocation mémoire impossible");
                status = EXPR_ERR_NOMEM;
            }
            break;
        }

        case TOK_OPERATOR: {
            double right_value;
            double left_value;

            /*
             * Ordre important : le premier dépilé est l'opérande droit (b),
             * le second est l'opérande gauche (a).
             */
            if (!double_stack_pop(&values, &right_value)) {
                set_error(error, EXPR_ERR_SYNTAX, token->position,
                          "opérande droit manquant pour '%c'",
                          token->value.op);
                status = EXPR_ERR_SYNTAX;
                break;
            }
            if (!double_stack_pop(&values, &left_value)) {
                set_error(error, EXPR_ERR_SYNTAX, token->position,
                          "opérande gauche manquant pour '%c'",
                          token->value.op);
                status = EXPR_ERR_SYNTAX;
                break;
            }

            double a = left_value;
            double b = right_value;
            double computed = 0.0;

            switch (token->value.op) {
            case '+':
                computed = a + b;
                break;
            case '-':
                computed = a - b;
                break;
            case '*':
                computed = a * b;
                break;
            case '/':
                if (b == 0.0) {
                    set_error(error, EXPR_ERR_DIVISION_BY_ZERO,
                              token->position, "division par zéro");
                    status = EXPR_ERR_DIVISION_BY_ZERO;
                    break;
                }
                computed = a / b;
                break;
            case '^':
                computed = pow(a, b);
                break;
            default:
                set_error(error, EXPR_ERR_INTERNAL, token->position,
                          "opérateur inconnu '%c'", token->value.op);
                status = EXPR_ERR_INTERNAL;
                break;
            }

            if (status != EXPR_OK) {
                break;
            }

            if (!isfinite(computed)) {
                set_error(error, EXPR_ERR_NONFINITE, token->position,
                          "résultat infini ou NaN");
                status = EXPR_ERR_NONFINITE;
                break;
            }

            if (!double_stack_push(&values, computed)) {
                set_error(error, EXPR_ERR_NOMEM, token->position,
                          "allocation mémoire impossible");
                status = EXPR_ERR_NOMEM;
            }
            break;
        }

        case TOK_LPAREN:
        case TOK_RPAREN:
            set_error(error, EXPR_ERR_INTERNAL, token->position,
                      "parenthèse inattendue dans une expression postfixée");
            status = EXPR_ERR_INTERNAL;
            break;

        default:
            set_error(error, EXPR_ERR_INTERNAL, token->position,
                      "type de jeton inconnu");
            status = EXPR_ERR_INTERNAL;
            break;
        }

        if (status != EXPR_OK) {
            break;
        }
    }

    if (status == EXPR_OK && dequeue_status != DS_EMPTY) {
        set_error(error, EXPR_ERR_INTERNAL, last_position,
                  "erreur interne de file");
        status = EXPR_ERR_INTERNAL;
    }

    if (status == EXPR_OK) {
        if (!have_token) {
            set_error(error, EXPR_ERR_EMPTY, 0, "expression postfixée vide");
            status = EXPR_ERR_EMPTY;
        } else if (values.size != 1) {
            set_error(error, EXPR_ERR_SYNTAX, last_position,
                      "expression postfixée mal formée : %zu opérande(s) "
                      "restant(s)",
                      values.size);
            status = EXPR_ERR_SYNTAX;
        } else {
            double computed;
            if (!double_stack_pop(&values, &computed)) {
                set_error(error, EXPR_ERR_INTERNAL, last_position,
                          "erreur interne de pile");
                status = EXPR_ERR_INTERNAL;
            } else if (!isfinite(computed)) {
                set_error(error, EXPR_ERR_NONFINITE, last_position,
                          "résultat infini ou NaN");
                status = EXPR_ERR_NONFINITE;
            } else {
                *result = computed;
            }
        }
    }

    double_stack_destroy(&values);
    return status;
}

/* ------------------------------------------------------------------------- */
/* Partie 7 : fonction de haut niveau                                        */
/* ------------------------------------------------------------------------- */

ExprStatus expr_evaluate(const char *input, double *result, ExprError *error)
{
    expr_error_clear(error);

    if (input == NULL || result == NULL) {
        set_error(error, EXPR_ERR_NULL, 0, "argument NULL dans expr_evaluate");
        return EXPR_ERR_NULL;
    }

    bool only_spaces = true;
    for (const char *p = input; *p != '\0'; p++) {
        if (!isspace((unsigned char)*p)) {
            only_spaces = false;
            break;
        }
    }
    if (only_spaces) {
        set_error(error, EXPR_ERR_EMPTY, 0, "expression vide");
        return EXPR_ERR_EMPTY;
    }

    ExprStatus status = EXPR_OK;
    Queue *token_queue = queue_create();
    Queue *postfix_queue = queue_create();
    TokenList store;
    store.items = NULL;
    store.count = 0;

    if (token_queue == NULL || postfix_queue == NULL) {
        set_error(error, EXPR_ERR_NOMEM, 0, "allocation mémoire impossible");
        status = EXPR_ERR_NOMEM;
        goto cleanup;
    }

    /*
     * On analyse d'abord les caractères : une erreur lexicale (par exemple
     * '$') doit être signalée avant une éventuelle erreur de parenthésage
     * située plus loin dans la chaîne.
     */
    status = expr_tokenize(input, token_queue, &store, error);
    if (status != EXPR_OK) {
        goto cleanup;
    }

    status = expr_check_brackets(input, error);
    if (status != EXPR_OK) {
        goto cleanup;
    }

    status = expr_to_postfix(token_queue, postfix_queue, &store, error);
    if (status != EXPR_OK) {
        goto cleanup;
    }

    status = expr_eval_postfix(postfix_queue, &store, result, error);

cleanup:
    queue_destroy(token_queue);
    queue_destroy(postfix_queue);
    token_list_free(&store);
    return status;
}

void token_list_free(TokenList *store)
{
    if (store == NULL) {
        return;
    }
    free(store->items);
    store->items = NULL;
    store->count = 0;
}
