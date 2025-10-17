import numpy as np
import matplotlib.pyplot as plt
import ctypes
import os
import sys
from pathlib import Path
import math

# Tentar carregar a biblioteca C++
dll_loaded = False

# Detectar extensão correta baseado no sistema operacional
import platform
system = platform.system()

if system == "Windows":
    lib_name = "planar_solver.dll"
    subfolder = "windows"
else:  # Linux
    lib_name = "planar_solver.so"
    subfolder = "linux"

dll_path = Path(__file__).parent / subfolder / lib_name # linux/planar_solver.so ou windows/planar_solver.dll


if dll_path.exists():
    solver_lib = ctypes.CDLL(str(dll_path))
    
    # Definir estrutura de resultado
    class SolutionResult(ctypes.Structure):
        _fields_ = [
            ("found", ctypes.c_bool),
            ("b_permutation", ctypes.POINTER(ctypes.c_int)),
            ("attempts", ctypes.c_longlong)
        ]
    
    # Configurar assinaturas das funções
    solver_lib.find_solution_single_target.argtypes = [
        ctypes.POINTER(ctypes.c_double),  # points_A_x
        ctypes.POINTER(ctypes.c_double),  # points_A_y
        ctypes.c_int,                      # n
        ctypes.POINTER(ctypes.c_int),     # idx_acima
        ctypes.c_int,                      # n_acima
        ctypes.POINTER(ctypes.c_int),     # idx_abaixo
        ctypes.c_int,                      # n_abaixo
        ctypes.POINTER(ctypes.c_double),  # points_B_x
        ctypes.POINTER(ctypes.c_double),  # points_B_y
        ctypes.c_double,                   # target_x
        ctypes.c_double,                   # target_y
        ctypes.c_longlong                  # max_iterations
    ]
    solver_lib.find_solution_single_target.restype = ctypes.POINTER(SolutionResult)
    
    solver_lib.free_solution.argtypes = [ctypes.POINTER(SolutionResult)]
    solver_lib.free_solution.restype = None
    
    dll_loaded = True
    print(f"✅ Biblioteca C++ carregada com sucesso! ({lib_name})")
    print("   Usando backend otimizado.")
else:
    print(f"⚠️  Biblioteca {lib_name} não encontrada.")

def connect_points_cpp(pontos_A, meio_navio, tamanho_mooring):
    """Versão usando backend C++ via biblioteca compartilhada."""
    n = len(pontos_A)
    
    idx_acima = [i for i, pt in enumerate(pontos_A) if pt[1] > meio_navio]
    idx_abaixo = [i for i, pt in enumerate(pontos_A) if pt[1] <= meio_navio]
    
    n_acima = len(idx_acima)
    n_abaixo = len(idx_abaixo)
    
    pontos_B = [(tamanho_mooring + 50 * i, meio_navio) for i in range(n_acima)]
    # TODOS os pontos terminam em (0, 0)
    pontos_C = [(0, 0)] * n
    
    print(f"Configuração:")
    print(f"  • Total pontos A: {n}")
    print(f"  • Pontos acima: {n_acima}, Pontos abaixo: {n_abaixo}")
    print(f"  • TODOS os pontos terminam em (0, 0)")
    
    total = math.factorial(n_acima)  # Apenas permutação de B!
    print(f"  • Combinações possíveis: {total:,} (apenas B)")
    print(f"\n🚀 Iniciando busca com C++ backend...\n")
    
    # Preparar arrays para C++
    points_A_x = (ctypes.c_double * n)(*[pt[0] for pt in pontos_A])
    points_A_y = (ctypes.c_double * n)(*[pt[1] for pt in pontos_A])
    
    idx_acima_arr = (ctypes.c_int * n_acima)(*idx_acima)
    idx_abaixo_arr = (ctypes.c_int * n_abaixo)(*idx_abaixo)
    
    points_B_x = (ctypes.c_double * n_acima)(*[pt[0] for pt in pontos_B])
    points_B_y = (ctypes.c_double * n_acima)(*[pt[1] for pt in pontos_B])
    
    # Todos os pontos C são (0, 0)
    points_C_x = (ctypes.c_double * n)(*[0.0] * n)
    points_C_y = (ctypes.c_double * n)(*[0.0] * n)
    
    # Chamar função C++
    result_ptr = solver_lib.find_solution_single_target(
        points_A_x, points_A_y, n,
        idx_acima_arr, n_acima,
        idx_abaixo_arr, n_abaixo,
        points_B_x, points_B_y,
        0.0, 0.0,  # target (0, 0)
        0  # max_iterations (0 = sem limite)
    )
    
    result = result_ptr.contents
    
    if result.found:
        print(f"✅ Solução encontrada após {result.attempts:,} tentativas!")
        
        # Extrair permutação de B
        b_perm = [result.b_permutation[i] for i in range(n_acima)]
        
        # Construir conexões - TODOS terminam em (0, 0)
        connections = [None] * n
        target = (0, 0)
        
        for i, global_a_idx in enumerate(idx_acima):
            b_idx = b_perm[i]
            pt_a = pontos_A[global_a_idx]
            pt_b = pontos_B[b_idx]
            connections[global_a_idx] = [pt_a, pt_b, target]
        
        for global_a_idx in idx_abaixo:
            pt_a = pontos_A[global_a_idx]
            connections[global_a_idx] = [pt_a, (-1, -1), target]
        
        # Liberar memória
        solver_lib.free_solution(result_ptr)
        
        return connections
    else:
        print(f"❌ Nenhuma solução após {result.attempts:,} tentativas!")
        solver_lib.free_solution(result_ptr)
        return None


