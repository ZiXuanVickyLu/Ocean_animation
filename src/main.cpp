#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "render/shader.h"
#include "render/camera.h"

#include <ctime>
#include <iostream>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "point.h"
#include "plane.h"


#define XFIELD 50
#define ZFIELD 50

#define QUADSIZE 0.4
#define VTXSIZE (0.05f)
#define TEXDIVIDER 40
#define SPEED (0.008f)
#define WAVESIZE (3.0f)

float speed=250;

int octaves=5;
float elapsed;
float timer;
time_t start ,end;
float func(float x,float z)
{
    float y=4.5;

    float factor=1.0f;
    float d=sqrt(x*x+z*z);
    d=d/40.0f;
    if (d>1.5) d=1.5f;
    if (d<0) d=0;
    for (int i=0;i<octaves;i++)
    {
        y-=	factor*VTXSIZE*d*cosf((timer*SPEED)+(1/factor)*x*z*WAVESIZE)+
               (factor)*VTXSIZE*d*sinf((timer*SPEED)+(1/factor)*x*z*WAVESIZE) ;
        factor=factor/2.0f;
    }
    return y;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);
void renderQuad();
void computer_sea_caustics();
void computer_sea();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.3f, 0.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "ocean base", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();


    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader shader_base("../src/shader/mapping.vs", "../src/shader/mapping.fs");
    Shader screenShader("../src/shader/framebuffers_screen.vs", "../src/shader/framebuffers_screen.fs");
    Shader seaShader("../src/shader/waves.vs", "../src/shader/waves.fs");
    Shader cauticsShader("../src/shader/waves.vs", "../src/shader/waves.fs");

    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
    };

    // load textures
    // -------------
    unsigned int diffuseMap = loadTexture((GLchar*)("../reference/textures/sandy.png"));
    unsigned int normalMap  = loadTexture((GLchar*)("../reference/textures/sandy_n.png"));
    unsigned int heightMap = loadTexture((GLchar*)("../reference/textures/sandy_d.png"));


    unsigned int enviorMap = loadTexture((GLchar*)("../reference/textures/sky.tga"));
    unsigned int causticsMap = loadTexture((GLchar*)("../reference/textures/light.png"));
    // shader configuration
    // -------------------
    shader_base.use();
    shader_base.setInt("diffuseMap", 0);
    shader_base.setInt("normalMap", 1);
    shader_base.setInt("depthMap", 2);

    screenShader.use();
    screenShader.setInt("screenTexture", 0);

    seaShader.use();
    seaShader.setInt("texture1",0);

    cauticsShader.use();
    cauticsShader.setInt("texture1",0);

    // screen quad VAO
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));


    // framebuffer configuration
    // -------------------------
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // create a color attachment texture
    unsigned int textureColorbuffer;
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

    // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // lighting info
    // -------------
    glm::vec3 lightPos(0.0f, 3.0f, 0.0f);

    // render loop
    start = clock();
    // -----------
    while (!glfwWindowShouldClose(window))
    {

        //imgui
        // feed inputs to dear imgui, start new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        std::string s = "Ocean base ";
        s += std::to_string(1/deltaTime);
        s += " fps  ";
        glfwSetWindowTitle(window,(char *)s.c_str());
        ImGui::Begin("Rendering Speed ");

        ImGui::Text("%s", s.c_str());
        ImGui::End();

        // input
        // -----
        processInput(window);

        // render
        // -----------------------------------------------------------------------------------------------

        // bind to framebuffer and draw scene as we normally would to color texture
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // configure view/projection matrices
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        //first render pass -- render the ocean base
        shader_base.use();
        shader_base.setMat4("projection", projection);
        shader_base.setMat4("view", view);

        // render normal-paradox-mapped quad
        glm::mat4 model = glm::mat4(1.0f);

        shader_base.setMat4("model", model);
        shader_base.setVec3("viewPos", camera.Position);
        shader_base.setVec3("lightPos", lightPos);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, heightMap);
        renderQuad();

        //second render pass: render caustics of light
        cauticsShader.use();
        cauticsShader.setMat4("projection", projection);
        cauticsShader.setMat4("view", view);
        cauticsShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, causticsMap);
        computer_sea_caustics();


        //third render pass: render over waves
        seaShader.use();
        seaShader.setMat4("projection", projection);
        seaShader.setMat4("view", view);
        seaShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, enviorMap);
        computer_sea();

        // now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
        // clear all relevant buffers
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
        glClear(GL_COLOR_BUFFER_BIT);

        screenShader.use();
        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);	// use the color attachment texture as the texture of the quad plane
        glDrawArrays(GL_TRIANGLES, 0, 6);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
unsigned int SeaVAO = 0;
unsigned int SeaVBO;

