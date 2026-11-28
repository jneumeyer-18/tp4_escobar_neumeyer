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

        // Empieza el bfs en i
        // frente es el que toca
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

        componentes.push_back(cola);
    }

    return componentes;
}

// ================================================================
//        EJERCICIO 2: CAMINO MÍNIMO CON PESOS DE TODOS CON TODOS
// ================================================================

const int INFINITO = 1000000000;

// dist[i][j] es la distancia mínima en km entre la ciudad i y la ciudad j, con
// las ciudades numeradas en el orden de g.get_vertices()
vector<vector<int>> distanciasTodosConTodos(const GrafoCiudades& g) {
    vector<string> ciudades = g.get_vertices();
    int n = (int)ciudades.size();
    vector<vector<int>> vecino(n);
    vector<vector<int>> peso(n);
    for (int i = 0; i < n; i++) {
        vector<string> vecinos = g.get_neighbors(ciudades[i]);
        for (int j = 0; j < (int)vecinos.size(); j++) {
            vecino[i].push_back(indiceDe(g, vecinos[j]));
            peso[i].push_back(*g.get_edge_data(ciudades[i], vecinos[j]));
        }
    }

 
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

    // ahora hacemos el dijkstra
    vector<vector<int>> dist(n, vector<int>(n, INFINITO));

    for (int s = 0; s < n; s++) {
        vector<int> listo(n, 0); 
        dist[s][s] = 0;

        while (true) {

            int u = -1;
            for (int v = 0; v < n; v++) {
                if (listo[v] == 1) continue;
                if (comp[v] != comp[s]) continue;
                if (u == -1 || dist[s][v] < dist[s][u]) u = v;
            }
            if (u == -1) break;  
            listo[u] = 1;

            for (int j = 0; j < (int)vecino[u].size(); j++) {
                int v = vecino[u][j];
                if (dist[s][u] + peso[u][j] < dist[s][v]) {
                    dist[s][v] = dist[s][u] + peso[u][j];
                }
            }
        }
    }

    
   
    return dist;
}

// ================================================================
//                     EJERCICIO 3: TRIÁNGULOS
// ================================================================

int contarTriangulos(const GrafoCiudades& g) {
    vector<string> ciudades = g.get_vertices();
    int n = (int)ciudades.size();

    vector<vector<int>> vecino(n);
    for (int i = 0; i < n; i++) {
        vector<string> vecinos = g.get_neighbors(ciudades[i]);
        for (int j = 0; j < (int)vecinos.size(); j++) {
            vecino[i].push_back(indiceDe(g, vecinos[j]));
        }
    }

    // la idea es pararnos en a y contar todos los triángulos que la contienen
    // después se borra a, para que ningún triángulo se vuelva a contar
    vector<int> borrado(n, 0);  
    int triangulos = 0;

    for (int a = 0; a < n; a++) {
        // Recorro los pares de vecinos (b, c) de a
        for (int j = 0; j < (int)vecino[a].size(); j++) {
            int b = vecino[a][j];
            if (borrado[b] == 1) continue; 

            for (int k = j + 1; k < (int)vecino[a].size(); k++) {
                int c = vecino[a][k];
                if (borrado[c] == 1) continue;

               
                if (g.edge_exists(ciudades[b], ciudades[c])) {
                    triangulos++;
                }
            }
        }
        borrado[a] = 1;  
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


    int mayor = 0;
    int c1 = 0;
    int c2 = 0;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {  
            if (dist[i][j] == INFINITO) continue;  // están en grafos distintos
            if (dist[i][j] > mayor) {
                mayor = dist[i][j];
                c1 = i;
                c2 = j;
            }
        }
    }

   
    Diametro resultado;
    resultado.distancia = mayor;
    resultado.ciudad1 = ciudades[c1];
    resultado.ciudad2 = ciudades[c2];
    return resultado;
}

// ================================================================
//          EJERCICIO 5: RANDOM WALKS Y LA CIUDAD MÁS CENTRAL
// ================================================================

// visitas[c] es la cantidad de veces que los random walks pasaron por c.
using Visitas = unordered_map<string, int>;

