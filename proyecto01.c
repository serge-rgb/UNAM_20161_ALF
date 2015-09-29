#define LIBSERG_IMPLEMENTATION
#include "libserg/libserg.h"

/**
 *
 * Proyecto 1 -- Reducir un autómata finito determinista.
 *  Autómatas y Lenguajes Formales
 *      Sergio Gonzalez
 *
 *
 * Definicion de un AF en CSV (Comma separated values):
 *
 *      ESTADO, [ENTRADA, ESTADO]+, FINAL
 *  Todas las líneas tienen el mismo número de columnas.
 *
 *  Donde:
 *      ESTADO:     numero entero positivo. Denotando el estado. -1 para el estado error
 *      ENTRADA:    Caracter en ASCII (un byte)
 *      ESTADO:     Resultado de la función de transición
 *      FINAL:      0 para no-final. 1 para final.
 *
 *  La definicion del autómata se deriva de este formato de la siguiente manera:
 *
 *      Alfabeto: Es el conjunto de caracteres ASCII para los que existe una transición
 */

int main(int argc, char** argv)
{
    static char* test_fa [] = {
        "af0.csv",
    };

    for (int32_t i = 0; i < sgl_array_count(test_fa); ++i) {
        int64_t read = 0;
        char* contents = sgl_slurp_file(test_fa[i], &read);
        if (read) {
            int32_t num_lines;
            char** lines = sgl_split_lines(contents, &num_lines);
            if (lines) {
                for (int32_t line_i = 0; line_i < num_lines; ++line_i) {
                    puts(lines[line_i]);
                }
            }
        }
    }
}
