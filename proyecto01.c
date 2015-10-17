#define LIBSERG_IMPLEMENTATION
#include "libserg.h"

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
 *  - Las líneas no necesariamente tienen el mismo número de columnas.
 *  - El estado inicial tiene que ser 1
 *  - Se pueden hacer comentarios iniciando la linea con #
 *
 *  Donde:
 *      ESTADO:     numero entero positivo. Denotando el estado. -1 para el estado error, pero no es necesario
 *      ENTRADA:    Caracter en ASCII (un byte)
 *      ESTADO:     Resultado de la función de transición
 *      FINAL:      0 para no-final. 1 para final.
 *
 *
 * ... nota. uso libserg para
 */

#define MAX_ALFABETO NUM_ASCII_CHARS
#define MAX_NUM_ESTADOS 64
#define NUM_ASCII_CHARS 128

static int g_AF[MAX_NUM_ESTADOS][MAX_ALFABETO];
static char g_alfabeto[NUM_ASCII_CHARS];
static int g_finales[MAX_NUM_ESTADOS];

// Maquina de estados para interpretar las lineas de los archivos csv
enum {
    PARSE_estado,
    PARSE_entrada,
    PARSE_trans,
    PARSE_final
};

void panico(char* m)
{
    sgl_log("%s\n", m);
    exit(EXIT_FAILURE);
}
#ifndef min
#define min(a, b) ( (a) < (b) ) ? a : b
#endif
#ifndef max
#define max(a, b) ( (a) > (b) ) ? a : b
#endif

// Funcion extra de ayuda
int sb_find(int* a, int e)
{
    for(int i = 0; i < sb_count((a)); ++i) {
        if (a[i] == e)
            return 1;
    }
    return 0;
}

char* finalizar_tabla()
{
    // marcar estado error como no-final.
    g_finales[0] = 0;
    char* mi_alfabeto = NULL;
    int c_alfabeto = 0;
    for(int i = 0; i < NUM_ASCII_CHARS; ++i) {
        if (g_alfabeto[i] == 1) {
            c_alfabeto++;
            sb_push(mi_alfabeto, (char)i);
        }
    }
    sgl_log("El alfabeto es: ");
    for (int i = 0; i < c_alfabeto; ++i) {
        sgl_log("%c", mi_alfabeto[i]);
        if (i < c_alfabeto - 1) {
            sgl_log(", ");
        } else {
            sgl_log("\n");
        }
    }
    for (int qi = 0; qi < MAX_NUM_ESTADOS; ++qi) {
        if (g_finales[qi] >= 0) {
            for (int ai = 0; ai < sb_count(mi_alfabeto); ++ai) {
                char a = mi_alfabeto[ai];
                sgl_log("d(%d, %c) = %d (F=%d)\n",
                        qi, a,
                        g_AF[qi][a],
                        g_finales[qi]);
            }
        }
    }
    return mi_alfabeto;
}

void marcar_distinguibles(int* tabla, int p, int q)
{
    int M = max(p, q);
    int m = min(p, q);
    tabla[m * MAX_NUM_ESTADOS + M] = 1;
}

int son_distinguibles(int* tabla, int p, int q)
{
    if ( p == q ) {
        return 0;
    }
    int M = max(p, q);
    int m = min(p, q);
    int res = tabla[m * MAX_NUM_ESTADOS + M];

    return res;
}