Visitas randomWalks(const GrafoCiudades& g, int cantidad, int largo) {
    vector<string> ciudades = g.get_vertices();
    int n = (int)ciudades.size();

    
    vector<vector<int>> vecino(n);
    for (int i = 0; i < n; i++) {
        vector<string> vecinos = g.get_neighbors(ciudades[i]);
        for (int j = 0; j < (int)vecinos.size(); j++) {
            vecino[i].push_back(indiceDe(g, vecinos[j]));
        }
    }

    // generador de números al azar, con un punto fija para que el resultado siempre.
    mt19937 generador(67);

    
    vector<int> cuenta(n, 0);

    for (int w = 0; w < cantidad; w++) {
        // Ciudad de partida al azar: como random.randint(0, n - 1)
        uniform_int_distribution<int> ciudadAlAzar(0, n - 1);
        int actual = ciudadAlAzar(generador);
        cuenta[actual]++;

        for (int paso = 0; paso < largo; paso++) {
            int grado = (int)vecino[actual].size();
           
            if (grado == 0) break;

            // elegir un vecino al azar, todos con la misma probabilidad
            uniform_int_distribution<int> elegir(0, grado - 1);
            actual = vecino[actual][elegir(generador)];
            cuenta[actual]++;
        }
    }

    
    Visitas visitas;
    for (int i = 0; i < n; i++) {
        visitas[ciudades[i]] = cuenta[i];
    }
    return visitas;
}

Visitas randomWalksPorDistancia(const GrafoCiudades& g, int cantidad, int largo) {
    vector<string> ciudades = g.get_vertices();
    int n = (int)ciudades.size();

    vector<vector<int>> vecino(n);
    vector<vector<int>> peso(n);
    for (int i = 0; i < n; i++) {
        vector<string> vecinos = g.get_neighbors(ciudades[i]);
        for (int j = 0; j < (int)vecinos.size(); j++) {
            vecino[i].push_back(indiceDe(g, vecinos[j]));
            peso[i].push_back(*g.get_edge_data(ciudades[i], vecinos[j]));
        }
    }

    mt19937 generador(67);

    vector<int> cuenta(n, 0);

    for (int w = 0; w < cantidad; w++) {
        uniform_int_distribution<int> ciudadAlAzar(0, n - 1);
        int actual = ciudadAlAzar(generador);
        cuenta[actual]++;

        for (int paso = 0; paso < largo; paso++) {
            int grado = (int)vecino[actual].size();
            if (grado == 0) break;

            double total = 0;
            for (int j = 0; j < grado; j++) {
                total += 1.0 / peso[actual][j];
            }

            uniform_real_distribution<double> tiro(0.0, total);
            double r = tiro(generador);

            int elegido = grado - 1; 
            double acumulado = 0;
            for (int j = 0; j < grado; j++) {
                acumulado += 1.0 / peso[actual][j];
                if (r < acumulado) {
                    elegido = j;
                    break;
                }
            }

            actual = vecino[actual][elegido];
            cuenta[actual]++;
        }
    }

    Visitas visitas;
    for (int i = 0; i < n; i++) {
        visitas[ciudades[i]] = cuenta[i];
    }
    return visitas;
}

// ================================================================
//                   EJERCICIO 6: ANÁLISIS LIBRE
// ================================================================

void analisisLibre(const GrafoCiudades& g) {
    vector<string> ciudades = g.get_vertices();
    int n = (int)ciudades.size();

    vector<vector<int>> vecino(n);
    for (int i = 0; i < n; i++) {
        vector<string> vecinos = g.get_neighbors(ciudades[i]);
        for (int j = 0; j < (int)vecinos.size(); j++) {
            vecino[i].push_back(indiceDe(g, vecinos[j]));
        }
    }

    vector<int> orden;
    for (int d = n; d >= 0; d--) {
        for (int i = 0; i < n; i++) {
            if ((int)vecino[i].size() == d) orden.push_back(i);
        }
    }

    cout << "Coloreo: ciudades unidas por una ruta no pueden tener el mismo color\n";

    // Greedy: a cada ciudad, el color más chico que no use ningún vecino
    vector<int> color(n, -1);  // -1: todavía sin color
    int coloresUsados = 0;
    for (int k = 0; k < n; k++) {
        int v = orden[k];
        int c = 0;
        bool libre = false;
        while (!libre) {
            libre = true;
            for (int j = 0; j < (int)vecino[v].size(); j++) {
                if (color[vecino[v][j]] == c) libre = false;
            }
            if (!libre) c++;
        }
        color[v] = c;
        if (c + 1 > coloresUsados) coloresUsados = c + 1;
    }
    cout << "Greedy (de mayor a menor grado): " << coloresUsados << " colores\n";

    // si las 9 de Misiones están todas conectadas necesitan 9 colores
    vector<string> misiones = {"Posadas", "Oberá", "Eldorado", "Puerto Iguazú",
                               "Apóstoles", "Leandro N. Alem", "San Vicente",
                               "Puerto Rico", "Bernardo de Irigoyen"};
    int m = (int)misiones.size();
    bool todasConectadas = true;
    for (int i = 0; i < m; i++) {
        for (int j = i + 1; j < m; j++) {
            if (!g.edge_exists(misiones[i], misiones[j])) todasConectadas = false;
        }
    }

    if (todasConectadas && coloresUsados == m) {
        cout << "Las " << m << " ciudades de Misiones están todas conectadas entre sí, "
             << "así que con menos de " << m << " colores es imposible.\n";
        cout << "Conclusión: el mínimo es exactamente " << m << " colores.\n";
    }
}




