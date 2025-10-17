#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>

// Macro multiplataforma para exportar funções
#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __attribute__((visibility("default")))
#endif

// Definições de estruturas
struct Point {
    double x, y;
    Point(double x_ = 0, double y_ = 0) : x(x_), y(y_) {}
};

struct Path {
    std::vector<Point> points;
};

// Função auxiliar: teste CCW
inline bool ccw(const Point& A, const Point& B, const Point& C) {
    return (C.y - A.y) * (B.x - A.x) > (B.y - A.y) * (C.x - A.x);
}

// Função auxiliar: verifica se dois segmentos se cruzam
inline bool segments_intersect(const Point& p1, const Point& p2, 
                               const Point& p3, const Point& p4) {
    const double eps = 1e-9;
    
    // Verificar se compartilham endpoints
    if ((std::abs(p1.x - p3.x) < eps && std::abs(p1.y - p3.y) < eps) ||
        (std::abs(p1.x - p4.x) < eps && std::abs(p1.y - p4.y) < eps) ||
        (std::abs(p2.x - p3.x) < eps && std::abs(p2.y - p3.y) < eps) ||
        (std::abs(p2.x - p4.x) < eps && std::abs(p2.y - p4.y) < eps)) {
        return false;
    }
    
    return (ccw(p1, p3, p4) != ccw(p2, p3, p4)) && 
           (ccw(p1, p2, p3) != ccw(p1, p2, p4));
}

// Verifica se um novo caminho cruza com caminhos existentes
inline bool path_crosses_existing(const Path& new_path, 
                                  const std::vector<Path>& existing_paths) {
    for (const auto& existing : existing_paths) {
        for (size_t i = 0; i < new_path.points.size() - 1; ++i) {
            for (size_t j = 0; j < existing.points.size() - 1; ++j) {
                if (segments_intersect(new_path.points[i], new_path.points[i+1],
                                     existing.points[j], existing.points[j+1])) {
                    return true;
                }
            }
        }
    }
    return false;
}

// Estrutura de resultado
struct SolutionResult {
    bool found;
    int* b_permutation;  // Array de tamanho n_acima
    long long attempts;
};

extern "C" {
    
    EXPORT SolutionResult* find_solution_single_target(
        double* points_A_x, double* points_A_y, int n,
        int* idx_acima, int n_acima,
        int* idx_abaixo, int n_abaixo,
        double* points_B_x, double* points_B_y,
        double target_x, double target_y,  // Único ponto destino
        long long max_iterations) {
        
        // Criar vetores de pontos
        std::vector<Point> pointsA(n), pointsB(n_acima);
        Point target(target_x, target_y);
        
        for (int i = 0; i < n; ++i) {
            pointsA[i] = Point(points_A_x[i], points_A_y[i]);
        }
        
        for (int i = 0; i < n_acima; ++i) {
            pointsB[i] = Point(points_B_x[i], points_B_y[i]);
        }
        
        // Criar permutação inicial de B
        std::vector<int> b_perm(n_acima);
        for (int i = 0; i < n_acima; ++i) b_perm[i] = i;
        
        long long attempts = 0;
        
        // Iterar sobre todas as permutações de B apenas
        do {
            ++attempts;
            
            if (max_iterations > 0 && attempts > max_iterations) {
                goto end_search;
            }
            
            // Construir solução incrementalmente
            std::vector<Path> paths;
            bool has_crossing = false;
            
            // Adicionar caminhos de pontos acima (via B)
            for (int i = 0; i < n_acima; ++i) {
                int global_a_idx = idx_acima[i];
                int b_idx = b_perm[i];
                
                Path new_path;
                new_path.points.push_back(pointsA[global_a_idx]);
                new_path.points.push_back(pointsB[b_idx]);
                new_path.points.push_back(target);
                
                if (path_crosses_existing(new_path, paths)) {
                    has_crossing = true;
                    break;
                }
                
                paths.push_back(new_path);
            }
            
            if (has_crossing) continue;
            
            // Adicionar caminhos de pontos abaixo (direto ao target)
            for (int i = 0; i < n_abaixo; ++i) {
                int global_a_idx = idx_abaixo[i];
                
                Path new_path;
                new_path.points.push_back(pointsA[global_a_idx]);
                new_path.points.push_back(target);
                
                if (path_crosses_existing(new_path, paths)) {
                    has_crossing = true;
                    break;
                }
                
                paths.push_back(new_path);
            }
            
            if (!has_crossing) {
                // SOLUÇÃO ENCONTRADA!
                SolutionResult* result = new SolutionResult;
                result->found = true;
                result->attempts = attempts;
                
                result->b_permutation = new int[n_acima];
                for (int i = 0; i < n_acima; ++i) {
                    result->b_permutation[i] = b_perm[i];
                }
                
                return result;
            }
            
        } while (std::next_permutation(b_perm.begin(), b_perm.end()));
        
    end_search:
        // Nenhuma solução encontrada
        SolutionResult* result = new SolutionResult;
        result->found = false;
        result->attempts = attempts;
        result->b_permutation = nullptr;
        
        return result;
    }
    
    EXPORT void free_solution(SolutionResult* result) {
        if (result) {
            if (result->b_permutation) delete[] result->b_permutation;
            delete result;
        }
    }
}