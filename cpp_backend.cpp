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
    constexpr Point(double x_ = 0, double y_ = 0) noexcept : x(x_), y(y_) {}
    
    // Operador de igualdade aproximada (inline para melhor performance)
    inline bool near_equal(const Point& other, double eps = 1e-9) const noexcept {
        const double dx = x - other.x;
        const double dy = y - other.y;
        return (dx * dx + dy * dy) < eps * eps;
    }
};

// Estrutura de segmento para melhor cache locality
struct Segment {
    Point p1, p2;
    constexpr Segment(const Point& a, const Point& b) noexcept : p1(a), p2(b) {}
};

// Função auxiliar: teste CCW otimizado
inline bool ccw(const Point& A, const Point& B, const Point& C) noexcept {
    return (C.y - A.y) * (B.x - A.x) > (B.y - A.y) * (C.x - A.x);
}

// Função auxiliar: verifica se dois segmentos se cruzam (OTIMIZADA)
inline bool segments_intersect(const Point& p1, const Point& p2, 
                               const Point& p3, const Point& p4) noexcept {
    constexpr double eps = 1e-9;
    
    // Verificar se compartilham endpoints (versão otimizada)
    if (p1.near_equal(p3, eps) || p1.near_equal(p4, eps) ||
        p2.near_equal(p3, eps) || p2.near_equal(p4, eps)) {
        return false;
    }
    
    // Teste CCW otimizado (apenas duas comparações)
    const bool ccw1 = ccw(p1, p3, p4);
    const bool ccw2 = ccw(p2, p3, p4);
    const bool ccw3 = ccw(p1, p2, p3);
    const bool ccw4 = ccw(p1, p2, p4);
    
    return (ccw1 != ccw2) && (ccw3 != ccw4);
}

// Versão otimizada usando segmentos pré-computados
inline bool segment_intersects_any(const Segment& seg, 
                                   const std::vector<Segment>& existing_segments) noexcept {
    for (const auto& existing : existing_segments) {
        if (segments_intersect(seg.p1, seg.p2, existing.p1, existing.p2)) {
            return true;
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
        std::vector<Point> pointsA, pointsB;
        pointsA.reserve(n);
        pointsB.reserve(n_acima);
        
        const Point target(target_x, target_y);
        
        for (int i = 0; i < n; ++i) {
            pointsA.emplace_back(points_A_x[i], points_A_y[i]);
        }
        
        for (int i = 0; i < n_acima; ++i) {
            pointsB.emplace_back(points_B_x[i], points_B_y[i]);
        }
        
        // Criar permutação inicial de B
        std::vector<int> b_perm(n_acima);
        for (int i = 0; i < n_acima; ++i) b_perm[i] = i;
        
        // Pré-alocar vetor de segmentos (evita realocações)
        std::vector<Segment> segments;
        segments.reserve(n_acima * 2 + n_abaixo);
        
        long long attempts = 0;
        
        // Iterar sobre todas as permutações de B apenas
        do {
            ++attempts;
            
            if (max_iterations > 0 && attempts > max_iterations) {
                goto end_search;
            }
            
            // Construir solução incrementalmente com segmentos
            segments.clear();
            bool has_crossing = false;
            
            // Adicionar segmentos de pontos acima (via B)
            for (int i = 0; i < n_acima; ++i) {
                const int global_a_idx = idx_acima[i];
                const int b_idx = b_perm[i];
                
                const Point& pt_a = pointsA[global_a_idx];
                const Point& pt_b = pointsB[b_idx];
                
                // Criar dois segmentos: A->B e B->Target
                const Segment seg1(pt_a, pt_b);
                const Segment seg2(pt_b, target);
                
                // Verificar cruzamentos apenas com segmentos existentes
                if (segment_intersects_any(seg1, segments) ||
                    segment_intersects_any(seg2, segments)) {
                    has_crossing = true;
                    break;
                }
                
                segments.push_back(seg1);
                segments.push_back(seg2);
            }
            
            if (has_crossing) continue;
            
            // Adicionar segmentos de pontos abaixo (direto ao target)
            for (int i = 0; i < n_abaixo; ++i) {
                const int global_a_idx = idx_abaixo[i];
                const Point& pt_a = pointsA[global_a_idx];
                
                const Segment seg(pt_a, target);
                
                if (segment_intersects_any(seg, segments)) {
                    has_crossing = true;
                    break;
                }
                
                segments.push_back(seg);
            }
            
            if (!has_crossing) {
                // SOLUÇÃO ENCONTRADA!
                SolutionResult* result = new SolutionResult;
                result->found = true;
                result->attempts = attempts;
                
                result->b_permutation = new int[n_acima];
                std::memcpy(result->b_permutation, b_perm.data(), n_acima * sizeof(int));
                
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