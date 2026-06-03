/* Câmera em Primeira Pessoa com Iluminação de 3 Pontos
 *
 * Adaptado por Amanda Vidal
 * para a disciplina de Computação Gráfica - Unisinos
 * Última atualização: 02/06/2026
 *
 * Baseado em Iluminacao.cpp — adiciona câmera em primeira pessoa (classe Camera)
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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

// Protótipos
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
GLuint setupShader();
int loadSimpleOBJ(string filePATH, int &nVertices, string &texturePath,
                  vec3 &ka, vec3 &ks, float &ns);
GLuint loadTexture(string filePath);

const GLuint WIDTH = 800, HEIGHT = 600;

// Transformações do objeto
float scaleValue = 1.0f;
float angleX = 0.0f;
float angleY = 0.0f;
float angleZ = 0.0f;

// Luzes
bool keyLightOn  = true;
bool fillLightOn = true;
bool backLightOn = true;

// ============================================================
// Classe Camera — câmera em primeira pessoa
// ============================================================
class Camera
{
public:
    vec3  position;
    vec3  front;
    vec3  up;
    float yaw;          // ângulo horizontal (em graus)
    float pitch;        // ângulo vertical   (em graus)
    float speed;
    float sensitivity;

    Camera(vec3 startPos)
        : position(startPos),
          front(0.0f, 0.0f, -1.0f),
          up(0.0f, 1.0f, 0.0f),
          yaw(-90.0f), pitch(0.0f),
          speed(3.0f), sensitivity(0.1f)
    {}

    mat4 getViewMatrix()
    {
        return lookAt(position, position + front, up);
    }

    void move(GLFWwindow *window, float deltaTime)
    {
        float vel   = speed * deltaTime;
        vec3  right = normalize(cross(front, up));

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += vel * front;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= vel * front;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= right * vel;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += right * vel;
    }

    void rotate(float xoffset, float yoffset)
    {
        yaw   += xoffset * sensitivity;
        pitch += yoffset * sensitivity;

        if (pitch >  89.0f) pitch =  89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        vec3 newFront;
        newFront.x = cos(radians(yaw)) * cos(radians(pitch));
        newFront.y = sin(radians(pitch));
        newFront.z = sin(radians(yaw)) * cos(radians(pitch));
        front = normalize(newFront);
    }
};

// Câmera e estado do mouse (globais para acessar nos callbacks)
Camera camera(vec3(0.0f, 0.0f, 5.0f));

float deltaTime  = 0.0f;
float lastFrame  = 0.0f;

float lastX     = WIDTH  / 2.0f;
float lastY     = HEIGHT / 2.0f;
bool  firstMouse = true;

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

const GLchar *fragmentShaderSource = R"(
#version 400

in vec3 fragNormal;
in vec3 fragPos;
in vec2 fragTexCoord;

out vec4 color;

uniform sampler2D tex;
uniform vec3 viewPos;
uniform vec3 ka;
uniform vec3 ks;
uniform float ns;

// Luz principal (key light)
uniform vec3 keyLightPos;
uniform vec3 keyLightColor;
uniform int  keyLightOn;

// Luz de preenchimento (fill light)
uniform vec3 fillLightPos;
uniform vec3 fillLightColor;
uniform int  fillLightOn;

// Luz de fundo (back light)
uniform vec3 backLightPos;
uniform vec3 backLightColor;
uniform int  backLightOn;

void main()
{
    vec3 texColor = vec3(texture(tex, fragTexCoord));
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(viewPos - fragPos);

    vec3 ambient = ka * 0.15;

    vec3 result = ambient;

    if (keyLightOn == 1)
    {
        vec3  L    = normalize(keyLightPos - fragPos);
        float dist = length(keyLightPos - fragPos);
        float att  = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);

        float diff    = max(dot(N, L), 0.0);
        vec3  diffuse = diff * keyLightColor * att;

        vec3  R    = reflect(-L, N);
        float spec = pow(max(dot(V, R), 0.0), ns);
        vec3  specular = ks * spec * keyLightColor;

        result += (diffuse + specular);
    }

    if (fillLightOn == 1)
    {
        vec3  L    = normalize(fillLightPos - fragPos);
        float dist = length(fillLightPos - fragPos);
        float att  = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);

        float diff    = max(dot(N, L), 0.0);
        vec3  diffuse = diff * fillLightColor * att;

        vec3  R    = reflect(-L, N);
        float spec = pow(max(dot(V, R), 0.0), ns);
        vec3  specular = ks * spec * fillLightColor;

        result += (diffuse + specular);
    }

    if (backLightOn == 1)
    {
        vec3  L    = normalize(backLightPos - fragPos);
        float dist = length(backLightPos - fragPos);
        float att  = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);

        float diff    = max(dot(N, L), 0.0);
        vec3  diffuse = diff * backLightColor * att;

        vec3  R    = reflect(-L, N);
        float spec = pow(max(dot(V, R), 0.0), ns);
        vec3  specular = ks * spec * backLightColor;

        result += (diffuse + specular);
    }

    color = vec4(result * texColor, 1.0);
}
)";

// MAIN
int main()
{
    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Camera 1ª Pessoa -- Amanda Vidal", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

    glUniform3fv(glGetUniformLocation(shaderID, "ka"), 1, value_ptr(ka));
    glUniform3fv(glGetUniformLocation(shaderID, "ks"), 1, value_ptr(ks));
    glUniform1f (glGetUniformLocation(shaderID, "ns"), ns);

    // Cores das luzes calibradas por função:
    //   Key  (principal)     — branca, intensidade total
    //   Fill (preenchimento) — branca, metade da intensidade
    //   Back (fundo)         — levemente azulada, 70%
    glUniform3fv(glGetUniformLocation(shaderID, "keyLightColor"),  1, value_ptr(vec3(1.0f, 1.0f, 1.0f)));
    glUniform3fv(glGetUniformLocation(shaderID, "fillLightColor"), 1, value_ptr(vec3(0.5f, 0.5f, 0.5f)));
    glUniform3fv(glGetUniformLocation(shaderID, "backLightColor"), 1, value_ptr(vec3(0.7f, 0.7f, 0.9f)));

    // Posições fixas das luzes em torno da origem
    glUniform3fv(glGetUniformLocation(shaderID, "keyLightPos"),  1, value_ptr(vec3( 3.0f,  3.0f,  3.0f)));
    glUniform3fv(glGetUniformLocation(shaderID, "fillLightPos"), 1, value_ptr(vec3(-3.0f,  1.5f,  3.0f)));
    glUniform3fv(glGetUniformLocation(shaderID, "backLightPos"), 1, value_ptr(vec3( 0.0f,  3.0f, -3.0f)));

    mat4 projection = perspective(radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        camera.move(window, deltaTime);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Atualiza view e posição da câmera a cada frame
        mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, value_ptr(view));
        glUniform3fv(glGetUniformLocation(shaderID, "viewPos"), 1, value_ptr(camera.position));

        glUniform1i(glGetUniformLocation(shaderID, "keyLightOn"),  keyLightOn  ? 1 : 0);
        glUniform1i(glGetUniformLocation(shaderID, "fillLightOn"), fillLightOn ? 1 : 0);
        glUniform1i(glGetUniformLocation(shaderID, "backLightOn"), backLightOn ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);

        mat4 model = mat4(1.0f);
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

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_1) keyLightOn  = !keyLightOn;
        if (key == GLFW_KEY_2) fillLightOn = !fillLightOn;
        if (key == GLFW_KEY_3) backLightOn = !backLightOn;
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_RIGHT_BRACKET) scaleValue += 0.1f;
        if (key == GLFW_KEY_LEFT_BRACKET)  scaleValue -= 0.1f;

        if (key == GLFW_KEY_X) angleX += 5.0f;
        if (key == GLFW_KEY_Y) angleY += 5.0f;
        if (key == GLFW_KEY_Z) angleZ += 5.0f;
    }
}

// Callback mouse — rotaciona a câmera
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset =  (float)xpos - lastX;
    float yoffset =  lastY - (float)ypos;   // invertido: y cresce para baixo na tela
    lastX = (float)xpos;
    lastY = (float)ypos;

    camera.rotate(xoffset, yoffset);
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

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                vBuffer.push_back(normals[ni].x);
                vBuffer.push_back(normals[ni].y);
                vBuffer.push_back(normals[ni].z);
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
