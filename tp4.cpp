#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "graphClass.h"

using namespace std;

// Vértices: nombre de la ciudad, con su provincia como dato.
// Aristas: distancia aproximada en km. Cada ruta está en los dos sentidos.
using GrafoCiudades = Graph<string, int>;

// ================================================================
//                     CARGA DEL GRAFO (ya dada)
// ================================================================

static string strip(const string& s) {
    const char* ws = " \t\r\n";
    size_t b = s.find_first_not_of(ws);
    if (b == string::npos) return "";
    size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

static vector<string> split(const string& l) {
    vector<string> campos;
    stringstream ss(l);
    string campo;
    while (getline(ss, campo, ',')) campos.push_back(strip(campo));
    return campos;
}

static bool cargarCiudades(GrafoCiudades& g, const string& path) {
    ifstream file(path);
    if (!file) {
        cerr << "No se pudo abrir el archivo: " << path << "\n";
        return false;
    }
    string l;
    getline(file, l);  // encabezado: ciudad,provincia,lat,lon
    while (getline(file, l)) {
        l = strip(l);
        if (l.empty()) continue;
        vector<string> c = split(l);
        g.add_vertex(c[0], c[1]);
    }
    return true;
}

static bool cargarRutas(GrafoCiudades& g, const string& path) {
    ifstream file(path);
    if (!file) {
        cerr << "No se pudo abrir el archivo: " << path << "\n";
        return false;
    }
    string l;
    getline(file, l);  // encabezado: ciudad1,ciudad2,distancia_km
    while (getline(file, l)) {
        l = strip(l);
        if (l.empty()) continue;
        vector<string> c = split(l);
        int distancia = stoi(c[2]);
        g.add_edge(c[0], c[1], distancia);
        g.add_edge(c[1], c[0], distancia);
    }
    return true;
}

// Posición de la ciudad en g.get_vertices() (-1 si no está).
int indiceDe(const GrafoCiudades& g, const string& ciudad) {
    vector<string> ciudades = g.get_vertices();
    for (int i = 0; i < (int)ciudades.size(); i++) {
        if (ciudades[i] == ciudad) return i;
    }
    return -1;
}

// ================================================================
//                  EJERCICIO 1: COMPONENTES CONEXAS
// ================================================================

vector<vector<string>> componentesConexas(const GrafoCiudades& g) {
    vector<string> ciudades = g.get_vertices();
    int n = (int)ciudades.size();

    // visitado[i] dice si la ciudad i ya está en alguna componente
    vector<bool> visitado(n, false);
    vector<vector<string>> componentes;

    for (int i = 0; i < n; i++) {
        if (visitado[i]) continue;

        // Empieza una componente nueva: BFS desde la ciudad i.
        // La cola es un vector común; 'frente' marca cuál toca procesar.
        vector<string> cola;
        cola.push_back(ciudades[i]);
        visitado[i] = true;
        int frente = 0;

        while (frente < (int)cola.size()) {
            string actual = cola[frente];
            frente++;

            vector<string> vecinos = g.get_neighbors(actual);
            for (int j = 0; j < (int)vecinos.size(); j++) {
                int k = indiceDe(g, vecinos[j]);
                if (!visitado[k]) {
                    visitado[k] = true;
                    cola.push_back(vecinos[j]);
                }
            }
        }

        // Todo lo que pasó por la cola es la componente
        componentes.push_back(cola);
    }

    // Buscar la componente más grande
    int mayor = 0;
    for (int i = 1; i < (int)componentes.size(); i++) {
        if (componentes[i].size() > componentes[mayor].size()) mayor = i;
    }
    cout << "Tamaño de la más grande: " << componentes[mayor].size() << "\n";

    // Imprimir las ciudades de las demás componentes
    cout << "Ciudades fuera de la más grande:\n";
    for (int i = 0; i < (int)componentes.size(); i++) {
        if (i == mayor) continue;
        cout << "  Componente " << i << ":";
        for (int j = 0; j < (int)componentes[i].size(); j++) {
            cout << " " << componentes[i][j] << ";";
        }
        cout << "\n";
    }

    return componentes;
}

// ================================================================
//        EJERCICIO 2: CAMINO MÍNIMO CON PESOS DE TODOS CON TODOS
// ================================================================

const int INFINITO = 1000000000;

// dist[i][j]: distancia mínima en km entre la ciudad i y la ciudad j, con
// las ciudades numeradas en el orden de g.get_vertices() (INFINITO si no hay
// camino).
vector<vector<int>> distanciasTodosConTodos(const GrafoCiudades& g) {
    vector<string> ciudades = g.get_vertices();
    int n = (int)ciudades.size();

    // ---------- 1. Vecinos y distancias de cada ciudad, con índices ----------
    // vecino[i][j] = índice del j-ésimo vecino de la ciudad i
    // peso[i][j]   = km de la ruta entre i y ese vecino
    vector<vector<int>> vecino(n);
    vector<vector<int>> peso(n);
    for (int i = 0; i < n; i++) {
        vector<string> vecinos = g.get_neighbors(ciudades[i]);
        for (int j = 0; j < (int)vecinos.size(); j++) {
            vecino[i].push_back(indiceDe(g, vecinos[j]));
            peso[i].push_back(*g.get_edge_data(ciudades[i], vecinos[j]));
        }
    }

    // ---------- 2. Componente conexa de cada ciudad (BFS como en el ej. 1) ----------
    // comp[i] = número de componente de la ciudad i
    vector<int> comp(n, -1);
    int c = 0;
    for (int i = 0; i < n; i++) {
        if (comp[i] != -1) continue;
        vector<int> cola;
        cola.push_back(i);
        comp[i] = c;
        int frente = 0;
        while (frente < (int)cola.size()) {
            int actual = cola[frente];
            frente++;
            for (int j = 0; j < (int)vecino[actual].size(); j++) {
                int v = vecino[actual][j];
                if (comp[v] == -1) {
                    comp[v] = c;
                    cola.push_back(v);
                }
            }
        }
        c++;
    }

    // ---------- 3. Dijkstra desde cada ciudad ----------
    vector<vector<int>> dist(n, vector<int>(n, INFINITO));

    for (int s = 0; s < n; s++) {
        vector<int> listo(n, 0);  // listo[v] == 1: ya sé la distancia mínima a v
        dist[s][s] = 0;

        while (true) {
            // Elegir, entre las ciudades de la MISMA componente que s que no
            // están listas, la de menor distancia. Las de otra componente no
            // se miran: no hay camino, quedan en INFINITO.
            int u = -1;
            for (int v = 0; v < n; v++) {
                if (listo[v] == 1) continue;
                if (comp[v] != comp[s]) continue;
                if (u == -1 || dist[s][v] < dist[s][u]) u = v;
            }
            if (u == -1) break;  // ya procesé toda la componente
            listo[u] = 1;

            // Relajar: ¿llego más corto a algún vecino pasando por u?
            for (int j = 0; j < (int)vecino[u].size(); j++) {
                int v = vecino[u][j];
                if (dist[s][u] + peso[u][j] < dist[s][v]) {
                    dist[s][v] = dist[s][u] + peso[u][j];
                }
            }
        }
    }

    // ---------- 4. Distancias pedidas ----------
    int ba = indiceDe(g, "Buenos Aires");
    int lq = indiceDe(g, "La Quiaca");
    int us = indiceDe(g, "Ushuaia");

    if (comp[ba] != comp[lq]) {
        cout << "Buenos Aires - La Quiaca: no hay camino (están en componentes distintas)\n";
    } else {
        cout << "Buenos Aires - La Quiaca: " << dist[ba][lq] << " km\n";
    }

    if (comp[ba] != comp[us]) {
        cout << "Buenos Aires - Ushuaia: no hay camino (están en componentes distintas)\n";
    } else {
        cout << "Buenos Aires - Ushuaia: " << dist[ba][us] << " km\n";
    }

    return dist;
}

// ================================================================
//                     EJERCICIO 3: TRIÁNGULOS
// ================================================================

int contarTriangulos(const GrafoCiudades& g) {
    vector<string> ciudades = g.get_vertices();
    int n = (int)ciudades.size();

    // Listas de adyacencia con índices: vecino[i] = índices de los vecinos de i
    vector<vector<int>> vecino(n);
    for (int i = 0; i < n; i++) {
        vector<string> vecinos = g.get_neighbors(ciudades[i]);
        for (int j = 0; j < (int)vecinos.size(); j++) {
            vecino[i].push_back(indiceDe(g, vecinos[j]));
        }
    }

    // Idea: me paro en una ciudad a, cuento todos los triángulos que la
    // contienen (pares de vecinos b, c que además son vecinos entre sí) y
    // después "borro" a, para que ningún triángulo se vuelva a contar.
    vector<int> borrado(n, 0);  // borrado[i] == 1: la ciudad i ya fue procesada
    int triangulos = 0;

    for (int a = 0; a < n; a++) {
        // Recorro los pares de vecinos (b, c) de a, cada par una sola vez (j < k)
        for (int j = 0; j < (int)vecino[a].size(); j++) {
            int b = vecino[a][j];
            if (borrado[b] == 1) continue;  // b ya se borró: sus triángulos ya se contaron

            for (int k = j + 1; k < (int)vecino[a].size(); k++) {
                int c = vecino[a][k];
                if (borrado[c] == 1) continue;

                // a-b y a-c son aristas; si b-c también, es un triángulo
                if (g.edge_exists(ciudades[b], ciudades[c])) {
                    triangulos++;
                }
            }
        }
        borrado[a] = 1;  // "borro" a del grafo
    }
    return triangulos;
}

// ================================================================
//                EJERCICIO 4: DIÁMETRO CON PESOS
// ================================================================

class Diametro {
public:
    int distancia;
    string ciudad1;
    string ciudad2;
};

Diametro diametro(const GrafoCiudades& g, const vector<vector<int>>& dist) {
    vector<string> ciudades = g.get_vertices();
    int n = (int)ciudades.size();

    // Busco el máximo de dist[i][j] entre todos los pares de ciudades.
    // Los pares sin camino (INFINITO) no cuentan: no tienen distancia mínima.
    int mayor = 0;
    int c1 = 0;
    int c2 = 0;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {  // j > i: cada par una sola vez
            if (dist[i][j] == INFINITO) continue;  // están en componentes distintas
            if (dist[i][j] > mayor) {
                mayor = dist[i][j];
                c1 = i;
                c2 = j;
            }
        }
    }

    // Armo el resultado usando la clase Diametro
    Diametro resultado;
    resultado.distancia = mayor;
    resultado.ciudad1 = ciudades[c1];
    resultado.ciudad2 = ciudades[c2];
    return resultado;
}

