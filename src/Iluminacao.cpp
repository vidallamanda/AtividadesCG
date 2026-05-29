/* Iluminacao
 *
 * Adaptado por Amanda Vidal
 * para a disciplina de Computação Gráfica - Unisinos
 * Última atualização: 28/05/2026
 *
 * Baseado em Textura.cpp — adiciona iluminação de Phong
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STB Image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

// Protótipos
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
GLuint setupShader();
int loadSimpleOBJ(string filePATH, int &nVertices, string &texturePath,
                  vec3 &ka, vec3 &ks, float &ns);
GLuint loadTexture(string filePath);

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;

// Variáveis de transformação
vec3 objPosition(0.0f, 0.0f, 0.0f);
float scaleValue = 1.0f;
float angleX = 0.0f;
float angleY = 0.0f;
float angleZ = 0.0f;

// Vertex Shader — passa posição, normal e texCoord ao fragment shader
const GLchar *vertexShaderSource = R"(
#version 400

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

out vec3 fragNormal;
out vec3 fragPos;
out vec2 fragTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position  = projection * view * model * vec4(position, 1.0);
    fragPos      = vec3(model * vec4(position, 1.0));
    fragNormal   = mat3(transpose(inverse(model))) * normal;
    fragTexCoord = texCoord;
}
)";

// Fragment Shader — calcula iluminação de Phong
const GLchar *fragmentShaderSource = R"(
#version 400

in vec3 fragNormal;
in vec3 fragPos;
in vec2 fragTexCoord;

out vec4 color;

uniform sampler2D tex;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 ka;
uniform vec3 ks;
uniform float ns;
uniform vec3 lightColor;

void main()
{
    vec3 texColor = vec3(texture(tex, fragTexCoord));

    // Parcela ambiente
    vec3 ambient = ka * lightColor;

    // Parcela difusa
    vec3 N     = normalize(fragNormal);
    vec3 L     = normalize(lightPos - fragPos);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * lightColor;

    // Parcela especular
    vec3 V     = normalize(viewPos - fragPos);
    vec3 R     = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), ns);
    vec3 specular = ks * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * texColor;
    color = vec4(result, 1.0);
}
)";

// MAIN
int main()
{
    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Iluminacao Phong -- Amanda Vidal", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Erro ao iniciar GLAD" << endl;
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShader();

    int nVertices;
    string texturePath;
    vec3 ka, ks;
    float ns;
    GLuint VAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", nVertices, texturePath, ka, ks, ns);

    if (VAO == (GLuint)-1)
    {
        cout << "Erro ao carregar o modelo OBJ." << endl;
        glfwTerminate();
        return -1;
    }

    GLuint texID = loadTexture("../assets/Modelos3D/" + texturePath);

    glUseProgram(shaderID);

    glUniform1i(glGetUniformLocation(shaderID, "tex"), 0);

    // Coeficientes de material lidos do MTL
    glUniform3fv(glGetUniformLocation(shaderID, "ka"), 1, value_ptr(ka));
    glUniform3fv(glGetUniformLocation(shaderID, "ks"), 1, value_ptr(ks));
    glUniform1f (glGetUniformLocation(shaderID, "ns"), ns);

    // Luz e câmera (estáticos neste exemplo)
    vec3 lightPos(2.5f, 2.5f, 2.5f);
    vec3 cameraPos(0.0f, 0.0f, 5.0f);
    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"),   1, value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderID, "viewPos"),    1, value_ptr(cameraPos));
    glUniform3fv(glGetUniformLocation(shaderID, "lightColor"), 1, value_ptr(vec3(1.0f)));

    mat4 projection = perspective(radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    mat4 view = mat4(1.0f);
    view = translate(view, vec3(0.0f, 0.0f, -5.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, value_ptr(view));

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);

        mat4 model = mat4(1.0f);
        model = translate(model, objPosition);
        model = rotate(model, radians(angleX), vec3(1.0f, 0.0f, 0.0f));
        model = rotate(model, radians(angleY), vec3(0.0f, 1.0f, 0.0f));
        model = rotate(model, radians(angleZ), vec3(0.0f, 0.0f, 1.0f));
        model = scale(model, vec3(scaleValue));

        glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, nVertices);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

// Callback teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_A)
            objPosition.x -= 0.1f;
        if (key == GLFW_KEY_D)
            objPosition.x += 0.1f;
        if (key == GLFW_KEY_I)
            objPosition.y += 0.1f;
        if (key == GLFW_KEY_J)
            objPosition.y -= 0.1f;
        if (key == GLFW_KEY_W)
            objPosition.z -= 0.1f;
        if (key == GLFW_KEY_S)
            objPosition.z += 0.1f;

        if (key == GLFW_KEY_RIGHT_BRACKET)
            scaleValue += 0.1f;
        if (key == GLFW_KEY_LEFT_BRACKET)
            scaleValue -= 0.1f;

        if (key == GLFW_KEY_X)
            angleX += 5.0f;
        if (key == GLFW_KEY_Y)
            angleY += 5.0f;
        if (key == GLFW_KEY_Z)
            angleZ += 5.0f;
    }
}

GLuint setupShader()
{
    GLint success;
    GLchar infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "Erro Vertex Shader\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "Erro Fragment Shader\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "Erro Link\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

// Carrega .OBJ e .MTL
// Layout do vBuffer: x y z | nx ny nz | s t  (8 floats por vértice)
int loadSimpleOBJ(string filePATH, int &nVertices, string &texturePath,
                  vec3 &ka, vec3 &ks, float &ns)
{
    vector<vec3> vertices;
    vector<vec2> texCoords;
    vector<vec3> normals;
    vector<GLfloat> vBuffer;

    // Valores padrão caso o MTL não os contenha
    ka = vec3(0.2f);
    ks = vec3(0.5f);
    ns = 32.0f;

    string folder = filePATH.substr(0, filePATH.find_last_of("/\\") + 1);

    ifstream arqEntrada(filePATH);
    if (!arqEntrada.is_open())
    {
        cerr << "Erro ao tentar ler o arquivo " << filePATH << endl;
        return -1;
    }

    string line;
    while (getline(arqEntrada, line))
    {
        istringstream ssline(line);
        string word;
        ssline >> word;

        if (word == "mtllib")
        {
            string mtlFile;
            ssline >> mtlFile;
            ifstream mtlIn(folder + mtlFile);
            if (mtlIn.is_open())
            {
                string mtlLine;
                while (getline(mtlIn, mtlLine))
                {
                    istringstream mtlSS(mtlLine);
                    string mtlWord;
                    mtlSS >> mtlWord;
                    if (mtlWord == "map_Kd")
                        mtlSS >> texturePath;
                    else if (mtlWord == "Ka")
                        mtlSS >> ka.r >> ka.g >> ka.b;
                    else if (mtlWord == "Ks")
                        mtlSS >> ks.r >> ks.g >> ks.b;
                    else if (mtlWord == "Ns")
                        mtlSS >> ns;
                }
                mtlIn.close();
            }
        }
        else if (word == "v")
        {
            vec3 v;
            ssline >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (word == "vt")
        {
            vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn")
        {
            vec3 vn;
            ssline >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (word == "f")
        {
            while (ssline >> word)
            {
                int vi = 0, ti = 0, ni = 0;
                istringstream ss(word);
                string index;

                if (getline(ss, index, '/'))
                    vi = !index.empty() ? stoi(index) - 1 : 0;
                if (getline(ss, index, '/'))
                    ti = !index.empty() ? stoi(index) - 1 : 0;
                if (getline(ss, index))
                    ni = !index.empty() ? stoi(index) - 1 : 0;

                // Posição
                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                // Normal
                vBuffer.push_back(normals[ni].x);
                vBuffer.push_back(normals[ni].y);
                vBuffer.push_back(normals[ni].z);
                // Textura
                vBuffer.push_back(texCoords[ti].s);
                vBuffer.push_back(texCoords[ti].t);
            }
        }
    }

    arqEntrada.close();

    cout << "Gerando buffer de geometria..." << endl;

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Atributo 0: posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Atributo 1: normal (nx, ny, nz)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Atributo 2: texCoord (s, t)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 8;
    return VAO;
}

GLuint loadTexture(string filePath)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
    if (data)
    {
        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        cout << "Falha ao carregar textura: " << filePath << endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}
