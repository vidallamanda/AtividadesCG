/* Textura
 *
 * Adaptado por Amanda Vidal
 * para a disciplina de Computação Gráfica - Unisinos
 * Última atualização: 28/05/2026
 *
 * Baseado em Cube3D.cpp — carrega modelo .OBJ com texturas via .MTL
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
int loadSimpleOBJ(string filePATH, int &nVertices, string &texturePath);
GLuint loadTexture(string filePath);

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;

// Variáveis de transformação
vec3 objPosition(0.0f, 0.0f, 0.0f);
float scaleValue = 1.0f;
float angleX = 0.0f;
float angleY = 0.0f;
float angleZ = 0.0f;
// Vertex Shader — recebe posição e coordenada de textura
const GLchar *vertexShaderSource = R"(
#version 400

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

out vec2 fragTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    fragTexCoord = texCoord;
}
)";

// Fragment Shader — amostra a textura
const GLchar *fragmentShaderSource = R"(
#version 400

in vec2 fragTexCoord;

out vec4 color;

uniform sampler2D tex;

void main()
{
    color = texture(tex, fragTexCoord);
}
)";

// MAIN
int main()
{
    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Textura OBJ -- Amanda Vidal", nullptr, nullptr);
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
    GLuint VAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", nVertices, texturePath);

    if (VAO == -1)
    {
        cout << "Erro ao carregar o modelo OBJ." << endl;
        glfwTerminate();
        return -1;
    }

    GLuint texID = loadTexture("../assets/Modelos3D/" + texturePath);

    glUseProgram(shaderID);

    glUniform1i(glGetUniformLocation(shaderID, "tex"), 0);

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
        cout << "Erro Vertex Shader\n"
             << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "Erro Fragment Shader\n"
             << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "Erro Link\n"
             << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

// Carrega arquivo .OBJ
// Agora armazena: x, y, z, s, t
// Lê o .MTL para obter o nome da textura
int loadSimpleOBJ(string filePATH, int &nVertices, string &texturePath)
{
    vector<vec3> vertices;
    vector<vec2> texCoords;
    vector<vec3> normals;
    vector<GLfloat> vBuffer;

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
                    {
                        mtlSS >> texturePath;
                        break;
                    }
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

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 5;
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
