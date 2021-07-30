#include<iostream>
#include"TSP.h"

using namespace std;

void firstRead() {
    // Leer matriz adyacencia
    cout << "Numero de ciudades: ";
    cin >> TSP::nVertices;
    TSP::cityNames.resize(TSP::nVertices);
    cout << "Nombre de las ciudades: \n";
    for (int i = 0; i < TSP::nVertices; i++) {
        cout << " Ciudad " << i << ": ";
        cin >> TSP::cityNames[i];
    }
    TSP::grafo.resize(TSP::nVertices * TSP::nVertices);
    cout << "\nMatriz de adyacencia:\n";
    for (int i = 0; i < TSP::nVertices; i++) {
        for (int j = 0; j < TSP::nVertices; j++) {
            cin >> TSP::grafo[i * TSP::nVertices + j];
        }
    }
}

void printGrafo(int n) {
    cout << endl;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            cout << TSP::grafo[i * n + j] << " ";
        } cout << endl;
    } cout << endl;
}

void updateGrafo(vector<int> &dist) {
    vector<int> newGrafo(TSP::nVertices * TSP::nVertices);
    for (int i = 0; i < TSP::nVertices-1; i++) {
        for (int j = 0; j < TSP::nVertices; j++) {
            if (j < TSP::nVertices -1) {
                newGrafo[i * TSP::nVertices + j] = TSP::grafo[i * (TSP::nVertices-1) + j];
            } else {
                newGrafo[i * TSP::nVertices + j] = dist[i];
                newGrafo[j * TSP::nVertices + i] = dist[i];
            }
        }
    }
    TSP::grafo = newGrafo;
}

void menu(int in) {
    cout << "\nElija su opcion:\n";
    cout << "1: Ingresar matriz de distancias\n";
    cout << "2: Agregar una nueva ciudad\n";
    if (in != 0) cout << "3: Calcular TSP\n";
    cout << "0: Salir\n";
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &TSP::size);
    MPI_Comm_rank(MPI_COMM_WORLD, &TSP::rank);
    int orden, i = 0;

    while (true) {
        if (TSP::rank == TSP::master) {
            int opcion;
            if (i == 0) {
                cout << "\n\t***BIENVENIDO***\n";
            }
            menu(i);
            cin >> opcion;
            if (opcion == 1) {
                firstRead();
            } 
            else if (opcion == 2) {
                string nCity;
                cout << "Nombre de la ciudad: ";
                cin >> nCity;
                TSP::cityNames.push_back(nCity);
                vector<int> dist(TSP::nVertices);
                cout << "Agregar la distancia desde la nueva ciudad \n";
                for (int j = 0; j < TSP::nVertices; j++) {
                     cout << "Hacia " << TSP::cityNames[j] << ": ";
                     cin >> dist[j];
                }
                TSP::nVertices++;
                updateGrafo(dist);
            } 
            else if (opcion == 3) {
                orden = 10;
                MPI_Bcast(&orden, 1, MPI_INT, TSP::master, MPI_COMM_WORLD);
                TSP::mainTSP(argc, argv);
                cout << "\n";
            }
            else {
                orden = -1;
                MPI_Bcast(&orden, 1, MPI_INT, TSP::master, MPI_COMM_WORLD);
                cout << "\nSaliendo...\n";
                break;
            }
        } 
        else {
            MPI_Bcast(&orden, 1, MPI_INT, TSP::master, MPI_COMM_WORLD);
            if (orden == -1) break;
            TSP::slaveTSP();
        }
        i++;
    }
    
    MPI_Finalize();
    
    return 0;
}
