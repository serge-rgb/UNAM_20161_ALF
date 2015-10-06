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
 *  Las líneas no necesariamente tienen el mismo número de columnas.
 *  El primer estado tiene que ser 1
 *
 *  Donde:
 *      ESTADO:     numero entero positivo. Denotando el estado. -1 para el estado error, pero no es necesario
 *      ENTRADA:    Caracter en ASCII (un byte)
 *      ESTADO:     Resultado de la función de transición
 *      FINAL:      0 para no-final. 1 para final.
 *
 *  La definicion del autómata se deriva de este formato de la siguiente manera:
 *
 *      Alfabeto: Es el conjunto de caracteres ASCII para los que existe una transición
 */

#define MAX_ALFABETO NUM_ASCII_CHARS
#define MAX_NUM_ESTADOS 64
#define NUM_ASCII_CHARS 128

static int g_AF[MAX_NUM_ESTADOS][MAX_ALFABETO];
static char g_alfabeto[NUM_ASCII_CHARS];
static int g_finales[MAX_NUM_ESTADOS];

// Solo un alias para ser mas claro.
typedef int Estado;

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

int main(int argc, char** argv)
{

    static char* test_fa [] = {
        "af0.csv",
    };


    for (int32_t i = 0; i < sgl_array_count(test_fa); ++i) {
        int64_t read = 0;
        // -- Nuevo archivo:
        // Inicializar el automata finito para dejarlo en valores invalidos, antes de cargar el archivo.
        memset(g_AF, -1, sizeof(int)*MAX_NUM_ESTADOS * MAX_ALFABETO);
        memset(g_alfabeto, 0, NUM_ASCII_CHARS);
        memset(g_finales, -1, sizeof(int) * MAX_NUM_ESTADOS);

        char* contents = sgl_slurp_file(test_fa[i], &read);
        if (read) {
            int32_t num_lines;
            char** lines = sgl_split_lines(contents, &num_lines);
            if (lines) {
                // Ahora interpretamos la linea actual para llenar nuestra tabla.
                // Primera linea, llenar los contadores
                for (int32_t line_i = 0; line_i < num_lines; ++line_i) {
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
                Estado* alcanzables = NULL;
                sb_push(alcanzables, 1);  // El estado inicial es alcanzable
                sglBool fijo = 0;
                while ( !fijo ) {
                    fijo = 1;
                    for (int qi = 0; qi < sb_count(alcanzables); ++qi) {
                        Estado q = alcanzables[qi];
                        for (int ai = 0; ai < sb_count(alfabeto); ++ai) {
                            char a = alfabeto[ai];
                            Estado p = g_AF[q][a];
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
                    Estado q = alcanzables[qi];
                    sgl_log("%d", q);
                    if (qi == sb_count(alcanzables) - 1) {
                        sgl_log("\n");
                    } else {
                        sgl_log(", ");
                    }
                }

                // Tabla inicialmente en zeros, de estados distinguibles
                int* distinguibles = (int*) sgl_calloc(MAX_NUM_ESTADOS * MAX_NUM_ESTADOS, sizeof(int));
#define son_distinguibles(p, q) distinguibles[min(p, q) * MAX_NUM_ESTADOS + max(p, q)]  // Macro util para evitar confusiones..

                // Llenar la tabla distinguiendo entre finales y no finales.
                for (int qi = 0; qi < sb_count(alcanzables) - 1; ++qi) {
                    for (int pi = qi + 1; pi < sb_count(alcanzables); ++pi) {
                        Estado p = alcanzables[pi];
                        Estado q = alcanzables[qi];
                        if (g_finales[q] != g_finales[p]) {
                            son_distinguibles(p, q) = 1;
                            sgl_log("%d y %d son dist.\n", p, q);
                        }
                    }
                }
                for (int qi = 0; qi < sb_count(alcanzables) ; ++qi) {
                    son_distinguibles(qi, qi) = 0;
                }

                // Punto fijo.
                fijo = 0;
                while (!fijo) {
                    fijo = 1;
                    for (int qi = 0; qi < sb_count(alcanzables) - 1; ++qi) {
                        for (int pi = qi + 1; pi < sb_count(alcanzables); ++pi) {
                            Estado p = alcanzables[pi];
                            Estado q = alcanzables[qi];
                            if ( !son_distinguibles(q, p) ) {
                                for (int ai = 0; ai < sb_count(alfabeto); ++ai) {
                                    char a = alfabeto[ai];
                                    Estado pa = g_AF[p][a];
                                    Estado qa = g_AF[q][a];
                                    if ( son_distinguibles(pa, qa) ) {
                                        son_distinguibles(p, q) = 1;
                                        // p y q son distinguibles porque d(p, a) y d(q, a) son distinguibles.
                                        fijo = 0;
                                    }
                                }
                            }
                        }
                    }

                }

                // Ver quienes *no* son distinguibles y crear clases

#define agregar_sin_repetir(c, p) if (!sb_find((c), p)) { sb_push((c), p); }

                int* clases[MAX_NUM_ESTADOS] = { 0 };
                int clases_c = 0;

                // Agregar al estado incicial
                {
                    int* nueva_clase = NULL;
                    sb_push(nueva_clase, 1);
                    clases[clases_c++] = nueva_clase;
                }

                // Punto fijo
                fijo = 0;
                while (!fijo) {
                    fijo = 1;
                    for (int pi = 0; pi < sb_count(alcanzables); ++pi) {
                        Estado p = alcanzables[pi];
                        // Buscar en las clases de equivalencia para ver si creamos una nueva o agregamos a existente.
                        int esta_en_clase = 0;
                        for (int ci = 0; ci < clases_c; ++ci) {
                            Estado p_clase = clases[ci][0];
                            if ( !son_distinguibles(p_clase, p) ) {
                                // Agregar p y q a la clase existente.
                                if (!sb_find(clases[ci], p)) {
                                    sb_push(clases[ci], p);
                                    fijo = 0;
                                }
                                esta_en_clase = 1;
                            }
                        }
                        if (!esta_en_clase) {
                            fijo = 0;
                            // Crear nueva clase!
                            int* nueva_clase = NULL;
                            sb_push(nueva_clase, p);
                            // Agregar a esta nueva clase
                            clases[clases_c++] = nueva_clase;
                        }
                    }
                }
                // Imprimir clases de equivalencia
                for (int ci = 0; ci < clases_c; ++ci) {
                    sgl_log("Clase q%d: [", ci+1);
                    int* clase = clases[ci];
                    for (int ei = 0; ei < sb_count(clase); ++ei) {
                        sgl_log ("%d", clase[ei]);
                        if (ei == sb_count(clase) - 1) {
                            sgl_log("]\n");
                        } else {
                            sgl_log(", ");

                        }
                    }
                }

                // Por ultimo, imprimir la nueva funcion de transicion.
                for (int ci = 0; ci < clases_c; ++ci) {
                    int* clase = clases[ci];
                    for (int ai = 0; ai < sb_count(alfabeto); ++ai) {
                        char a = alfabeto[ai];

                        // Encontrar la clase a la que corresponde.
                        Estado p = clase[0];
                        Estado q = g_AF[p][a];
                        for (int di = 0; di < clases_c; ++di) {
                            if (sb_find(clases[di], q)) {
                                sgl_log("d(q%d, %c) = q%d\n", ci + 1, a, di+1);
                            }
                        }
                    }
                }
            }
        }
    }
    return EXIT_SUCCESS;
}