void computer_sea_caustics(){
   end = clock();
    timer=(float)(end - start)/CLOCKS_PER_SEC * 1000.0f;
// second pass: caustic on top of the floor as an additive blend
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
  if(SeaVAO == 0){
      glGenVertexArrays(1, &SeaVAO);
      glGenBuffers(1, &SeaVBO);
  }

    for (int xi=-XFIELD;xi<XFIELD;xi++)
    {
        int size_of_vertex = 0;
        std::vector<float> vertexBuffer;
        for (int zi=-XFIELD;zi<ZFIELD;zi++)
        {
            // compute caustic environment mapping for point 1 in the strip
            point p(xi*QUADSIZE,0,zi*QUADSIZE);p.y=func(xi,zi);
            point q((xi+1)*QUADSIZE,0,zi*QUADSIZE);q.y=func(xi+1,zi);
            point r((xi)*QUADSIZE,0,(zi+1)*QUADSIZE);r.y=func(xi,zi+1);

            point e1=q-p;
            point e2=r-p;
            point e3=e1^e2;	// e3 is the normal of the sea above the sampling point #1


            plane pl(20,-1,20,20);
            point res;
            pl.testline(p,e3,res);
            // compute the collision to the lightmap
            vertexBuffer.push_back(p.x);
            vertexBuffer.push_back(0.01f);
            vertexBuffer.push_back(p.z);
            vertexBuffer.push_back(res.x/TEXDIVIDER);
            vertexBuffer.push_back(res.z/TEXDIVIDER);
            size_of_vertex ++;

            // compute caustic environment mapping for point 2 in the strip (shift 1 in xi)
            p=q;
            q.create((xi+2)*QUADSIZE,0,zi*QUADSIZE);q.y=func(xi+2,zi);
            r.create((xi+1)*QUADSIZE,0,(zi+1)*QUADSIZE);r.y=func(xi+1,zi+1);

            e1=q-p;
            e2=r-p;
            e3=e1^e2;
            // e3 is the normal of the sea above the sampling point #2

            pl.testline(p,e3,res);
            // compute the collision to the lightmap
            vertexBuffer.push_back(p.x);
            vertexBuffer.push_back(0.01f);
            vertexBuffer.push_back(p.z);
            vertexBuffer.push_back(res.x/TEXDIVIDER);
            vertexBuffer.push_back(res.z/TEXDIVIDER);
            size_of_vertex ++;

        }
        glBindVertexArray(SeaVAO);
        glBindBuffer(GL_ARRAY_BUFFER, SeaVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 5 * size_of_vertex, (vertexBuffer.data()), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        //draw:
        glBindVertexArray(SeaVAO);
        glDrawArrays(GL_TRIANGLE_STRIP,0,size_of_vertex);
        glBindVertexArray(0);
    }


}

// renders a 1x1 quad in NDC with manually calculated tangent vectors
// ------------------------------------------------------------------
unsigned int planeVAO = 0;
unsigned int planeVBO;
void renderQuad()
{
    if (planeVAO == 0)
    {
        // positions
        glm::vec3 pos1(-5.0f,  0.0f, 5.0f);
        glm::vec3 pos2(-5.0f,  0.0f, -5.0f);
        glm::vec3 pos3( 5.0f,  0.0f, -5.0f);
        glm::vec3 pos4( 5.0f,  0.0f, 5.0f);
        // texture coordinates
        glm::vec2 uv1(0.0f, 1.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);
        glm::vec2 uv4(1.0f, 1.0f);
        // normal vector
        glm::vec3 nm(0.0f, 1.0f, 0.0f);

        // calculate tangent/bitangent vectors of both triangles
        glm::vec3 tangent1, bitangent1;
        glm::vec3 tangent2, bitangent2;
        // triangle 1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);


        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);


        float quadVertices_[] = {
                // positions            // normal         // texcoords  // tangent                          // bitangent
                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        // configure plane VAO
        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices_), &quadVertices_, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }
    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
unsigned int waveVAO = 0;
unsigned int waveVBO;

void computer_sea() {
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    if (waveVAO == 0) {
        glGenVertexArrays(1, &waveVAO);
        glGenBuffers(1, &waveVBO);
    }

    for (int xi =-XFIELD; xi < XFIELD; xi++) {
        int size_of_vertex = 0;
        std::vector<float> vertexBuffer;
        for (int zi = -XFIELD; zi < ZFIELD; zi++) {
            // compute caustic environment mapping for point 1 in the strip
            point p(xi * QUADSIZE, 0, zi * QUADSIZE);
            p.y = func(xi, zi);
            point q((xi + 1) * QUADSIZE, 0, zi * QUADSIZE);
            q.y = func(xi + 1, zi);
            point r((xi) * QUADSIZE, 0, (zi + 1) * QUADSIZE);
            r.y = func(xi, zi + 1);

            point e1 = q - p;
            point e2 = r - p;
            point e3 = e1 ^ e2;    // e3 is the normal of the sea above the sampling point #1


            plane pl(0, -1, 0, 12);
            point res;
            pl.testline(p, e3, res);
            // compute the collision to the lightmap
            vertexBuffer.push_back(p.x);
            vertexBuffer.push_back(p.y);
            vertexBuffer.push_back(p.z);
            vertexBuffer.push_back(res.x / TEXDIVIDER);
            vertexBuffer.push_back(res.z / TEXDIVIDER);
            size_of_vertex++;

            // compute caustic environment mapping for point 2 in the strip (shift 1 in xi)
            p = q;
            q.create((xi + 2) * QUADSIZE, 0, zi * QUADSIZE);
            q.y = func(xi + 2, zi);
            r.create((xi + 1) * QUADSIZE, 0, (zi + 1) * QUADSIZE);
            r.y = func(xi + 1, zi + 1);

            e1 = q - p;
            e2 = r - p;
            e3 = e1 ^ e2;
            // e3 is the normal of the sea above the sampling point #2

            pl.testline(p, e3, res);
            // compute the collision to the lightmap
            vertexBuffer.push_back(p.x);
            vertexBuffer.push_back(p.y);
            vertexBuffer.push_back(p.z);
            vertexBuffer.push_back(res.x / TEXDIVIDER);
            vertexBuffer.push_back(res.z / TEXDIVIDER);
            size_of_vertex++;

        }
        glBindVertexArray(waveVAO);
        glBindBuffer(GL_ARRAY_BUFFER, waveVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 5 * size_of_vertex, (vertexBuffer.data()), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        //draw:
        glBindVertexArray(waveVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, size_of_vertex);
        glBindVertexArray(0);
    }
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
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

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
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