// ================================================================
//          EJERCICIO 5: RANDOM WALKS Y LA CIUDAD MÁS CENTRAL
// ================================================================

// visitas[c]: cantidad de veces que los random walks pasaron por c.
using Visitas = unordered_map<string, int>;

Visitas randomWalks(const GrafoCiudades& g, int cantidad, int largo) {
    // TODO: implementar
    return {};
}

Visitas randomWalksPorDistancia(const GrafoCiudades& g, int cantidad, int largo) {
    // TODO: implementar
    return {};
}

// ================================================================
//                   EJERCICIO 6: ANÁLISIS LIBRE
// ================================================================

void analisisLibre(const GrafoCiudades& g) {
    // TODO: implementar
    return;
}

// ================================================================
//                              MAIN
// ================================================================

int main(int argc, char* argv[]) {
    string pathCiudades = argc > 1 ? argv[1] : "ciudades_argentina.csv";
    string pathRutas = argc > 2 ? argv[2] : "grafo_ciudades.csv";

    GrafoCiudades g;
    if (!cargarCiudades(g, pathCiudades)) return 1;
    if (!cargarRutas(g, pathRutas)) return 1;
    cout << "Ciudades cargadas: " << g.get_vertices().size() << "\n\n";

    cout << "=== Ejercicio 1 ===\n";
    vector<vector<string>> componentes = componentesConexas(g);
    cout << "Componentes conexas: " << componentes.size() << "\n";
    // TODO: tamaño de la más grande y ciudades que quedan fuera de ella

    cout << "\n=== Ejercicio 2 ===\n";
    vector<vector<int>> dist = distanciasTodosConTodos(g);
    // TODO: distancias Buenos Aires - La Quiaca y Buenos Aires - Ushuaia

    cout << "\n=== Ejercicio 3 ===\n";
    cout << "Triángulos: " << contarTriangulos(g) << "\n";

    cout << "\n=== Ejercicio 4 ===\n";
    Diametro d = diametro(g, dist);
    cout << "Diámetro: " << d.distancia << " km, entre " << d.ciudad1 << " y "
              << d.ciudad2 << "\n";

    cout << "\n=== Ejercicio 5 ===\n";
    Visitas visitas = randomWalks(g, 10000, 100);
    Visitas visitasPorDistancia = randomWalksPorDistancia(g, 10000, 100);
    // TODO: las 10 ciudades más visitadas en cada caso, y las 10 de mayor grado

    cout << "\n=== Ejercicio 6 ===\n";
    analisisLibre(g);

    return 0;
}
