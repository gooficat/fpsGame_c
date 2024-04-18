#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <cglm/cam.h>

#define WIDTH 800
#define HEIGHT 600

static float lastMouseX;
static float lastMouseY;
GLFWwindow* window;

GLuint compile_shader(const char *source, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check compilation status
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("Shader compilation failed: %s\n", infoLog);
    }

    return shader;
}

GLuint create_shaderprogram(const char *vertexSource, const char *fragmentSource) {
    // Compile vertex shader
    GLuint vertexShader = compile_shader(vertexSource, GL_VERTEX_SHADER);
    // Compile fragment shader
    GLuint fragmentShader = compile_shader(fragmentSource, GL_FRAGMENT_SHADER);

    // Create shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check linking status
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("Shader program linking failed: %s\n", infoLog);
    }

    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

typedef struct {
    vec3 position;
    vec3 front;
    vec3 up;
    float yaw;
    float pitch;
    float fov;
} camera3D;

void use_camera(GLuint shaderProgram, camera3D *cam) {
    mat4 view;
    glm_lookat(cam->position, cam->position + cam->front, cam->up, view);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);

    mat4 projection;
    glm_perspective(glm_rad(cam->fov), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f, projection);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
}

typedef struct {
    GLuint* vertices;
    size_t vertexCount;
    GLfloat* indices;
    size_t indexCount;
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    vec3 position;
    vec3 rotation;
} mesh3D;

void generate_mesh(mesh3D *mesh) {
    // Generate VAO, VBO, and EBO
    glGenVertexArrays(1, &(mesh->VAO));
    glGenBuffers(1, &(mesh->VBO));
    glGenBuffers(1, &(mesh->EBO));

    // Bind VAO
    glBindVertexArray(mesh->VAO);

    // Bind VBO and EBO and send data
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertexCount * sizeof(GLfloat), mesh->vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indexCount * sizeof(GLuint), mesh->indices, GL_STATIC_DRAW);

    // Set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind VAO
    glBindVertexArray(0);
}

void draw_mesh(mesh3D *mesh) {
    // Bind VAO and draw
    glBindVertexArray(mesh->VAO);
    glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}


camera3D playerCam = {
    .position = {0.0f, 0.0f, 5.0f},
    .front = {0.0f, 0.0f, -1.0f},
    .up = {0.0f, 1.0f, 0.0f},
    .yaw = -90.0f,
    .pitch = 0.0f,
    .fov = 75.0f
};

float sensitivity = 0.05f;
void handle_camlook(camera3D *cam, double mouseX, double mouseY) {

    float xOffset = mouseX - lastMouseX;
    float yOffset = mouseY - lastMouseY; // Reversed since y-coordinates range from bottom to top

    xOffset *= sensitivity;
    yOffset *= sensitivity;

    cam->yaw += xOffset;
    cam->pitch += yOffset;

    if (cam->pitch > 89.0f) cam->pitch = 89.9f;
    if (cam->pitch < -89.0f) cam->pitch = -89.9f;

    vec3 front;
    front[0] = cos(glm_rad(cam->yaw)) * cos(glm_rad(cam->pitch));
    front[1] = sin(glm_rad(cam->pitch));
    front[2] = sin(glm_rad(cam->yaw)) * cos(glm_rad(cam->pitch));
    glm_normalize_to(front, cam->front);
}

float cameraSpeed = 0.05f;
void handle_cammovement(GLFWwindow *window, camera3D *cam) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        glm_vec3_add(cam->position, (vec3){0.0f, 0.0f, -cameraSpeed}, cam->position);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        glm_vec3_add(cam->position, (vec3){0.0f, 0.0f, cameraSpeed}, cam->position);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        glm_vec3_add(cam->position, (vec3){-cameraSpeed, 0.0f, 0.0f}, cam->position);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        glm_vec3_add(cam->position, (vec3){cameraSpeed, 0.0f, 0.0f}, cam->position);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    handle_camlook(&playerCam, xpos, ypos);

    lastMouseX = xpos;
    lastMouseY = ypos;
}

int main(void)
{
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glewInit();
    glViewport(0, 0, WIDTH, HEIGHT);

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        handle_cammovement(window, &playerCam);
	use_camera(shaderProgram, &playerCam);
	

        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}
