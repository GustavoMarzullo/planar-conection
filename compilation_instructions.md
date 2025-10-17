# Instruções de Compilação da DLL C++ no Linux/WSL    
## Otimizações Avançadas

### Para compilar ambos ao mesmo tempo
```bash
chmod +x build.sh
./build.sh
```

### Windows - Visual Studio (Máxima Performance)

```bash
x86_64-w64-mingw32-g++ -O3 -march=x86-64 -mtune=generic -flto -shared -static-libgcc -static-libstdc++ -o planar_solver.dll cpp_backend.cpp -std=c++17
```

### GCC/Clang (Máxima Performance)

```bash
g++ -O3 -march=native -mtune=native -flto -shared -fPIC -o planar_solver.so cpp_backend.cpp -std=c++17
```

## ⚠️ Notas Importantes

1. **`-ffast-math`**: Esta flag assume que não há NaN/Inf nos cálculos. Se houver casos especiais, remova esta flag.

2. **`-march=native`** (Linux): Otimiza para o processador atual. Binário pode não funcionar em processadores mais antigos. Use `-march=x86-64` para compatibilidade.

3. **Cache locality**: As otimizações de estruturas de dados aproveitam o fato de que acessos sequenciais à memória são muito mais rápidos.