def connect_points(pontos_A, meio_navio, tamanho_mooring):
    """Wrapper do C++ backend."""
    return connect_points_cpp(pontos_A, meio_navio, tamanho_mooring)


def plot_connections(pontos_A, connections, meio_navio, tamanho_mooring):
    """Plota as conexões."""
    if connections is None:
        print("❌ Nenhuma conexão para plotar.")
        return
    
    fig, ax = plt.subplots(figsize=(16, 10))
    
    pontos_B = []
    # Todos convergem para (0, 0)
    ponto_C = (0, 0)
    
    for conn in connections:
        if conn[1] != (-1, -1):
            pontos_B.append(conn[1])
    
    ax.axhline(y=meio_navio, color='gray', linestyle='--', 
               linewidth=2.5, alpha=0.6, label='Meio Navio', zorder=1)
    
    for i, conn in enumerate(connections):
        if conn[1] == (-1, -1):
            ax.plot([conn[0][0], conn[2][0]], [conn[0][1], conn[2][1]], 
                    'g-', linewidth=3, alpha=0.8, zorder=2)
        else:
            ax.plot([conn[0][0], conn[1][0]], [conn[0][1], conn[1][1]], 
                    'b-', linewidth=3, alpha=0.8, zorder=2)
            ax.plot([conn[1][0], conn[2][0]], [conn[1][1], conn[2][1]], 
                    'b-', linewidth=3, alpha=0.8, zorder=2)
    
    pontos_A_arr = np.array(pontos_A)
    ax.scatter(pontos_A_arr[:, 0], pontos_A_arr[:, 1], 
               c='red', s=300, marker='o', edgecolors='darkred', 
               linewidths=3, label='Pontos A', zorder=5)
    
    if pontos_B:
        pontos_B_arr = np.array(pontos_B)
        ax.scatter(pontos_B_arr[:, 0], pontos_B_arr[:, 1], 
                   c='blue', s=300, marker='s', edgecolors='darkblue', 
                   linewidths=3, label='Pontos B', zorder=5)
    
    # Apenas um ponto C em (0, 0)
    ax.scatter([ponto_C[0]], [ponto_C[1]], 
               c='green', s=400, marker='^', edgecolors='darkgreen', 
               linewidths=4, label='Ponto C (origem)', zorder=5)
    
    for i, pt in enumerate(pontos_A):
        ax.annotate(f'A{i}', xy=pt, xytext=(15, 15), 
                    textcoords='offset points', fontsize=13, 
                    fontweight='bold',
                    bbox=dict(boxstyle='round,pad=0.5', 
                             facecolor='yellow', alpha=0.9, 
                             edgecolor='black', linewidth=2))
    
    ax.set_xlabel('X (metros)', fontsize=16, fontweight='bold')
    ax.set_ylabel('Y (metros)', fontsize=16, fontweight='bold')
    ax.set_title('SOLUÇÃO SEM CRUZAMENTOS - TODOS PARA (0,0)', 
                 fontsize=18, fontweight='bold', pad=20, color='green')
    ax.legend(fontsize=14, loc='best')
    ax.grid(True, alpha=0.3, linestyle=':', linewidth=1)
    
    # Ajustar limites incluindo origem (0,0)
    all_x = [pt[0] for pt in pontos_A] + [0]
    all_y = [pt[1] for pt in pontos_A] + [0]
    if pontos_B:
        all_x.extend([pt[0] for pt in pontos_B])
        all_y.extend([pt[1] for pt in pontos_B])
    
    margin_x = max((max(all_x) - min(all_x)) * 0.1, 100)
    margin_y = max((max(all_y) - min(all_y)) * 0.1, 10)
    
    ax.set_xlim(min(all_x) - margin_x, max(all_x) + margin_x)
    ax.set_ylim(min(all_y) - margin_y, max(all_y) + margin_y)
    
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    import time
    
    meio_navio = 150
    tamanho_mooring = 2000
    
    # Caso com 5 pontos
    # Caso com 20 pontos
    pontos_A = [
        (1750, 200),  # A0  - acima
        (1780, 95),   # A1  - abaixo
        (1810, 185),  # A2  - acima
        (1840, 110),  # A3  - abaixo
        (1870, 175),  # A4  - acima
        (1900, 125),  # A5  - abaixo
        (1930, 195),  # A6  - acima
        (1960, 105),  # A7  - abaixo
        (1990, 165),  # A8  - acima
        (2020, 90),   # A9  - abaixo
        (2050, 180),  # A10 - acima
        (2080, 115),  # A11 - abaixo
        (2110, 170),  # A12 - acima
        (2140, 100),  # A13 - abaixo
        (2170, 190),  # A14 - acima
        (2200, 85),   # A15 - abaixo
        (1720, 160),  # A16 - acima
        (1850, 75),   # A17 - abaixo
        (2000, 155),  # A18 - acima
        (2100, 70),   # A19 - abaixo
    ]
    
    print("="*70)
    print("BUSCA DE SOLUÇÃO SEM CRUZAMENTOS - TODOS PARA (0,0)")
    print("="*70)
    print()
    
    start_time = time.time()
    connections = connect_points(pontos_A, meio_navio, tamanho_mooring)
    elapsed = time.time() - start_time
    
    print(f"\n⏱️  Tempo total: {elapsed:.3f} segundos")
    
    if connections:
        print("\n" + "="*70)
        print("CONEXÕES FINAIS:")
        print("="*70)
        for i, conn in enumerate(connections):
            if conn[1] == (-1, -1):
                print(f"A{i}: {conn[0]} → {conn[2]}")
            else:
                print(f"A{i}: {conn[0]} → {conn[1]} → {conn[2]}")
        
        plot_connections(pontos_A, connections, meio_navio, tamanho_mooring)