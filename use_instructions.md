## Visão geral

`planar_connection.py` carrega um backend C++ (biblioteca compartilhada) para encontrar conexões entre pontos sem cruzamentos, onde todos convergem para o alvo (0, 0). O script detecta o sistema operacional e tenta carregar a biblioteca correta automaticamente.

## Pré-requisitos

- Compilador para gerar a biblioteca C++:
  - **Linux**: `g++` (GCC)
  - **Windows**: `x86_64-w64-mingw32-g++` (MinGW-w64) via MSYS2/Git Bash/WSL

## Como compilar as bibliotecas

Use o script de build do projeto. Ele compila ambos os artefatos e salva nas pastas corretas:

```bash
chmod +x build.sh
./build.sh
```

No Windows puro (PowerShell/CMD), rode via Git Bash/MSYS2/WSL:

```bash
bash build.sh
```

Resultado esperado:
- Linux: `linux/planar_solver.so`
- Windows: `windows/planar_solver.dll`

## Onde o `planar_connection.py` procura a biblioteca

O script define automaticamente:
- Windows: `windows/planar_solver.dll`
- Linux: `linux/planar_solver.so`

Isto é, ele tenta carregar a biblioteca em `Path(__file__).parent / subfolder / lib_name`.

## Executar um exemplo rápido

Com as bibliotecas já compiladas nas pastas `linux/` e `windows/`, execute:

```bash
python planar_connection.py
```

Você deverá ver logs indicando que a biblioteca foi carregada e um gráfico de conexões sem cruzamentos, com todos os caminhos convergindo para (0, 0).

## Usando como módulo (API)

Funções principais expostas pelo script:

- `connect_points(pontos_A, meio_navio, tamanho_mooring)`
  - Wrapper que chama o backend C++.
  - Parâmetros:
    - `pontos_A`: lista de tuplas `(x, y)` dos pontos A
    - `meio_navio`: valor Y que separa pontos acima/abaixo
    - `tamanho_mooring`: comprimento base utilizado para posicionar pontos B acima
  - Retorno: lista `connections` com elementos `[pontoA, pontoB_ou_(-1,-1), (0,0)]` ou `None` se não houver solução.

Exemplo de uso:

```python
from planar_connection import connect_points

pontos_A = [(1750, 200), (1780, 95), (1810, 185)]
meio_navio = 150
tamanho_mooring = 2000

connections = connect_points(pontos_A, meio_navio, tamanho_mooring)
if connections is None:
    print("Sem solução")
else:
    for i, conn in enumerate(connections):
        print(f"A{i}: {conn}")
```

## Solução de problemas

- "Biblioteca não encontrada":
  - Certifique-se de que `linux/planar_solver.so` (Linux) ou `windows/planar_solver.dll` (Windows) foi gerado.
  - Rode o `./build.sh` novamente e verifique se o compilador está no PATH.

- "Falha ao carregar DLL no Windows":
  - Prefira executar via Git Bash/MSYS2/WSL para garantir ambiente POSIX.
  - Verifique dependências do MinGW-w64. O build usa `-static-libgcc -static-libstdc++` para reduzir dependências dinâmicas.

## Estrutura relevante

```
Conectar_pontos/
  build.sh
  planar_connection.py
  linux/
    planar_solver.so
  windows/
    planar_solver.dll
```