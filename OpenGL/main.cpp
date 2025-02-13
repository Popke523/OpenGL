#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_t.h>
#include <learnopengl/mesh.h>
#include <learnopengl/model.h>
#include <learnopengl/camera.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "light.h"
#include "car.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);

// settings
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

const glm::vec3 day_sky_color = glm::vec3(135.0f, 206.0f, 235.0f) / 255.0f; // html skyblue
const glm::vec3 night_sky_color = glm::vec3(0.1f, 0.1f, 0.1f);

enum time_of_day
{
    DAY,
    NIGHT
} current_time_of_day;

Car car(glm::vec3(16.0f, 0.0f, -21.0f));

Camera mainCamera(glm::vec3(0.0f, 0.0f, -21.0f));
Camera carCamera(car.position);
Camera staticCamera = Camera(glm::vec3(17.5f, 3.5f, -22.5f), glm::vec3(0.0f, 1.0f, 0.0f), 150.0f, -45.0f);
Camera* activeCamera = &carCamera;

glm::vec3 fogColor(0.5f, 0.5f, 0.5f);
float fogIntensity = 1.0f;

struct bezierSurfaceVertex
{
	glm::vec3 position;
	glm::vec2 texCoords;
};

bool blinn = false;
bool debug_window = true;

float frametime = 0.0f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------

    auto monitor = glfwGetPrimaryMonitor();

    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    SCR_WIDTH = mode->width;
    SCR_HEIGHT = mode->height;

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", monitor, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    // Set up ImGui backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // Set up ImGui style
    ImGui::StyleColorsDark();

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile our shader zprogram
    // ------------------------------------
    Shader lightingShader("6.multiple_lights.vs", "6.multiple_lights.fs");
    Shader lightCubeShader("6.light_cube.vs", "6.light_cube.fs");

    dir_light daylight;
    daylight.direction = glm::vec3(-1.0f, -1.0f, -1.0f);
    daylight.ambient = glm::vec3(0.5f, 0.4f, 0.3f);
    daylight.diffuse = glm::vec3(0.5f, 0.4f, 0.3f);
    daylight.specular = glm::vec3(1.0f, 0.8f, 0.6f);

    dir_light night_light;
    night_light.direction = glm::vec3(-1.0f, -1.0f, -1.0f);
	night_light.ambient = glm::vec3(0.03f, 0.04f, 0.05f);
	night_light.diffuse = glm::vec3(0.03f, 0.04f, 0.05f);
	night_light.specular = glm::vec3(0.06f, 0.08f, 0.1f);

    dir_light active_light;

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    const float cube_vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };
    // positions all containers
    glm::vec3 cubePositions[] = {
        glm::vec3(15.0f, 3.0f, -19.0f),
        glm::vec3(12.0f,  5.0f, -8.0f),
        glm::vec3(9.0f, -2.2f, -6.0f),
        glm::vec3(2.0f, 0.0f, -24.0f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f,  2.0f, -2.5f),
        glm::vec3(1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    // positions of the point lights
    glm::vec3 pointLightPositions[] = {
        glm::vec3(16.0f, 3.0f, -19.0f),
        glm::vec3(5.0f, 0.5f, -25.0f)
    };
    // first, configure the cube's VAO (and VBO)
    unsigned int VBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // note that we update the lamp's position attribute's stride to reflect the updated buffer data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // load textures (we now use a utility function to keep the code more organized)
    // -----------------------------------------------------------------------------
    unsigned int diffuseMap = loadTexture("container2.png");
    unsigned int specularMap = loadTexture("container2_specular.png");

    // shader configuration
    // --------------------
    lightingShader.use();
    lightingShader.setInt("texture_diffuse1", 0);
    lightingShader.setInt("texture_specular1", 1);


    // build and compile shaders
    // -------------------------
    Shader ourShader("1.model_loading.vs", "1.model_loading.fs");
	Shader carShader("car_shader.vs", "car_shader.fs");
	Shader bezierSurfaceShader("bezier_surface.vs", "bezier_surface.fs", nullptr, "bezier_surface.tcs", "bezier_surface.tes");

    // load models
    // -----------
    Model bmw_g82_m4_model("resources/FINAL_MODEL_M22/FINAL_MODEL_M22.fbx");
    Model de_dust2_model("resources/de_dust2/de_dust2.obj");

    glm::mat4 dust2_model_matrix(1.0f);
    dust2_model_matrix = glm::scale(dust2_model_matrix, glm::vec3(0.01f));
    dust2_model_matrix = glm::rotate(dust2_model_matrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    const int rez = 4;
    bezierSurfaceVertex bezierSurfaceVertices[rez][rez];

    glm::mat4 flagMatrix(1.0f);
	flagMatrix = glm::translate(flagMatrix, glm::vec3(10.5f, 2.0f, -23.5f));
	flagMatrix = glm::scale(flagMatrix, glm::vec3(0.4f));
	flagMatrix = glm::rotate(flagMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	unsigned int bezierSurfaceVBO, bezierSurfaceVAO;
	glGenVertexArrays(1, &bezierSurfaceVAO);
    glBindVertexArray(bezierSurfaceVAO);
	
    glGenBuffers(1, &bezierSurfaceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, bezierSurfaceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(bezierSurfaceVertices), &bezierSurfaceVertices[0], GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

    const unsigned int NUM_PATCH_PTS = 16;

    glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        frametime = deltaTime;

        // input
        // -----
        processInput(window);

        // render
        // ------
        switch (current_time_of_day)
        {
        case DAY:
            glClearColor(day_sky_color.x, day_sky_color.y, day_sky_color.z, 1.0f);
			active_light = daylight;
            fogColor = glm::vec3(0.5f);
            break;
        case NIGHT:
			glClearColor(night_sky_color.x, night_sky_color.y, night_sky_color.z, 1.0f);
            active_light = night_light;
            fogColor = glm::vec3(0.1f);
            break;
        }
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        double time = glfwGetTime();

        for (int i = 0; i < rez; i++)
        {
            for (int j = 0; j < rez; j++)
            {
				bezierSurfaceVertices[i][j].position.x = float(i);
				bezierSurfaceVertices[i][j].texCoords = glm::vec2(i / float(rez - 1), j / float(rez - 1));
            }
        }
        for (int j = 0; j < rez; j++)
        {
			bezierSurfaceVertices[0][j].position.y = j * 0.5f;
            bezierSurfaceVertices[1][j].position.y = j * 0.5f + float(cos(time));
            bezierSurfaceVertices[2][j].position.y = j * 0.5f + float(sin(time));
            bezierSurfaceVertices[3][j].position.y = j * 0.5f;
        }
        for (int i = 0; i < rez; i++)
        {
            bezierSurfaceVertices[i][0].position.z = 0.0f;
            bezierSurfaceVertices[i][1].position.z = float(cos(time));
            bezierSurfaceVertices[i][2].position.z = float(sin(time));
			bezierSurfaceVertices[i][3].position.z = 0.0f;
        }

        glBindVertexArray(bezierSurfaceVAO);
        glBindBuffer(GL_ARRAY_BUFFER, bezierSurfaceVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bezierSurfaceVertices), &bezierSurfaceVertices[0], GL_DYNAMIC_DRAW);
        glBindVertexArray(0);

        glm::mat4 bmw_model_matrix(1.0f);
        bmw_model_matrix = glm::translate(bmw_model_matrix, car.position);
        bmw_model_matrix = glm::rotate(bmw_model_matrix, car.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        bmw_model_matrix = glm::scale(bmw_model_matrix, glm::vec3(0.5f));

		glm::vec4 baseCarCameraPosition = glm::vec4(0.0f, 2.0f, -4.0f, 1.0f);
		carCamera.Position = glm::vec3(bmw_model_matrix * baseCarCameraPosition);
        carCamera.SetYaw(-glm::degrees(car.yaw - glm::radians(90.0f)));

        glm::vec4 baseCarSpotlight1 = glm::vec4(0.8f, 0.8f, 1.0f, 1.0f);
        glm::vec4 baseCarSpotlight2 = glm::vec4(-0.8f, 0.8f, 1.0f, 1.0f);
		//baseCarSpotlight1 = bmw_model_matrix * baseCarSpotlight1;
		//baseCarSpotlight2 = bmw_model_matrix * baseCarSpotlight2;

        spotlight spotlights[2];
        spotlights[0].position = bmw_model_matrix * baseCarSpotlight1;
		spotlights[1].position = bmw_model_matrix * baseCarSpotlight2;
        glm::mat4 spotlightDirectionMatrix(1.0f);

		spotlightDirectionMatrix = glm::rotate(spotlightDirectionMatrix, car.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        spotlightDirectionMatrix = glm::rotate(spotlightDirectionMatrix, car.spotlightPitch, glm::vec3(1.0f, 0.0f, 0.0f));

        for (int i = 0; i < 2; i++)
        {
			spotlights[i].direction = glm::vec3(spotlightDirectionMatrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
			spotlights[i].ambient = glm::vec3(0.0f, 0.0f, 0.0f);
			spotlights[i].diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
			spotlights[i].specular = glm::vec3(1.0f, 1.0f, 1.0f);
			spotlights[i].constant = 1.0f;
			spotlights[i].linear = 0.09f;
			spotlights[i].quadratic = 0.032f;
			spotlights[i].cutOff = glm::cos(glm::radians(12.5f));
			spotlights[i].outerCutOff = glm::cos(glm::radians(15.0f));
        }
 
        // be sure to activate shader when setting uniforms/drawing objects
        lightingShader.use();
        lightingShader.setVec3("viewPos", activeCamera->Position);
        lightingShader.setFloat("shininess", 32.0f);

        /*
           Here we set all the uniforms for the 5/6 types of lights we have. We have to set them manually and index
           the proper PointLight struct in the array to set each uniform variable. This can be done more code-friendly
           by defining light types as classes and set their values in there, or by using a more efficient uniform approach
           by using 'Uniform buffer objects', but that is something we'll discuss in the 'Advanced GLSL' tutorial.
        */
        // directional light
		active_light.apply(lightingShader);
        // point light 1
        lightingShader.setVec3("pointLights[0].position", pointLightPositions[0]);
        lightingShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
        lightingShader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
        lightingShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("pointLights[0].constant", 1.0f);
        lightingShader.setFloat("pointLights[0].linear", 0.09f);
        lightingShader.setFloat("pointLights[0].quadratic", 0.032f);
        // point light 2
        lightingShader.setVec3("pointLights[1].position", pointLightPositions[1]);
        lightingShader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
        lightingShader.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
        lightingShader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("pointLights[1].constant", 1.0f);
        lightingShader.setFloat("pointLights[1].linear", 0.09f);
        lightingShader.setFloat("pointLights[1].quadratic", 0.032f);

        // spotLight
        for (int i = 0; i < 2; i++)
        {
			spotlights[i].apply(lightingShader, i);
        }

        lightingShader.setBool("blinn", blinn);

        lightingShader.setFloat("fogIntensity", fogIntensity);
        lightingShader.setVec3("fogColor", fogColor);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(activeCamera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 100.0f);
        glm::mat4 view = activeCamera->GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        lightingShader.setMat4("model", model);

        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        // render containers
        glBindVertexArray(cubeVAO);
        for (unsigned int i = 0; i < 10; i++)
        {
            // calculate the model matrix for each object and pass it to shader before drawing
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            lightingShader.setMat4("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // also draw the lamp object(s)
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);

        // we now draw as many light bulbs as we have point lights.
        glBindVertexArray(lightCubeVAO);
        for (unsigned int i = 0; i < 4; i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pointLightPositions[i]);
            model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
            lightCubeShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glBindVertexArray(0);

        ourShader.use();
        ourShader.setVec3("viewPos", activeCamera->Position);
        ourShader.setFloat("shininess", 32.0f);

        /*
   Here we set all the uniforms for the 5/6 types of lights we have. We have to set them manually and index
   the proper PointLight struct in the array to set each uniform variable. This can be done more code-friendly
   by defining light types as classes and set their values in there, or by using a more efficient uniform approach
   by using 'Uniform buffer objects', but that is something we'll discuss in the 'Advanced GLSL' tutorial.
*/
// directional light
        active_light.apply(ourShader);
        // point light 1
        ourShader.setVec3("pointLights[0].position", pointLightPositions[0]);
        ourShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
        ourShader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
        ourShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("pointLights[0].constant", 1.0f);
        ourShader.setFloat("pointLights[0].linear", 0.09f);
        ourShader.setFloat("pointLights[0].quadratic", 0.032f);
        // point light 2
        ourShader.setVec3("pointLights[1].position", pointLightPositions[1]);
        ourShader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
        ourShader.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
        ourShader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("pointLights[1].constant", 1.0f);
        ourShader.setFloat("pointLights[1].linear", 0.09f);
        ourShader.setFloat("pointLights[1].quadratic", 0.032f);

        // spotLight
        for (int i = 0; i < 2; i++)
        {
            spotlights[i].apply(ourShader, i);
        }

        ourShader.setBool("blinn", blinn);

		ourShader.setFloat("fogIntensity", fogIntensity);
        ourShader.setVec3("fogColor", fogColor);

		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);
		ourShader.setMat4("model", dust2_model_matrix);
        de_dust2_model.Draw(ourShader);

        carShader.use();
        carShader.setVec3("viewPos", activeCamera->Position);
        carShader.setFloat("shininess", 32.0f);

        /*
   Here we set all the uniforms for the 5/6 types of lights we have. We have to set them manually and index
   the proper PointLight struct in the array to set each uniform variable. This can be done more code-friendly
   by defining light types as classes and set their values in there, or by using a more efficient uniform approach
   by using 'Uniform buffer objects', but that is something we'll discuss in the 'Advanced GLSL' tutorial.
*/
// directional light
        active_light.apply(carShader);
        // point light 1
        carShader.setVec3("pointLights[0].position", pointLightPositions[0]);
        carShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
        carShader.setVec3("pointLights[0].diffuse", 0.5f, 0.5f, 0.5f);
        carShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
        carShader.setFloat("pointLights[0].constant", 1.0f);
        carShader.setFloat("pointLights[0].linear", 0.09f);
        carShader.setFloat("pointLights[0].quadratic", 0.032f);
        // point light 2
        carShader.setVec3("pointLights[1].position", pointLightPositions[1]);
        carShader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
        carShader.setVec3("pointLights[1].diffuse", 0.5f, 0.5f, 0.5f);
        carShader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
        carShader.setFloat("pointLights[1].constant", 1.0f);
        carShader.setFloat("pointLights[1].linear", 0.09f);
        carShader.setFloat("pointLights[1].quadratic", 0.032f);

        // spotLight
        for (int i = 0; i < 2; i++)
        {
            spotlights[i].apply(carShader, i);
        }

        carShader.setBool("blinn", blinn);

		carShader.setFloat("fogIntensity", fogIntensity);
		carShader.setVec3("fogColor", fogColor);

        carShader.setMat4("projection", projection);
        carShader.setMat4("view", view);
		carShader.setMat4("model", bmw_model_matrix);
        bmw_g82_m4_model.Draw(carShader);
        
        glBindVertexArray(bezierSurfaceVAO);

		// render bezier surface
        bezierSurfaceShader.use();
        bezierSurfaceShader.setMat4("projection", projection);
        bezierSurfaceShader.setMat4("view", view);
		bezierSurfaceShader.setMat4("model", flagMatrix);
		bezierSurfaceShader.setInt("uDegree", rez - 1);
		bezierSurfaceShader.setInt("vDegree", rez - 1);

        bezierSurfaceShader.setVec3("viewPos", activeCamera->Position);
        bezierSurfaceShader.setFloat("shininess", 32.0f);

        // directional light
        active_light.apply(bezierSurfaceShader);
        // point light 1
        bezierSurfaceShader.setVec3("pointLights[0].position", pointLightPositions[0]);
        bezierSurfaceShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
        bezierSurfaceShader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
        bezierSurfaceShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
        bezierSurfaceShader.setFloat("pointLights[0].constant", 1.0f);
        bezierSurfaceShader.setFloat("pointLights[0].linear", 0.09f);
        bezierSurfaceShader.setFloat("pointLights[0].quadratic", 0.032f);
        // point light 2
        bezierSurfaceShader.setVec3("pointLights[1].position", pointLightPositions[1]);
        bezierSurfaceShader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
        bezierSurfaceShader.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
        bezierSurfaceShader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
        bezierSurfaceShader.setFloat("pointLights[1].constant", 1.0f);
        bezierSurfaceShader.setFloat("pointLights[1].linear", 0.09f);
        bezierSurfaceShader.setFloat("pointLights[1].quadratic", 0.032f);

        bezierSurfaceShader.setVec3("material_diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
        bezierSurfaceShader.setVec3("material_specular", glm::vec3(0.5f, 0.5f, 0.5f));

        // spotLight
        for (int i = 0; i < 2; i++)
        {
            spotlights[i].apply(bezierSurfaceShader, i);
        }

        bezierSurfaceShader.setBool("blinn", blinn);

        bezierSurfaceShader.setFloat("fogIntensity", fogIntensity);
        bezierSurfaceShader.setVec3("fogColor", fogColor);

		glDrawArrays(GL_PATCHES, 0, rez * rez);
        glBindVertexArray(0);

        if (debug_window)
        {
            // Start new ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Debug", NULL);
            ImGui::SetWindowSize(ImVec2(256, 160));
            ImGui::SetWindowPos(ImVec2(16, 16));
            ImGui::Text("%4.1f FPS", ImGui::GetIO().Framerate);
            ImGui::Text("Cam Pos: %7.2f %7.2f %7.2f", activeCamera->Position.x, activeCamera->Position.y, activeCamera->Position.z);
            ImGui::Text("Car Pos: %7.2f %7.2f %7.2f", car.position.x, car.position.y, car.position.z);
            ImGui::Text("Car Spotlight Pitch: %4.1f", glm::degrees(-car.spotlightPitch));
            ImGui::Text("Shading: %s", blinn ? "Blinn" : "Phong");
            ImGui::Text("Time: %s", current_time_of_day == DAY ? "Day" : "Night");
            ImGui::Text("Fog Intensity: %4.3f", fogIntensity);
            ImGui::End();

            // Render ImGui
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &VBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}



// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        mainCamera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        mainCamera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        mainCamera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        mainCamera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        current_time_of_day = NIGHT;
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        current_time_of_day = DAY;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        car.move(frametime * 2.0f);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        car.move(-frametime * 2.0f);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        car.rotate(frametime * 2.0f);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        car.rotate(-frametime * 2.0f);
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		car.position.y += frametime * 2.0f;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        car.position.y -= frametime * 2.0f;

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        activeCamera = &mainCamera;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        activeCamera = &carCamera;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        activeCamera = &staticCamera;

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		car.rotateSpotlight(-frametime * 0.2f);
    if (glfwGetKey(window, GLFW_KEY_SEMICOLON) == GLFW_PRESS)
        car.rotateSpotlight(frametime * 0.2f);

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        fogIntensity += frametime * 0.1f;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
    {
        fogIntensity -= frametime * 0.1f;
        if (fogIntensity < 0.0f) fogIntensity = 0.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
        blinn = true;
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
        blinn = false;

    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
        debug_window = true;
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
        debug_window = false;

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    activeCamera->ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    activeCamera->ProcessMouseScroll(static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}