# Computação Gráfica - Híbrido

Repositório de exemplos de códigos em C++ utilizando OpenGL moderna (3.3+) criado para a Atividade Acadêmica Computação Gráfica do curso de graduação em Ciência da Computação - modalidade híbrida - da Unisinos. Ele é estruturado para facilitar a organização dos arquivos e a compilação dos projetos utilizando CMake.

## 📂 Estrutura do Repositório

```plaintext
📂 CGCCHibrido/
├── 📂 include/               # Cabeçalhos e bibliotecas de terceiros
│   ├── 📂 glad/              # Cabeçalhos da GLAD (OpenGL Loader)
│   │   ├── glad.h
│   │   ├── 📂 KHR/           # Diretório com cabeçalhos da Khronos (GLAD)
│   │       ├── khrplatform.h
├── 📂 common/                # Código reutilizável entre os projetos
│   ├── glad.c                # Implementação da GLAD
├── 📂 src/                   # Código-fonte dos exemplos e exercícios
│   ├── Hello3D.cpp           # Exemplo básico de renderização com OpenGL
│   ├── Cube3D.cpp                 # Cubo
│   ├── Textura.cpp                # Texturas
│   ├── Iluminacao.cpp             # Iluminação de phong
│   ├── AtividadeVivencial2.cpp    # Iluminação de 3 pontos
│   ├── Camera.cpp                 # Câmera em primeira pessoa
│   ├── Trajetorias.cpp            # Definindo trajetórias para alguns objetos
├── 📂 build/                 # Diretório gerado pelo CMake (não incluído no repositório)
├── 📂 assets/                # diretório com modelos 3D, texturas, fontes etc
├── 📄 CMakeLists.txt         # Configuração do CMake para compilar os projetos
├── 📄 README.md              # Este arquivo, com a documentação do repositório
├── 📄 GettingStarted.md      # Tutorial detalhado sobre como compilar usando o CMake
```

Siga as instruções detalhadas em [GettingStarted.md](GettingStarted.md) para configurar e compilar o projeto.

## ⚠️ **IMPORTANTE: Baixar a GLAD Manualmente**

Para que o projeto funcione corretamente, é necessário **baixar a GLAD manualmente** utilizando o **GLAD Generator**.

### 🔗 **Acesse o web service do GLAD**:

👉 [GLAD Generator](https://glad.dav1d.de/)

### ⚙️ **Configuração necessária:**

- **API:** OpenGL
- **Version:** 3.3+ (ou superior compatível com sua máquina)
- **Profile:** Core
- **Language:** C/C++

### 📥 **Baixe e extraia os arquivos:**

Após a geração, extraia os arquivos baixados e coloque-os nos diretórios correspondentes:

- Copie **`glad.h`** para `include/glad/`
- Copie **`khrplatform.h`** para `include/glad/KHR/`
- Copie **`glad.c`** para `common/`

🚨 **Sem esses arquivos, a compilação falhará!** É necessário colocar esses arquivos nos diretórios corretos, conforme a orientação acima.

---

## Integração GB — Visualizador de Cenas 3D

**Aluna:** Amanda Vidal | **Disciplina:** Computação Gráfica — Unisinos

Trabalho final que integra todos os conceitos do semestre em um único visualizador funcional.

### Funcionalidades implementadas

- Leitura de múltiplos OBJs com material (ka, kd, ks, ns do `.mtl`) e textura por objeto
- Iluminação de Phong com múltiplas fontes de luz parametrizáveis (toggle individual: teclas 1/2/3)
- Câmera livre em primeira pessoa (WASD + mouse; clique esq. captura, clique dir. libera)
- Seleção de objetos com TAB e transformações individuais (rotação, translação, escala uniforme)
- Animação de trajetória por curva de Bézier cúbica — algoritmo de de Casteljau (tecla T)
- Configuração de cena via arquivo de texto (`assets/scene.txt`)

### Compilação

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build . --target IntegracaoGB
./IntegracaoGB.exe   # rodar a partir da pasta build/
```

### Controles

| Tecla               | Ação                                  |
| ------------------- | ------------------------------------- |
| Clique Esq / Dir    | Capturar / liberar mouse              |
| W A S D / Q E       | Mover câmera                          |
| Mouse               | Orientar câmera                       |
| TAB                 | Selecionar próximo objeto             |
| X Y Z (+Shift)      | Rotacionar objeto (inverso com Shift) |
| Setas / PageUp/Down | Transladar objeto                     |
| `[` `]`             | Escala uniforme                       |
| T                   | Ligar/desligar animação Bézier        |
| 1 2 3               | Ligar/desligar luzes                  |
| R                   | Resetar rotação e escala              |
| ESC                 | Sair                                  |

### Arquivo de cena (`assets/scene.txt`)

```
# CAMERA: pos_x pos_y pos_z  yaw pitch  fov near far
CAMERA 0.0 1.5 7.0  -90.0 -5.0  45.0 0.1 100.0

# LIGHT: pos_x pos_y pos_z  r g b  intensidade
LIGHT  3.0  4.0  3.0   1.0 1.0 1.0  1.2
LIGHT -3.0  2.0  2.0   0.6 0.6 0.9  0.7

# OBJECT: arquivo  pos  rot  escala  [BEZIER pontos...]
OBJECT Suzanne.obj  -2.5 0.0 0.0  0.0 30.0 0.0  1.0
OBJECT Cube.obj      2.5 0.0 0.0  0.0 45.0 0.0  0.8  BEZIER  2.5 0 0  4.5 1.5 -2  4.5 -1.5 2  2.5 0 0
```

### Assets

| Arquivo                       | Origem                                                                     | Processamento                                               |
| ----------------------------- | -------------------------------------------------------------------------- | ----------------------------------------------------------- |
| `Suzanne.obj` + `Suzanne.png` | Modelo padrão do [Blender](https://www.blender.org/) (Suzanne monkey head) | Exportado com normais e UVs, triangularizado no Blender 4.3 |
| `Cube.obj`                    | Primitiva cubo do [Blender](https://www.blender.org/)                      | Exportado triangularizado no Blender 4.3                    |

### Referências

- [LearnOpenGL](https://learnopengl.com/) — Joey de Vries. Referência principal para GLFW, shaders, iluminação e câmera.
- [OpenGL Reference Pages](https://registry.khronos.org/OpenGL-Refpages/gl4/) — Especificação oficial OpenGL 4.x.
- [GLM Documentation](https://glm.g-truc.net/) — Biblioteca matemática.
- [stb_image](https://github.com/nothings/stb) — Sean Barrett (domínio público).
