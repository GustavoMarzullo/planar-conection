#!/usr/bin/env bash
set -u -o pipefail

# Build script for planar solver - compila para Linux (.so) e Windows (.dll) na mesma execução
# Requisitos:
#  - Para .so: g++ disponível no PATH
#  - Para .dll: x86_64-w64-mingw32-g++ disponível no PATH (MinGW-w64)

# Criar pastas de destino
mkdir -p linux windows

LINUX_CMD=(g++ -O3 -march=native -mtune=native -flto -ffast-math -funroll-loops -finline-functions -fomit-frame-pointer -shared -fPIC -o linux/planar_solver.so cpp_backend.cpp -std=c++17)
WIN_CMD=(x86_64-w64-mingw32-g++ -O3 -march=x86-64 -mtune=generic -flto -ffast-math -funroll-loops -finline-functions -shared -static-libgcc -static-libstdc++ -o windows/planar_solver.dll cpp_backend.cpp -std=c++17)

linux_status=1
win_status=1

echo "==> Iniciando build múltiplo"

if command -v g++ >/dev/null 2>&1; then
    echo "[Linux] Compilando linux/planar_solver.so..."
    if "${LINUX_CMD[@]}"; then
        linux_status=0
        echo "[Linux] Sucesso: linux/planar_solver.so gerado."
    else
        linux_status=$?
        echo "[Linux] Falha (código ${linux_status})." >&2
    fi
else
    echo "[Linux] g++ não encontrado no PATH; pulando." >&2
fi

if command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
    echo "[Windows] Compilando windows/planar_solver.dll (MinGW-w64)..."
    if "${WIN_CMD[@]}"; then
        win_status=0
        echo "[Windows] Sucesso: windows/planar_solver.dll gerado."
    else
        win_status=$?
        echo "[Windows] Falha (código ${win_status})." >&2
    fi
else
    echo "[Windows] x86_64-w64-mingw32-g++ não encontrado no PATH; pulando." >&2
fi

echo "==> Resumo: Linux status=${linux_status}, Windows status=${win_status}"

# retorna 0 se pelo menos um build deu certo
if [ ${linux_status} -eq 0 ] || [ ${win_status} -eq 0 ]; then
    exit 0
else
    exit 1
fi


