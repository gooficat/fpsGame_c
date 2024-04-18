#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <cglm/cam.h>

#define WIDTH 800
#define HEIGHT 600

static float lastMouseX;
static float lastMouseY;
GLFWwindow* window;


typedef struct {
    vec3 position;
    vec3 front;
    vec3 up;
    float yaw;
    float pitch;
    float fov;
} camera3D;

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

    if (cam->pitch > 89.0f) cam->pitch = 89.0f;
    if (cam->pitch < -89.0f) cam->pitch = -89.0f;

    vec3 front;
    front[0] = cos(glm_rad(cam->yaw)) * cos(glm_rad(cam->pitch));
    front[1] = sin(glm_rad(cam->pitch));
    front[2] = sin(glm_rad(cam->yaw)) * cos(glm_rad(cam->pitch));
    glm_normalize_to(front, cam->front);
}

float cameraSpeed = 0.05f;
void handle_cammove(GLFWwindow *window, camera3D *cam) {
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

int main()
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
        handle_cammove(window, &playerCam);

        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}