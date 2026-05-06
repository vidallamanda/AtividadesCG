/* Cube 3D
 *
 * Adaptado por Amanda Vidal
 * Última atualização em 05/05/2026
 */

#include <iostream>
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

using namespace glm;

// Protótipos
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

GLuint setupShader();
GLuint setupGeometry();

// Dimensões
const GLuint WIDTH = 800;
const GLuint HEIGHT = 600;

// Variáveis globais
vec3 cubePosition(0.0f, 0.0f, 0.0f);

float scaleValue = 1.0f;

float angleX = 0.0f;
float angleY = 0.0f;
float angleZ = 0.0f;

// Código fonte do Vertex Shader
const GLchar *vertexShaderSource = R"(
#version 400

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

out vec3 fragColor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    fragColor = color;
}
)";

// Código fonte do Fragment Shader
const GLchar *fragmentShaderSource = R"(
#version 400

in vec3 fragColor;

out vec4 color;

void main()
{
    color = vec4(fragColor, 1.0);
}
)";

// MAIN
int main()
{
    glfwInit();

    GLFWwindow *window = glfwCreateWindow(
        WIDTH,
        HEIGHT,
        "Cubo 3D",
        nullptr,
        nullptr
    );

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

    GLuint VAO = setupGeometry();

    glUseProgram(shaderID);

    // Matrizes
    mat4 projection = perspective(
        radians(45.0f),
        (float)WIDTH / HEIGHT,
        0.1f,
        100.0f
    );

    mat4 view = mat4(1.0f);

    view = translate(view, vec3(0.0f, 0.0f, -8.0f));

    glUniformMatrix4fv(
        glGetUniformLocation(shaderID, "projection"),
        1,
        GL_FALSE,
        value_ptr(projection)
    );

    glUniformMatrix4fv(
        glGetUniformLocation(shaderID, "view"),
        1,
        GL_FALSE,
        value_ptr(view)
    );

    // Instâncias dos cubos
    vector<vec3> cubePositions = {
        vec3(0.0f, 0.0f, 0.0f),
        vec3(3.0f, 0.0f, 0.0f),
        vec3(-3.0f, 0.0f, 0.0f)
    };

    // Loop principal
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(VAO);

        for (GLuint i = 0; i < cubePositions.size(); i++)
        {
            mat4 model = mat4(1.0f);

            vec3 finalPosition = cubePositions[i];

            if (i == 0)
            {
                finalPosition += cubePosition;
            }

            model = translate(model, finalPosition);

            model = rotate(model, radians(angleX), vec3(1.0f, 0.0f, 0.0f));
            model = rotate(model, radians(angleY), vec3(0.0f, 1.0f, 0.0f));
            model = rotate(model, radians(angleZ), vec3(0.0f, 0.0f, 1.0f));

            model = scale(model, vec3(scaleValue));

            glUniformMatrix4fv(
                glGetUniformLocation(shaderID, "model"),
                1,
                GL_FALSE,
                value_ptr(model)
            );

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glfwTerminate();

    return 0;
}

// Callback teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        // Movimento X
        if (key == GLFW_KEY_A)
            cubePosition.x -= 0.1f;

        if (key == GLFW_KEY_D)
            cubePosition.x += 0.1f;

        // Movimento Y
        if (key == GLFW_KEY_I)
            cubePosition.y += 0.1f;

        if (key == GLFW_KEY_J)
            cubePosition.y -= 0.1f;

        // Movimento Z
        if (key == GLFW_KEY_W)
            cubePosition.z -= 0.1f;

        if (key == GLFW_KEY_S)
            cubePosition.z += 0.1f;

        // Escala
        if (key == GLFW_KEY_RIGHT_BRACKET)
            scaleValue += 0.1f;

        if (key == GLFW_KEY_LEFT_BRACKET)
            scaleValue -= 0.1f;

        // Rotação
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

    // Vertex Shader
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

    // Fragment Shader
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

    // Programa
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

GLuint setupGeometry()
{
    GLfloat vertices[] = {

        // Frente - vermelho
        -0.5f,-0.5f, 0.5f, 1,0,0,
         0.5f,-0.5f, 0.5f, 1,0,0,
         0.5f, 0.5f, 0.5f, 1,0,0,

         0.5f, 0.5f, 0.5f, 1,0,0,
        -0.5f, 0.5f, 0.5f, 1,0,0,
        -0.5f,-0.5f, 0.5f, 1,0,0,

        // Trás - verde
        -0.5f,-0.5f,-0.5f, 0,1,0,
         0.5f,-0.5f,-0.5f, 0,1,0,
         0.5f, 0.5f,-0.5f, 0,1,0,

         0.5f, 0.5f,-0.5f, 0,1,0,
        -0.5f, 0.5f,-0.5f, 0,1,0,
        -0.5f,-0.5f,-0.5f, 0,1,0,

        // Esquerda - azul
        -0.5f, 0.5f, 0.5f, 0,0,1,
        -0.5f, 0.5f,-0.5f, 0,0,1,
        -0.5f,-0.5f,-0.5f, 0,0,1,

        -0.5f,-0.5f,-0.5f, 0,0,1,
        -0.5f,-0.5f, 0.5f, 0,0,1,
        -0.5f, 0.5f, 0.5f, 0,0,1,

        // Direita - amarelo
         0.5f, 0.5f, 0.5f, 1,1,0,
         0.5f, 0.5f,-0.5f, 1,1,0,
         0.5f,-0.5f,-0.5f, 1,1,0,

         0.5f,-0.5f,-0.5f, 1,1,0,
         0.5f,-0.5f, 0.5f, 1,1,0,
         0.5f, 0.5f, 0.5f, 1,1,0,

        // Topo - ciano
        -0.5f, 0.5f,-0.5f, 0,1,1,
         0.5f, 0.5f,-0.5f, 0,1,1,
         0.5f, 0.5f, 0.5f, 0,1,1,

         0.5f, 0.5f, 0.5f, 0,1,1,
        -0.5f, 0.5f, 0.5f, 0,1,1,
        -0.5f, 0.5f,-0.5f, 0,1,1,

        // Base - magenta
        -0.5f,-0.5f,-0.5f, 1,0,1,
         0.5f,-0.5f,-0.5f, 1,0,1,
         0.5f,-0.5f, 0.5f, 1,0,1,

         0.5f,-0.5f, 0.5f, 1,0,1,
        -0.5f,-0.5f, 0.5f, 1,0,1,
        -0.5f,-0.5f,-0.5f, 1,0,1
    };

    GLuint VBO, VAO;

    glGenVertexArrays(1, &VAO);

    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    // posição
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(GLfloat),
        (GLvoid*)0
    );

    glEnableVertexAttribArray(0);

    // cor
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(GLfloat),
        (GLvoid*)(3 * sizeof(GLfloat))
    );

    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    return VAO;
}