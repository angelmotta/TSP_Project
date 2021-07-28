#include <mpi.h>
#include <iostream>
#include <vector>
#include <list>

using namespace std;

int nVertices = 0;

int main (int argc, char *argv[]) {
    int rank, size;
    int master = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == master) {
        // Leer matriz adyacencia
        cin >> nVertices;
        vector <int> grafo(nVertices * nVertices, 0);
        for (int i = 0; i < nVertices; i++) {
            for (int j = 0; j < nVertices; j++) {
                cin >> grafo[i * nVertices + j];
            }
        }

        // Broadcast del número de vertices y la matriz
        MPI_Bcast(&nVertices, 1, MPI_INT, master, MPI_COMM_WORLD);
        MPI_Bcast(&grafo[0], nVertices * nVertices, MPI_INT, master, MPI_COMM_WORLD);

        // Setear estado de procesos (ocupado o libre)
        int procLibres = size - 1;
        vector <bool> procOcupados(size, false);
        procOcupados[master] = true;

        // Obtener el primer proceso libre
        int procDisponible = 1;
        procLibres--;
        procOcupados[procDisponible] = true;

        // Inicializar pathTSP y metada
        int costoOptimo = INT_MAX;
        vector <int> camino(nVertices, -1);

        // metadata[0] = costo optimo | metadata[1] = costo actual
        // metadata[2] = ultimo vertice del camino | metadata[3] = ha llegado a la hoja
        vector <int> metadata(4, 0);
        metadata[0] = costoOptimo;

        // Enviar camino y metadata
        MPI_Send(&camino[0], nVertices, MPI_INT, procDisponible, 1, MPI_COMM_WORLD);
        MPI_Send(&metadata[0], 4, MPI_INT, procDisponible, 1, MPI_COMM_WORLD);

        // Mientras haya algun proceso trabajando seguir computando el TSP
        // Detener procesos esclavos
        // Imprimir costo minimo
        // Imprimir ruta optima TSP
    }
    else { // Los otros procesos
        // Reciben el numero de vertices y creamos la matriz de costos
        MPI_Bcast(&nVertices, 1, MPI_INT, master, MPI_COMM_WORLD);
        vector <int> grafo(nVertices * nVertices, 0);
        MPI_Bcast(&grafo[0], n*n, MPI_INT, master, MPI_COMM_WORLD);

        // creamos la lista de posibles caminos
        list<vector<int>> caminos;

        // Mientras no recibemos un stop del maestro seguimos trabajando
        MPI_Status status;
        int costoOptimo;
        while(true) {
            MPI_Probe(master, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            // Recibimos una orden del maestro
            int orden = status.MPI_TAG;
            // Caso1: Terminar
            if (orden == 0) {
                // TODO
            }
            else if (orden == 1) { // Caso2: Descender en el arbol
                // posible camino
                vector<int> camino(nVertices);
                vector<int> metadata(4);
                MPI_Recv(&camino[0], nVertices, MPI_INT, master, orden, MPI_COMM_WORLD, &status);
                MPI_Recv(&metada[0], 4, MPI_INT, master, orden, MPI_COMM_WORLD, &status);
                costoOptimo = metadata[0];
                caminos.push_front(camino);
                // (while) Procesamos los vectores (caminos) de la lista
                while(!caminos.empty()){
                    // TODO
                    // Obtener el lower_bound (costo min hasta ese momento)
                    // Actualizamos el min costo global de ser el caso
                    // Indicar status libre del proceso y notificar al master
                }
            }
        }
    }
    return 0;
}