// d: tabla de distinguibilidad
// clase, tam. Arreglo de estados equivalentes.
// p: Estado candidato.
int pertenece_a_clase(int* d, int* clase, int tam, int p)
{
    for ( int ci = 0; ci < tam; ++ci ) {
        int q = clase[ci];
        if ( !son_distinguibles(d, p, q) ) {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char** argv)
{
    static char* test_fa [] = {
        "af0.csv",
        "af1.csv",
    };


    for (int32_t i = 0; i < sgl_array_count(test_fa); ++i) {
        int64_t read = 0;
        // -- Nuevo archivo:
        // Inicializar el automata finito para dejarlo en valores invalidos, antes de cargar el archivo.
        memset(g_AF, 0, sizeof(int)*MAX_NUM_ESTADOS * MAX_ALFABETO);
        memset(g_alfabeto, 0, NUM_ASCII_CHARS);
        memset(g_finales, -1, sizeof(int) * MAX_NUM_ESTADOS);

        sgl_log("\n\n***** Procesando archivo %s *****\n", test_fa[i]);

        char* contents = sgl_slurp_file(test_fa[i], &read);
        if (read) {
            int32_t num_lines;
            char** lines = sgl_split_lines(contents, &num_lines);
            if (lines) {
                // Ahora interpretamos la linea actual para llenar nuestra tabla.
                // Primera linea, llenar los contadores
                for (int32_t line_i = 0; line_i < num_lines; ++line_i) {
                    if ( lines[line_i][0] == '#' ) {
                        // Es un comentario.
                        continue;
                    }
                    int parse_state = PARSE_estado;
                    char** toks = sgl_tokenize(lines[line_i], ",");
                    int estado = -1;
                    char entrada_actual = 0;
                    for (int tok_i = 0; tok_i < sb_count(toks); ++tok_i) {
                        char* tok = sgl_strip_whitespace(toks[tok_i]);

                        switch (parse_state) {
                        case PARSE_estado: {
                                if (!sgl_is_number(tok)) {
                                    panico("El estado no se define correctamente.");
                                }
                                estado = atoi(tok);
                                if ( estado <= 0 || estado >= MAX_NUM_ESTADOS ) {
                                    panico("Estado invalido\n");
                                }
                                if (line_i == 0 && estado != 1) {
                                    panico("El primer estado tiene que ser 1");
                                }
                                parse_state = PARSE_entrada;
                                break;
                            }
                        case PARSE_entrada: {
                            if ( sgl_is_number(tok) ) {
                                // Al recibir un numbero en lugar de una letra, asumimos que es final
                                int final = atoi(tok);
                                if (final == 0 || final == 1 ) {
                                    g_finales[estado] = final;
                                    parse_state = PARSE_final;
                                } else {
                                    panico("Definicion de final tiene que ser 0 o 1.");
                                }
                            } else if (strlen(tok) == 1){
                                g_alfabeto[tok[0]] = 1;  // Marcar este caracter como "en el alfabeto"
                                entrada_actual = tok[0];
                                parse_state = PARSE_trans;
                            } else {
                                panico("entrada no bien definida (debe ser un caracter ascii no numerico)");
                            }
                            break;
                        }
                        case PARSE_trans: {
                            if ( !sgl_is_number(tok) ) {
                                panico("Las transiciones deben ser numeros positivos (estados).");
                            } else {
                                int e = atoi(tok);
                                if (e > 0) {
                                    g_AF[estado][entrada_actual] = e;
                                }
                                parse_state = PARSE_entrada;
                            }
                            break;
                        }
                        case PARSE_final: {
                            panico("Mas datos en el archivo de los esperados");
                            break;
                        }
                        }
                    }
                }
                // Llenar estados error.
                char* alfabeto = finalizar_tabla();

                // Marcar alcanzables
                int* alcanzables = NULL;
                sb_push(alcanzables, 1);  // El estado inicial es alcanzable
                int fijo = 0;
                while ( !fijo ) {
                    fijo = 1;
                    for (int qi = 0; qi < sb_count(alcanzables); ++qi) {
                        int q = alcanzables[qi];
                        for (int ai = 0; ai < sb_count(alfabeto); ++ai) {
                            char a = alfabeto[ai];
                            int p = g_AF[q][a];
                            if ( g_AF[q][a] != -1 && !sb_find(alcanzables, p)) {
                                // Encontramos un nuevo estado alcanzable.
                                sb_push(alcanzables, p);
                                fijo = 0;
                            }
                        }
                    }
                }
                sgl_log("Alcanzables: ");
                for (int qi = 0; qi < sb_count(alcanzables); ++qi) {
                    int q = alcanzables[qi];
                    sgl_log("%d", q);
                    if (qi == sb_count(alcanzables) - 1) {
                        sgl_log("\n");
                    } else {
                        sgl_log(", ");
                    }
                }

                // Tabla inicialmente en zeros, de estados distinguibles
                int* distinguibles = (int*) sgl_calloc(MAX_NUM_ESTADOS * MAX_NUM_ESTADOS, sizeof(int));

                int ac = sb_count(alcanzables);

                // Marcar finales y no finales como distinguibles.
                for ( int pi = 0; pi < ac; ++pi ) {
                    for ( int qi = pi + 1; qi < ac; ++qi ) {
                        int p = alcanzables[pi];
                        int q = alcanzables[qi];
                        if ( g_finales[p] != g_finales[q] ) {
                            marcar_distinguibles(distinguibles, p, q);
                        }
                    }
                }

                // Punto fijo: marcar (q,p) como distinguibles si d(p,a) y
                // d(q,a) son distinguibles para a en el alfabeto

                fijo = 0;
                while (!fijo) {
                    fijo = 1;
                    for ( int pi = 0; pi < ac; ++pi ) {
                        for ( int qi = pi + 1; qi < ac; ++qi ) {
                            int p = alcanzables[pi];
                            int q = alcanzables[qi];
                            if ( !son_distinguibles(distinguibles, p, q) ) {
                                for ( int ai = 0; ai < sb_count(alfabeto); ++ai ) {
                                    char a = alfabeto[ai];
                                    int pa = g_AF[p][a];
                                    int qa = g_AF[q][a];
                                    if ( son_distinguibles(distinguibles, pa, qa) ) {
                                        fijo = 0;
                                        marcar_distinguibles(distinguibles, p, q);
                                    }
                                }
                            }
                        }
                    }
                }

                // Imprimir informacion de estados equivalentes..
                for ( int pi = 0; pi < ac; ++pi ) {
                    for ( int qi = pi + 1; qi < ac; ++qi ) {
                        int p = alcanzables[pi];
                        int q = alcanzables[qi];
                        if ( !son_distinguibles(distinguibles, p, q) ) {
                            sgl_log("%d y %d son equivalentes\n", p, q);
                        }
                    }
                }

                // Crear clases.
                int* clases[MAX_NUM_ESTADOS] = { 0 };
                int num_clases = 0;

                // La primera clase tiene al estado inicial.
                int* nueva_clase = NULL;
                sb_push(nueva_clase, 1);
                clases[num_clases++] = nueva_clase;

                // Iterar por estados. Crear nuevas clases o agregar estados
                // equivalentes a las clases existentes.
                fijo = 0;
                while ( !fijo ) {
                    fijo = 1;
                    for ( int pi = 0; pi < ac; ++pi ) {
                        int p = alcanzables[pi];
                        int pertenece = 0;
                        for ( int ci = 0; ci < num_clases; ++ci ) {
                            if ( !sb_find(clases[ci], p) ) {
                                // Si no esta en la clase...
                                if ( pertenece_a_clase(distinguibles, clases[ci], sb_count(clases[ci]), p) ) {
                                    // ... pero pertenece, agregar.
                                    pertenece = 1;
                                    sb_push(clases[ci], p);
                                    fijo = 0;
                                }
                            } else {
                                pertenece = 1;
                            }
                        }

                        if ( !pertenece ) {
                            int* nc = NULL;
                            sb_push(nc, p);
                            clases[num_clases++] = nc;
                            fijo = 0;
                        }
                    }
                }

                // Para hacer las cosas mas legibles, encontrar la clase que tiene el estado error...
                int clase_error = -1;
                for ( int ci = 0; ci < num_clases; ++ci ) {
                    if ( sb_find(clases[ci], 0) ) {
                        clase_error = ci;
                        break;
                    }
                }

                // Imprimir el nuevo autómata.

                sgl_log ("    ==== El automata minimizado ====\n");

                for ( int ci = 0; ci < num_clases; ++ci ) {
                    if ( ci == clase_error ) {
                        continue;
                    }
                    int* clase = clases[ci];
                    int p = clase[0];  // Solo nos interesa un elemento, para ver a donde va.
                    for ( int ai = 0; ai < sb_count(alfabeto); ++ai ) {
                        char a = alfabeto[ai];
                        int q = g_AF[p][a];
                        // Encontrar la clase de q;
                        int transicion = -1;
                        for ( int cii = 0; cii < num_clases; ++cii ) {
                            if ( sb_find(clases[cii], q) ) {
                                transicion = cii;
                                break;
                            }
                        }
                        // Imprimir.
                        if ( transicion != clase_error ) {
                            sgl_log("d(q%d, %c) = q%d\n", ci, a, transicion);
                        } else {
                            sgl_log("d(q%d, %c) = E\n", ci, a);
                        }
                    }
                }
                // Imprimir las transiciones del estado error.
                for ( int ai = 0; ai < sb_count(alfabeto); ++ai ) {
                    sgl_log("d(E, %c) = E\n", alfabeto[ai]);
                }

                // Indicar los estados finales.
                sgl_log("Estados finales: [ ");
                for ( int ci = 0; ci < num_clases; ++ci ) {
                    int p = clases[ci][0];
                    if ( g_finales[p] ) {
                        sgl_log("q%d ", ci);
                    }
                }
                sgl_log("]\n");
            }
        }
    }
    return EXIT_SUCCESS;
}
