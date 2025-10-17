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