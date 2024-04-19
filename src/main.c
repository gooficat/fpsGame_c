#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <cglm/cam.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WIDTH 960
#define HEIGHT 540

#define GRAVITY 9.81f

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
        char infoLog[128];
        glGetShaderInfoLog(shader, 128, NULL, infoLog);
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
        char infoLog[128];
        glGetProgramInfoLog(shaderProgram, 128, NULL, infoLog);
        printf("Shader program linking failed: %s\n", infoLog);
    }

    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

typedef struct {
    GLuint id;
    int width;
    int height;
    int nrChannels;
} texture;

void generate_texture(const char *texturePath, texture *tex) {
    glGenTextures(1, &(tex->id));
    glBindTexture(GL_TEXTURE_2D, tex->id);

    // Set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Load and generate the texture
    stbi_set_flip_vertically_on_load(true); // Tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *data = stbi_load(texturePath, &(tex->width), &(tex->height), &(tex->nrChannels), 0);
    if (data) {
        GLenum format;
        if (tex->nrChannels == 1)
            format = GL_RED;
        else if (tex->nrChannels == 3)
            format = GL_RGB;
        else if (tex->nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, tex->width, tex->height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        printf("Failed to load texture\n");
    }
    stbi_image_free(data);
}

void create_transform(vec3 translation, vec3 rotation, mat4 outTransform) {
    // Create translation matrix
    glm_translate_make(outTransform, translation);

    // Create rotation matrix
    mat4 rotationMatrix;
    glm_rotate_make(rotationMatrix, glm_rad(rotation[0]), (vec3){1.0f, 0.0f, 0.0f});
    glm_rotate(rotationMatrix, glm_rad(rotation[1]), (vec3){0.0f, 1.0f, 0.0f});
    glm_rotate(rotationMatrix, glm_rad(rotation[2]), (vec3){0.0f, 0.0f, 1.0f});

    // Multiply translation and rotation matrices to get the final transform
    glm_mat4_mul(outTransform, rotationMatrix, outTransform);
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
	glm_lookat(cam->position, (vec3){cam->position[0] + cam->front[0], cam->position[1] + cam->front[1], cam->position[2] + cam->front[2]}, cam->up, view);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);

    mat4 projection;
    glm_perspective(glm_rad(cam->fov), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f, projection);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
}

typedef struct {
    GLfloat* vertices;
    size_t vertexCount;
    GLuint* indices;
    size_t indexCount;
    texture *tex;
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    vec3 position;
    vec3 rotation;
    mat4 matrix;
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Unbind VAO
    glBindVertexArray(0);
}

void draw_mesh(GLuint shaderProgram, mesh3D *mesh) {
    mat4 transformMatrix;
    create_transform(mesh->position, mesh->rotation, transformMatrix);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &transformMatrix[0][0]);
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mesh->tex->id);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    // Bind VAO and draw
    glBindVertexArray(mesh->VAO);
    glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}


camera3D playerCam = {
    .position = {0.0f, 0.0f, 0.0f},
    .front = {0.0f, 0.0f, -1.0f},
    .up = {0.0f, 1.0f, 0.0f},
    .yaw = 90.0f,
    .pitch = 0.0f,
    .fov = 75.0f
};

float sensitivity = 0.15f;
void handle_camlook(camera3D *cam, double mouseX, double mouseY) {

    float xOffset = mouseX - lastMouseX;
    float yOffset = lastMouseY - mouseY; // Reversed since y-coordinates range from bottom to top

    xOffset *= sensitivity;
    yOffset *= sensitivity;

    cam->yaw += xOffset;
    cam->pitch += yOffset;

    if (cam->pitch > 89.9f) cam->pitch = 89.9f;
    if (cam->pitch < -89.9f) cam->pitch = -89.9f;

    vec3 front;
    front[0] = cos(glm_rad(cam->yaw)) * cos(glm_rad(cam->pitch));
    front[1] = sin(glm_rad(cam->pitch));
    front[2] = sin(glm_rad(cam->yaw)) * cos(glm_rad(cam->pitch));
    glm_normalize_to(front, cam->front);
}


float cameraSpeed = 0.01f;
void handle_cammovement(GLFWwindow *window, camera3D *cam) {
    // Calculate the camera's front vector
    vec3 front;
    front[0] = cos(glm_rad(cam->yaw)) * cos(glm_rad(cam->pitch));
    front[1] = 0;//sin(glm_rad(cam->pitch));
    front[2] = sin(glm_rad(cam->yaw)) * cos(glm_rad(cam->pitch));
    glm_normalize_to(front, front);

    // Calculate the camera's right vector
    vec3 right;
    glm_cross(cam->up, front, right);

    // Normalize vectors to ensure consistent speed in all directions
    glm_normalize(front);
    glm_normalize(right);

    // Calculate the movement direction based on user input
    vec3 movement = {0.0f, 0.0f, 0.0f};
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        glm_vec3_add(movement, front, movement); // Move forward
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        glm_vec3_sub(movement, front, movement); // Move backward
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        glm_vec3_add(movement, right, movement); // Move left
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        glm_vec3_sub(movement, right, movement); // Move right

    // Apply movement to camera position
    glm_vec3_scale(movement, cameraSpeed, movement);
    glm_vec3_add(cam->position, movement, cam->position);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    handle_camlook(&playerCam, xpos, ypos);

    lastMouseX = xpos;
    lastMouseY = ypos;
}

const char *vertexShaderSource = 
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aUV;\n"
    "out vec2 UV;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "UV = aUV;\n"
    "}\0";

const char *fragmentShaderSource =
    "#version 330 core\n"
    "in vec2 UV;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D texture1;\n"
    "void main()\n"
    "{\n"
    "   FragColor = texture(texture1, UV);\n"
    "}\n\0";

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

    GLuint shaderProgram = create_shaderprogram(vertexShaderSource, fragmentShaderSource);

    texture bricks;
    generate_texture("textures/bricks.png", &bricks);


    GLfloat groundVertices[] = {
        // Positions            // Texture coordinates
        -8.0f, -1.0f, -8.0f,    0.0f, 0.0f,
         8.0f, -1.0f, -8.0f,    16.0f, 0.0f,
         8.0f, -1.0f,  8.0f,    16.0f, 16.0f,
        -8.0f, -1.0f,  8.0f,    0.0f, 16.0f
    };

    GLuint groundIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    mesh3D groundMesh;

    groundMesh.vertices = groundVertices;
    groundMesh.indices = groundIndices;
    groundMesh.vertexCount = sizeof(groundVertices) / sizeof(groundVertices[0]);	
    groundMesh.indexCount =  sizeof(groundIndices) / sizeof(groundIndices[0]);
    groundMesh.tex = &bricks;

    groundMesh.position[0] = 0.0f;
    groundMesh.position[1] = 0.0f;
    groundMesh.position[2] = 0.0f;

    groundMesh.rotation[0] = 0.0f;
    groundMesh.rotation[1] = -1.5f;
    groundMesh.rotation[2] = 0.0f;

    generate_mesh(&groundMesh);
   

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        handle_cammovement(window, &playerCam);
        

	glUseProgram(shaderProgram);
	use_camera(shaderProgram, &playerCam);
        draw_mesh(shaderProgram, &groundMesh);
        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}
