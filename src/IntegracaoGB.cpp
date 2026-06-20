/* Integracao GB — Visualizador de Cenas 3D
 * Amanda Vidal — Computacao Grafica — Unisinos
 *
 * Requisitos implementados:
 *  1. Multiplos OBJs com textura e material (ka, kd, ks, ns) por objeto
 *  2. Iluminacao de Phong com N fontes de luz parametrizaveis
 *  3. Camara livre (WASD + mouse)
 *  4. Selecao de objetos (TAB) e transformacoes geometricas individuais
 *  5. Arquivo de configuracao de cena (assets/scene.txt)
 *  6. Animacao de trajetoria por curva de Bezier cubica (tecla P)
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

const GLchar *vertSrc = R"(
#version 400
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(inPos, 1.0);
    gl_Position   = projection * view * worldPos;
    fragPos       = vec3(worldPos);
    fragNormal    = mat3(transpose(inverse(model))) * inNormal;
    fragUV        = inUV;
}
)";

const GLchar *fragSrc = R"(
#version 400

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragUV;

out vec4 outColor;

#define MAX_LIGHTS 8

uniform vec3  lightPos[MAX_LIGHTS];
uniform vec3  lightColor[MAX_LIGHTS];
uniform float lightIntensity[MAX_LIGHTS];
uniform int   numLights;

uniform vec3      ka;
uniform vec3      kd;
uniform vec3      ks;
uniform float     ns;
uniform sampler2D tex;
uniform vec3      viewPos;
uniform bool      selected;

void main()
{
    vec3 texColor = vec3(texture(tex, fragUV));
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(viewPos - fragPos);

    float ambStr = selected ? 0.40 : 0.15;
    vec3 result  = ka * ambStr * texColor;

    for (int i = 0; i < numLights; i++)
    {
        vec3  L    = normalize(lightPos[i] - fragPos);
        float dist = length(lightPos[i] - fragPos);
        float att  = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        float Li   = lightIntensity[i] * att;

        // Difuso
        float diff    = max(dot(N, L), 0.0);
        vec3  diffuse = kd * diff * lightColor[i] * texColor * Li;

        // Especular (Phong)
        vec3  R    = reflect(-L, N);
        float spec = pow(max(dot(V, R), 0.0), max(ns, 1.0));
        vec3  specular = ks * spec * lightColor[i] * Li;

        result += diffuse + specular;
    }

    outColor = vec4(result, 1.0);
}
)";

struct Light {
    vec3  position  = vec3(0.0f);
    vec3  color     = vec3(1.0f);
    float intensity = 1.0f;
    bool  enabled   = true;
};

struct SceneObject {
    GLuint VAO       = 0;
    int    nVerts    = 0;
    GLuint texID     = 0;

    // Propriedades de material (do .mtl)
    vec3  ka = vec3(0.2f);
    vec3  kd = vec3(1.0f);
    vec3  ks = vec3(0.5f);
    float ns = 32.0f;

    // Transformacoes
    vec3  position = vec3(0.0f);
    vec3  rotation = vec3(0.0f);
    float scale    = 1.0f;

    // Animacao por curva de Bezier
    bool         hasAnim  = false;
    bool         animating= false;
    vector<vec3> bezierPts;
    float        animT    = 0.0f;
    float        animSpeed= 0.25f;
};

const int SCR_W = 800, SCR_H = 600;

vector<SceneObject> objects;
vector<Light>       lights;
int  selectedObj = 0;

float camFOV  = 45.0f;
float camNear = 0.1f;
float camFar  = 100.0f;

struct Camera {
    vec3  pos   = vec3(0.0f, 1.5f, 7.0f);
    vec3  front = vec3(0.0f, 0.0f,-1.0f);
    vec3  up    = vec3(0.0f, 1.0f, 0.0f);
    float yaw   = -90.0f;
    float pitch =   0.0f;
    float speed =   5.0f;
    float sens  =   0.1f;

    mat4 viewMatrix() const { return lookAt(pos, pos + front, up); }

    void move(GLFWwindow *w, float dt)
    {
        float v     = speed * dt;
        vec3  right = normalize(cross(front, up));
        if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) pos += v * front;
        if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) pos -= v * front;
        if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) pos -= right * v;
        if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) pos += right * v;
        if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS) pos += v * up;
        if (glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS) pos -= v * up;
    }

    void look(float dx, float dy)
    {
        yaw   += dx * sens;
        pitch  = glm::clamp(pitch + dy * sens, -89.0f, 89.0f);
        front  = normalize(vec3(
            cos(radians(yaw)) * cos(radians(pitch)),
            sin(radians(pitch)),
            sin(radians(yaw)) * cos(radians(pitch))
        ));
    }
} camera;

float deltaTime = 0.0f, lastFrame = 0.0f;
float lastX = SCR_W / 2.0f, lastY = SCR_H / 2.0f;
bool  firstMouse = true;

vec3 bezierEval(const vector<vec3>& pts, float t)
{
    vector<vec3> p = pts;
    int n = (int)p.size();
    for (int k = 1; k < n; k++)
        for (int i = 0; i < n - k; i++)
            p[i] = (1.0f - t) * p[i] + t * p[i + 1];
    return p[0];
}

GLuint buildShader();
GLuint loadOBJ(const string &path, int &nVerts,
               vec3 &ka, vec3 &kd, vec3 &ks, float &ns, string &texFile);
GLuint loadTexture(const string &path);
GLuint whiteTexture();
bool   loadScene(const string &path);
void   keyCallback(GLFWwindow *w, int key, int scancode, int action, int mods);
void   mouseCallback(GLFWwindow *w, double xpos, double ypos);
void   mouseButtonCallback(GLFWwindow *w, int button, int action, int mods);
void   printHelp();

int main()
{
    glfwInit();
    GLFWwindow *window = glfwCreateWindow(SCR_W, SCR_H,
        "Integracao GB — Amanda Vidal", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Erro ao inicializar GLAD\n";
        return -1;
    }

    glViewport(0, 0, SCR_W, SCR_H);
    glEnable(GL_DEPTH_TEST);

    GLuint shader = buildShader();

    if (!loadScene("../assets/scene.txt")) {
        cout << "Falha ao carregar cena.\n";
        glfwTerminate();
        return -1;
    }

    if (objects.empty()) {
        cout << "Nenhum objeto carregado na cena.\n";
        glfwTerminate();
        return -1;
    }

    printHelp();

    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "tex"), 0);

    int nLights = (int)lights.size();
    glUniform1i(glGetUniformLocation(shader, "numLights"), nLights);
    for (int i = 0; i < nLights; i++) {
        glUniform3fv(glGetUniformLocation(shader, ("lightPos["   + to_string(i) + "]").c_str()), 1, value_ptr(lights[i].position));
        glUniform3fv(glGetUniformLocation(shader, ("lightColor[" + to_string(i) + "]").c_str()), 1, value_ptr(lights[i].color));
    }

    mat4 projection = perspective(radians(camFOV), (float)SCR_W / SCR_H, camNear, camFar);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, value_ptr(projection));

    while (!glfwWindowShouldClose(window))
    {
        float now  = (float)glfwGetTime();
        deltaTime  = now - lastFrame;
        lastFrame  = now;

        glfwPollEvents();
        camera.move(window, deltaTime);

        for (auto &obj : objects)
        {
            if (obj.hasAnim && obj.animating)
            {
                obj.animT += obj.animSpeed * deltaTime;
                if (obj.animT > 1.0f) obj.animT -= 1.0f;
                obj.position = bezierEval(obj.bezierPts, obj.animT);
            }
        }

        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (int i = 0; i < nLights; i++) {
            float eff = lights[i].enabled ? lights[i].intensity : 0.0f;
            glUniform1f(glGetUniformLocation(shader, ("lightIntensity[" + to_string(i) + "]").c_str()), eff);
        }

        mat4 view = camera.viewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, value_ptr(view));
        glUniform3fv(glGetUniformLocation(shader, "viewPos"), 1, value_ptr(camera.pos));

        for (int i = 0; i < (int)objects.size(); i++)
        {
            const SceneObject &obj = objects[i];

            // Material
            glUniform3fv(glGetUniformLocation(shader, "ka"), 1, value_ptr(obj.ka));
            glUniform3fv(glGetUniformLocation(shader, "kd"), 1, value_ptr(obj.kd));
            glUniform3fv(glGetUniformLocation(shader, "ks"), 1, value_ptr(obj.ks));
            glUniform1f (glGetUniformLocation(shader, "ns"),     obj.ns);
            glUniform1i (glGetUniformLocation(shader, "selected"), i == selectedObj ? 1 : 0);

            // Textura
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, obj.texID);

            // Matriz model
            mat4 model = translate(mat4(1.0f), obj.position);
            model = rotate(model, radians(obj.rotation.x), vec3(1.0f, 0.0f, 0.0f));
            model = rotate(model, radians(obj.rotation.y), vec3(0.0f, 1.0f, 0.0f));
            model = rotate(model, radians(obj.rotation.z), vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, vec3(obj.scale));

            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, value_ptr(model));

            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.nVerts);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    for (auto &obj : objects) glDeleteVertexArrays(1, &obj.VAO);
    glfwTerminate();
    return 0;
}

void mouseButtonCallback(GLFWwindow *w, int button, int action, int mods)
{
    (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (glfwGetInputMode(w, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) {
            glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
            cout << "Mouse capturado. Clique direito para liberar.\n";
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        if (glfwGetInputMode(w, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            cout << "Mouse liberado. Clique esquerdo para capturar novamente.\n";
        }
    }
}

void keyCallback(GLFWwindow *w, int key, int scancode, int action, int mods)
{
    (void)scancode;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(w, GL_TRUE);

    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    if (key == GLFW_KEY_1 && (int)lights.size() > 0) {
        lights[0].enabled = !lights[0].enabled;
        cout << "[Luz 1] " << (lights[0].enabled ? "LIGADA" : "DESLIGADA") << "\n";
    }
    if (key == GLFW_KEY_2 && (int)lights.size() > 1) {
        lights[1].enabled = !lights[1].enabled;
        cout << "[Luz 2] " << (lights[1].enabled ? "LIGADA" : "DESLIGADA") << "\n";
    }
    if (key == GLFW_KEY_3 && (int)lights.size() > 2) {
        lights[2].enabled = !lights[2].enabled;
        cout << "[Luz 3] " << (lights[2].enabled ? "LIGADA" : "DESLIGADA") << "\n";
    }

    if (key == GLFW_KEY_TAB) {
        selectedObj = (selectedObj + 1) % (int)objects.size();
        cout << "[Selecionado] Objeto " << selectedObj << "\n";
        return;
    }

    SceneObject &obj = objects[selectedObj];
    const bool shift = (mods & GLFW_MOD_SHIFT) != 0;

    // Rotacao (X / Y / Z — Shift inverte sentido)
    if (key == GLFW_KEY_X) obj.rotation.x += shift ? -5.0f : 5.0f;
    if (key == GLFW_KEY_Y) obj.rotation.y += shift ? -5.0f : 5.0f;
    if (key == GLFW_KEY_Z) obj.rotation.z += shift ? -5.0f : 5.0f;

    // Translacao — setas para XZ, PageUp/Down para Y
    if (key == GLFW_KEY_LEFT)      obj.position.x -= 0.1f;
    if (key == GLFW_KEY_RIGHT)     obj.position.x += 0.1f;
    if (key == GLFW_KEY_UP)        obj.position.z -= 0.1f;
    if (key == GLFW_KEY_DOWN)      obj.position.z += 0.1f;
    if (key == GLFW_KEY_PAGE_UP)   obj.position.y += 0.1f;
    if (key == GLFW_KEY_PAGE_DOWN) obj.position.y -= 0.1f;

    // Escala uniforme
    if (key == GLFW_KEY_RIGHT_BRACKET) obj.scale += 0.05f;
    if (key == GLFW_KEY_LEFT_BRACKET)  { obj.scale -= 0.05f; if (obj.scale < 0.05f) obj.scale = 0.05f; }

    // T: liga/desliga animacao Bezier do objeto selecionado
    if (key == GLFW_KEY_T) {
        if (obj.hasAnim) {
            obj.animating = !obj.animating;
            cout << "[Animacao] Objeto " << selectedObj
                 << (obj.animating ? " LIGADA" : " DESLIGADA") << "\n";
        } else {
            cout << "[Animacao] Objeto " << selectedObj << " nao tem trajetoria definida.\n";
        }
    }

    // R: reset das transformacoes do objeto selecionado
    if (key == GLFW_KEY_R) {
        obj.rotation = vec3(0.0f);
        obj.scale    = 1.0f;
        cout << "[Reset] Rotacao e escala do objeto " << selectedObj << " resetadas.\n";
    }
}

void mouseCallback(GLFWwindow *w, double xpos, double ypos)
{
    if (glfwGetInputMode(w, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) return;
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    float dx =  (float)xpos - lastX;
    float dy =  lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;
    camera.look(dx, dy);
}

GLuint buildShader()
{
    GLint  ok; GLchar log[512];

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertSrc, NULL);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(vs, 512, NULL, log); cout << "VS:\n" << log << "\n"; }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragSrc, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(fs, 512, NULL, log); cout << "FS:\n" << log << "\n"; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(prog, 512, NULL, log); cout << "Link:\n" << log << "\n"; }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

GLuint loadOBJ(const string &path, int &nVerts,
               vec3 &ka, vec3 &kd, vec3 &ks, float &ns, string &texFile)
{
    // Defaults
    ka = vec3(0.2f); kd = vec3(1.0f); ks = vec3(0.5f); ns = 32.0f; texFile = "";

    string folder = path.substr(0, path.find_last_of("/\\") + 1);

    vector<vec3>  vPos, vNorm;
    vector<vec2>  vUV;
    vector<float> buf;

    ifstream fin(path);
    if (!fin.is_open()) { cerr << "Nao foi possivel abrir: " << path << "\n"; return 0; }

    string line;
    while (getline(fin, line))
    {
        istringstream ss(line);
        string tok; ss >> tok;

        if (tok == "mtllib")
        {
            string mtlFile; ss >> mtlFile;
            ifstream mtl(folder + mtlFile);
            string ml;
            while (getline(mtl, ml)) {
                istringstream ms(ml); string mw; ms >> mw;
                if      (mw == "map_Kd") ms >> texFile;
                else if (mw == "Ka")     ms >> ka.r >> ka.g >> ka.b;
                else if (mw == "Kd")     ms >> kd.r >> kd.g >> kd.b;
                else if (mw == "Ks")     ms >> ks.r >> ks.g >> ks.b;
                else if (mw == "Ns")     ms >> ns;
            }
        }
        else if (tok == "v")  { vec3 v; ss >> v.x >> v.y >> v.z; vPos.push_back(v); }
        else if (tok == "vn") { vec3 n; ss >> n.x >> n.y >> n.z; vNorm.push_back(n); }
        else if (tok == "vt") { vec2 t; ss >> t.x >> t.y;        vUV.push_back(t); }
        else if (tok == "f")
        {
            string w;
            while (ss >> w)
            {
                int vi = 0, ti = 0, ni = 0;
                istringstream ws(w); string idx;
                if (getline(ws, idx, '/')) vi = idx.empty() ? 0 : stoi(idx) - 1;
                if (getline(ws, idx, '/')) ti = idx.empty() ? 0 : stoi(idx) - 1;
                if (getline(ws, idx))      ni = idx.empty() ? 0 : stoi(idx) - 1;

                buf.push_back(vPos.empty()  ? 0.0f : vPos[vi].x);
                buf.push_back(vPos.empty()  ? 0.0f : vPos[vi].y);
                buf.push_back(vPos.empty()  ? 0.0f : vPos[vi].z);
                buf.push_back(vNorm.empty() ? 0.0f : vNorm[ni].x);
                buf.push_back(vNorm.empty() ? 1.0f : vNorm[ni].y);
                buf.push_back(vNorm.empty() ? 0.0f : vNorm[ni].z);
                buf.push_back(vUV.empty()   ? 0.0f : vUV[ti].x);
                buf.push_back(vUV.empty()   ? 0.0f : vUV[ti].y);
            }
        }
    }

    nVerts = (int)buf.size() / 8;

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, buf.size() * sizeof(float), buf.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    cout << "[OBJ] " << path << " — " << nVerts << " vertices\n";
    return VAO;
}

GLuint loadTexture(const string &path)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int w, h, ch;
    unsigned char *data = stbi_load(path.c_str(), &w, &h, &ch, 0);
    if (data) {
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        cout << "[TEX] " << path << " (" << w << "x" << h << ")\n";
    } else {
        cout << "[TEX] Falha ao carregar: " << path << " — usando branca\n";
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
        return whiteTexture();
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

GLuint whiteTexture()
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    unsigned char white[3] = {255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, white);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

bool loadScene(const string &path)
{
    ifstream fin(path);
    if (!fin.is_open()) { cerr << "Cena nao encontrada: " << path << "\n"; return false; }

    const string modelsDir = "../assets/Modelos3D/";

    string line;
    while (getline(fin, line))
    {
        if (line.empty() || line[0] == '#') continue;
        istringstream ss(line);
        string cmd; ss >> cmd;

        // ── CAMERA ──────────────────────────────────────────────────────────
        if (cmd == "CAMERA")
        {
            float px, py, pz, yaw, pitch, fov, near, far;
            ss >> px >> py >> pz >> yaw >> pitch >> fov >> near >> far;
            camera.pos   = vec3(px, py, pz);
            camera.yaw   = yaw;
            camera.pitch = pitch;
            camera.look(0.0f, 0.0f);
            camFOV  = fov;
            camNear = near;
            camFar  = far;
            cout << "[CAMERA] pos(" << px << "," << py << "," << pz
                 << ") yaw=" << yaw << " pitch=" << pitch
                 << " fov=" << fov << "\n";
        }

        // ── LIGHT ───────────────────────────────────────────────────────────
        else if (cmd == "LIGHT")
        {
            Light l;
            ss >> l.position.x >> l.position.y >> l.position.z
               >> l.color.r    >> l.color.g    >> l.color.b
               >> l.intensity;
            lights.push_back(l);
            cout << "[LIGHT] pos(" << l.position.x << "," << l.position.y << ","
                 << l.position.z << ") int=" << l.intensity << "\n";
        }

        // ── OBJECT ──────────────────────────────────────────────────────────
        else if (cmd == "OBJECT")
        {
            SceneObject obj;
            string filename;
            ss >> filename
               >> obj.position.x >> obj.position.y >> obj.position.z
               >> obj.rotation.x >> obj.rotation.y >> obj.rotation.z
               >> obj.scale;

            string texFile;
            obj.VAO = loadOBJ(modelsDir + filename, obj.nVerts,
                               obj.ka, obj.kd, obj.ks, obj.ns, texFile);
            if (obj.VAO == 0) continue;

            obj.texID = texFile.empty()
                ? whiteTexture()
                : loadTexture(modelsDir + texFile);

            string extra;
            if (ss >> extra && extra == "BEZIER") {
                float bx, by, bz;
                while (ss >> bx >> by >> bz)
                    obj.bezierPts.push_back(vec3(bx, by, bz));
                obj.hasAnim = obj.bezierPts.size() >= 2;
                if (obj.hasAnim)
                    cout << "[BEZIER] Objeto com " << obj.bezierPts.size()
                         << " pontos de controle. Pressione P para animar.\n";
            }

            objects.push_back(obj);
        }
    }

    cout << "[CENA] " << objects.size() << " objeto(s), "
         << lights.size() << " luz(es) carregados.\n";
    return !objects.empty();
}

void printHelp()
{
    cout << "\n=== Integracao GB — Controles ===\n"
         << "  Clique Esq      : capturar mouse\n"
         << "  Clique Dir      : liberar mouse\n"
         << "  WASD / Q / E    : mover camara\n"
         << "  Mouse           : orientar camara\n"
         << "  TAB             : selecionar proximo objeto\n"
         << "  X / Y / Z       : rotacionar objeto (+Shift inverte)\n"
         << "  Setas           : transladar objeto (XZ)\n"
         << "  PageUp/Down     : transladar objeto (Y)\n"
         << "  [ / ]           : escalar objeto\n"
         << "  T               : ligar/desligar animacao Bezier\n"
         << "  R               : resetar rotacao e escala\n"
         << "  1 / 2 / 3       : ligar/desligar luzes\n"
         << "  ESC             : sair\n"
         << "=================================\n\n";
}
