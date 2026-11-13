#ifndef SLIDING_MAX_H
#define SLIDING_MAX_H

#include <stddef.h>

#include "ds_status.h"

/*
 * Calcule le maximum de chaque fenêtre glissante de taille window.
 * out doit pouvoir recevoir length - window + 1 éléments.
 * Retourne DS_BADARG si un pointeur est NULL, si window == 0 ou window > length.
 */
DsStatus sliding_max_compute(const Item *values, size_t length, size_t window,
                             Item *out);

#endif /* SLIDING_MAX_H */
