#include <mpi.h>
#include <iostream>
#include <vector>
#include <list>
using namespace std;

int main (int argc, char *argv[]) {
    int rank, size;
    int master = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == master) {
        // TODO
        // Leer matriz adyacencia
        // Broadcast del número de vertices y la matriz
        // Setear estado de procesos (ocupado o libre)
        // Inicializar pathTSP y metada
        // Enviar pathTSP y metadata
        // Mientras haya algun proceso trabajando seguir computando el TSP
        // Detener procesos esclavos
        // Imprimir costo minimo
        // Imprimir ruta optima TSP
    }
    else { // Los otros procesos
        // Reciben el numero de vertices y la matriz de costos
        // Creamos la lista de vectores de rutas
        // Mientras no recibemos un stop del maestro seguimos trabajando
        // Recibimos una orden del maestro
        // Caso1: Terminar
        // Caso2: Descender en el arbol
            // (while) Procesamos los vectores (rutas) de la lista
            // Obtener el lower_bound (costo min hasta ese momento)
            // Actualizamos el min costo global de ser el caso
        // Indicar status libre del proceso y notificar al master
    }
    return 0;
}