void imprimir10MasVisitadas(const GrafoCiudades& g, Visitas& visitas) {
    vector<string> ciudades = g.get_vertices();
    int n = (int)ciudades.size();
    vector<int> elegida(n, 0);
    for (int puesto = 1; puesto <= 10; puesto++) {
        int mejor = -1;
        for (int i = 0; i < n; i++) {
            if (elegida[i] == 1) continue;
            if (mejor == -1 || visitas[ciudades[i]] > visitas[ciudades[mejor]]) mejor = i;
        }
        elegida[mejor] = 1;
        cout << "  " << puesto << ". " << ciudades[mejor] << ": "
             << visitas[ciudades[mejor]] << " visitas (grado "
             << g.get_neighbors(ciudades[mejor]).size() << ")\n";
    }
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
    int mayor = 0;
    for (int i = 1; i < (int)componentes.size(); i++) {
        if (componentes[i].size() > componentes[mayor].size()) mayor = i;
    }
    cout << "Tamaño de la más grande: " << componentes[mayor].size() << "\n";

    cout << "Ciudades fuera de la más grande:\n";
    for (int i = 0; i < (int)componentes.size(); i++) {
        if (i == mayor) continue;
        cout << "  Componente " << i << ":";
        for (int j = 0; j < (int)componentes[i].size(); j++) {
            cout << " " << componentes[i][j] << ";";
        }
        cout << "\n";
    }

    cout << "\n=== Ejercicio 2 ===\n";
    vector<vector<int>> dist = distanciasTodosConTodos(g);

    int ba = indiceDe(g, "Buenos Aires");
    int lq = indiceDe(g, "La Quiaca");
    int us = indiceDe(g, "Ushuaia");

    if (dist[ba][lq] == INFINITO) {
        cout << "Buenos Aires - La Quiaca: no hay camino (están en componentes distintas)\n";
    } else {
        cout << "Buenos Aires - La Quiaca: " << dist[ba][lq] << " km\n";
    }

    if (dist[ba][us] == INFINITO) {
        cout << "Buenos Aires - Ushuaia: no hay camino (están en componentes distintas)\n";
    } else {
        cout << "Buenos Aires - Ushuaia: " << dist[ba][us] << " km\n";
    }

    cout << "\n=== Ejercicio 3 ===\n";
    cout << "Triángulos: " << contarTriangulos(g) << "\n";

    cout << "\n=== Ejercicio 4 ===\n";
    Diametro d = diametro(g, dist);
    cout << "Diámetro: " << d.distancia << " km, entre " << d.ciudad1 << " y "
              << d.ciudad2 << "\n";

     cout << "\n=== Ejercicio 5 ===\n";
    Visitas visitas = randomWalks(g, 10000, 100);
    Visitas visitasPorDistancia = randomWalksPorDistancia(g, 10000, 100);

    cout << "Las 10 más visitadas (random walk uniforme):\n";
    imprimir10MasVisitadas(g, visitas);

    cout << "Las 10 más visitadas (random walk por distancia):\n";
    imprimir10MasVisitadas(g, visitasPorDistancia);

    vector<string> ciudades = g.get_vertices();
    int n = (int)ciudades.size();
    vector<int> elegidaGrado(n, 0);
    cout << "Las 10 de mayor grado:\n";
    for (int puesto = 1; puesto <= 10; puesto++) {
        int mejor = -1;
        for (int i = 0; i < n; i++) {
            if (elegidaGrado[i] == 1) continue;
            if (mejor == -1 || g.get_neighbors(ciudades[i]).size() >
                                   g.get_neighbors(ciudades[mejor]).size()) mejor = i;
        }
        elegidaGrado[mejor] = 1;
        cout << "  " << puesto << ". " << ciudades[mejor] << ": grado "
             << g.get_neighbors(ciudades[mejor]).size() << "\n";
    }

    cout << "\n=== Ejercicio 6 ===\n";
    analisisLibre(g);

    return 0;
}