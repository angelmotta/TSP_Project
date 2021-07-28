#include <mpi.h>
#include <iostream>
#include <vector>
#include <list>
#include <climits>

using namespace std;

int nVertices = 0;
vector <int> grafo;

int getProcLibre(vector<bool> &procOcupados, int totalProc, int &libres){
    for (int i = 0; i < totalProc; i++){
        if (procOcupados[i] == false) {
            procOcupados[i] = true;
            libres--;
            return i;
        }
    }
    return -1;
}

int minCost( int start, int idx){
    int minimo = INT_MAX;
    for(int i=0; i<nVertices; i++){
        if(i != idx && grafo[start + i] < minimo)
            minimo = grafo[start + i];
    }
    return minimo;
}

int getLowerBound(vector<int>& camino, int currCost) {
    for(int i=0; i<nVertices; i++){
        if(camino[i] == -1){
            currCost += minCost(i*nVertices, i);
        }
    }
    return currCost;
}

int main (int argc, char *argv[]) {
    int rank, size;
    int master = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == master) {
        // Leer matriz adyacencia
        cin >> nVertices;
        grafo.resize(nVertices * nVertices);
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
        while (procLibres != size-1) {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int orden = status.MPI_TAG;
            int src = status.MPI_SOURCE;

            if (orden == 2) { // se halló un mejor camino con menor costo 
               MPI_Recv(&camino[0], nVertices, MPI_INT, src, orden, MPI_COMM_WORLD, &status);
               MPI_Recv(&metadata[0], 4, MPI_INT, src, orden, MPI_COMM_WORLD, &status);
               costoOptimo = metadata[1];
            }

            if (orden == 4) {   // recibe mensaje para seguir descendiendo
                vector<int> msg(2);
                MPI_Recv(&msg[0], 2, MPI_INT, src, orden, MPI_COMM_WORLD, &status);
                int currMinCost = msg[0];
                int caminosPorProcesar = msg[1];
                if (currMinCost < costoOptimo) {
                    int procAsignados = (caminosPorProcesar <= procLibres) ? caminosPorProcesar : procLibres;
                    vector<int> slavesAsignados(procAsignados);
                    for (int i=0; i<procAsignados; i++) {
                        slavesAsignados[i] = getProcLibre(procOcupados, size, procLibres);
                    }
                    MPI_Send(&costoOptimo, 1, MPI_INT, src, 4, MPI_COMM_WORLD);
                    MPI_Send(&slavesAsignados[0], procAsignados, MPI_INT, src, 4, MPI_COMM_WORLD);
                }
                else {
                    orden = 5;      // detener descenso
                    MPI_Send(&costoOptimo, 1, MPI_INT, src, orden, MPI_COMM_WORLD);
                }
            }
            
        }
        // Detener procesos esclavos
        // Imprimir costo minimo
        // Imprimir ruta optima TSP
    }
    else { // Los otros procesos
        // Reciben el numero de vertices y creamos la matriz de costos
        MPI_Bcast(&nVertices, 1, MPI_INT, master, MPI_COMM_WORLD);
        grafo.resize(nVertices * nVertices);
        MPI_Bcast(&grafo[0], nVertices*nVertices, MPI_INT, master, MPI_COMM_WORLD);

        // creamos la lista de posibles caminos con su metadata
        list< pair< vector<int>, vector<int> > > caminos;

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
                MPI_Recv(&metadata[0], 4, MPI_INT, master, orden, MPI_COMM_WORLD, &status);
                costoOptimo = metadata[0];
                caminos.push_front({camino, metadata});
                // (while) Procesamos los vectores (caminos) de la lista
                while(!caminos.empty()){
                    vector<int> posibleCamino = caminos.front().first;
                    vector<int> posMetadata = caminos.front().second;
                    caminos.pop_front();
                    // se llega al nivel hoja
                    if (posMetadata[3] == 1) {
                        if (posMetadata[1] < costoOptimo) {    // si el costo actual es menor al optimo
                            costoOptimo = posMetadata[1];
                            MPI_Send(&posibleCamino[0], nVertices, MPI_INT, master, 2, MPI_COMM_WORLD);
                            MPI_Send(&posMetadata[0], 4, MPI_INT, master, 2, MPI_COMM_WORLD);
                        }
                        continue;
                    }

                    // Obtener el lower_bound (costo min hasta ese momento)
                    int currMinCost = getLowerBound(posibleCamino, posMetadata[1]);

                    // Actualizamos el min costo global de ser el caso
                    if (currMinCost < costoOptimo) {
                        vector<int> msg(2);
                        msg[0] = currMinCost;
                        msg[1] = caminos.size();
                        MPI_Send(&msg[0], 2, MPI_INT, master, 4, MPI_COMM_WORLD); // orden 4: seguir descendiendo

                        // Recibir el costo optimo global desde el master
                        MPI_Recv(&costoOptimo, 1, MPI_INT, master, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                        // Verificar la orden del master
                        if (status.MPI_TAG == 4) {
                            // Branch: ramificar el problema
                        }
                        else {
                            // orden 5: No seguir
                            continue;
                        }
                    }

                    // Indicar status libre del proceso y notificar al master
                }
            }
        }
    }
    return 0;
}