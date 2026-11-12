#ifndef DS_STATUS_H
#define DS_STATUS_H

/* Code de retour commun à toutes les structures du chapitre. */
typedef enum {
    DS_OK = 0,   /* opération réussie                              */
    DS_EMPTY,    /* précondition violée : structure vide           */
    DS_NOMEM,    /* échec d'allocation (malloc/realloc)            */
    DS_SYNTAX,   /* expression mal formée (analyseur d'expressions) */
    DS_DIVZERO,  /* division par zéro (évaluation)                 */
    DS_BADARG    /* argument invalide (paramètre hors domaine)     */
} DsStatus;

/* Type des éléments : un seul endroit à modifier pour changer de type. */
typedef int Item;

#endif /* DS_STATUS_H */