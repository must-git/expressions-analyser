#ifndef TP5_EXPR_H
#define TP5_EXPR_H

#include <stdbool.h>
#include <stddef.h>

#include "queue.h"

/*
 * Mini-analyseur d'expressions arithmétiques.
 *
 * Chaîne de traitement :
 *
 *   texte
 *     -> expr_check_brackets  (validation des parenthèses)
 *     -> expr_tokenize        (analyse lexicale)
 *     -> expr_to_postfix      (shunting-yard)
 *     -> expr_eval_postfix    (évaluation avec une pile de doubles)
 *
 * Les fibres et les piles du TAD manipulent des Item, c'est-à-dire des int.
 * Les files de jetons contiennent donc des indices vers TokenList.items et
 * non des pointeurs. L'appelant doit garder la TokenList vivante tant que les
 * files sont utilisées.
 *
 * Convention de positions : toutes les positions sont des indices d'octets
 * dans la chaîne d'origine, en commençant à 0.
 *
 * Cas particuliers documentés :
 *   - caractère invalide        : position du caractère fautif ;
 *   - ')' sans '(' correspondant: position de la ')' fautive ;
 *   - '(' non fermée à la fin   : position de la première '(' non fermée en
 *                                 lisant de gauche à droite ;
 *   - opérateur en fin           : position située juste après cet opérateur
 *                                 (c'est là où un opérande est attendu).
 */

typedef enum token_type {
    TOK_NUMBER = 0, /* nombre réel                      */
    TOK_OPERATOR,   /* opérateur binaire + - * / ^       */
    TOK_UMINUS,     /* moins unaire, distinct de '-'     */
    TOK_UPLUS,      /* plus unaire                       */
    TOK_LPAREN,     /* '('                               */
    TOK_RPAREN      /* ')'                               */
} TokenType;

typedef struct token {
    TokenType type;
    size_t position;
    union {
        double number; /* valide si type == TOK_NUMBER          */
        char op;       /* valide si type == TOK_OPERATOR        */
    } value;
} Token;

/*
 * Stockage possédant tous les jetons d'une analyse.
 *
 * Les files de jetons contiennent seulement des indices (Item) vers les
 * éléments de items. Ainsi, un jeton n'est alloué qu'une seule fois et reste
 * valide même si la file grandit.
 */
typedef struct token_list {
    Token *items;
    size_t count;
} TokenList;

typedef enum expr_status {
    EXPR_OK = 0,
    EXPR_ERR_NULL,             /* argument NULL                       */
    EXPR_ERR_EMPTY,            /* expression vide                     */
    EXPR_ERR_LEXICAL,          /* caractère invalide                  */
    EXPR_ERR_UNBALANCED_OPEN,  /* '(' non fermée                      */
    EXPR_ERR_UNBALANCED_CLOSE, /* ')' sans '(' correspondant          */
    EXPR_ERR_SYNTAX,           /* expression mal formée               */
    EXPR_ERR_DIVISION_BY_ZERO, /* division par zéro                   */
    EXPR_ERR_NONFINITE,        /* résultat infini ou NaN              */
    EXPR_ERR_NOMEM,            /* allocation mémoire impossible       */
    EXPR_ERR_INTERNAL          /* incohérence interne                 */
} ExprStatus;

#define EXPR_ERROR_MESSAGE_SIZE 160

typedef struct expr_error {
    ExprStatus status;
    size_t position;
    char message[EXPR_ERROR_MESSAGE_SIZE];
} ExprError;

const char *expr_status_str(ExprStatus status);
const char *token_type_str(TokenType type);

/* Remet une structure d'erreur à l'état EXPR_OK. Accepter NULL. */
void expr_error_clear(ExprError *error);

/*
 * Vérifie l'équilibrage des parenthèses uniquement.
 * Retourne EXPR_OK, EXPR_ERR_UNBALANCED_CLOSE ou EXPR_ERR_UNBALANCED_OPEN.
 * Les caractères autres que '(' et ')' sont ignorés.
 */
ExprStatus expr_check_brackets(const char *input, ExprError *error);

/*
 * Découpe input en jetons et les place dans la file tokens.
 *
 * Préconditions :
 *   - tokens doit être une file vide créée par queue_create() ;
 *   - store pointe sur une structure non initialisée (elle sera remplie).
 *
 * Postconditions en cas de succès :
 *   - store->items possède store->count jetons ;
 *   - tokens contient store->count indices ; l'indice i désigne
 *     store->items[i].
 *
 * L'appelant doit garder store vivant tant que la file est utilisée, puis
 * appeler token_list_free(&store) et queue_destroy(tokens).
 *
 * En cas d'erreur, aucune ressource n'est laissée à libérer par l'appelant.
 */
ExprStatus expr_tokenize(const char *input, Queue *tokens, TokenList *store,
                         ExprError *error);

/*
 * Convertit la file de jetons en notation postfixée (algorithme shunting-yard).
 *
 * La file tokens est consommée et la file postfix doit être vide au départ.
 * store doit être la TokenList utilisée par expr_tokenize().
 * En cas d'erreur, postfix peut contenir une sortie partielle : il faut la
 * détruire, ce qui libère uniquement les nœuds de file (pas les jetons).
 */
ExprStatus expr_to_postfix(Queue *tokens, Queue *postfix,
                           const TokenList *store, ExprError *error);

/*
 * Évalue une file postfixée d'indices vers Token.
 * La file postfix est consommée. store doit rester valide pendant l'appel.
 * En cas de succès, *result reçoit la valeur et la pile interne est vide.
 */
ExprStatus expr_eval_postfix(Queue *postfix, const TokenList *store,
                             double *result, ExprError *error);

/*
 * Fonction de haut niveau : analyse et évalue directement une expression.
 * Elle enchaîne validation des parenthèses, tokenisation, conversion et
 * évaluation, puis libère toutes les ressources internes.
 */
ExprStatus expr_evaluate(const char *input, double *result, ExprError *error);

/* Libère store->items et remet store à zéro. Accepter NULL. */
void token_list_free(TokenList *store);

#endif /* TP5_EXPR_H */
