#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <learnopengl/shader.h>

#include <iostream>
#include <vector>
#include <cstdlib>

// 콜백 및 입력 처리
void framebuffer_size_callback(GLFWwindow* w, int width, int height);
void mouse_callback(GLFWwindow* w, double xpos, double ypos);
void scroll_callback(GLFWwindow* w, double xoffset, double yoffset);
void processInput(GLFWwindow* w, float dt);
void updateCameraVectorsSafe();

// 화면 크기
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
// 그림자 맵
const unsigned int SHADOW_WIDTH = 1024;
const unsigned int SHADOW_HEIGHT = 1024;

// 시뮬레이션 시간 관련 전역 변수
float simulationTime = 0.0f;  // 누적 시뮬레이션 시간
float deltaTime = 0.0f;  // 프레임 간 시간 차
float lastFrame = 0.0f;  // 이전 프레임 시각
bool  paused = false;  // 일시정지 상태
float timeMultiplier = 1.0f;    // 시간 가속 배율

// 카메라
Camera camera(glm::vec3(0.0f, 300.0f, 0.0f));
float lastX = SCR_WIDTH / 2.0f, lastY = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;

// 그림자 맵용 FBO와 큐브맵 텍스처 핸들
unsigned int depthCubemapFBO, depthCubemap;
float near_plane = 1.0f, far_plane = 50000.0f;
std::vector<glm::mat4> shadowTransforms;

// 고리(링) 인스턴스용 VAO, VBO, EBO, 매트릭스 VBO
unsigned int ringVAO, ringVBO, ringEBO, ringMatrixVBO;

// 단위 AU와 행성 반지름 스케일 상수
const float AU_SCALE = 800.0f;     // 1AU를 800 유닛으로 변환
const float PLANET_RADIUS_SCALE = 0.00005f;   // km 단위를 유닛으로 변환

// 행성 데이터
struct PlanetData { float a, radius; };
static const PlanetData planetInfo[] = {
    {0.387f,2439.7f},  // 수성
    {0.723f,6051.8f},  // 금성
    {1.000f,6371.0f},  // 지구
    {1.524f,3389.5f},  // 화성
    {5.203f,69911.0f}, // 목성
    {9.537f,58232.0f}, // 토성
    {19.191f,25362.0f},// 천왕성
    {30.068f,24622.0f} // 해왕성
};

//현재 행성 위치저장 벡터
std::vector<glm::vec3> currentPlanetPositions(9);

// 링 위치 매트릭스 생성
std::vector<glm::mat4> generateRing(const glm::vec3& center, float innerR, float outerR, unsigned int count) {
    std::vector<glm::mat4> mats;
    mats.reserve(count);
    float mid = (innerR + outerR) * 10.5f;
    float off = (outerR - innerR) * 10.5f;
    for (unsigned i = 0; i < count; ++i) {
        float ang = glm::radians(360.0f * i / float(count));
        float r = mid + ((rand() % 100) / 100.0f * 2.0f - 1.0f) * off;
        glm::mat4 m(1.0f);
        m = glm::translate(m, center + glm::vec3(cos(ang) * r, 0.0f, sin(ang) * r));
        float s = 0.02f + (rand() % 100) / 100.0f * 0.02f;
        m = glm::scale(m, glm::vec3(s));
        m = glm::rotate(m, glm::radians(float(rand() % 360)), glm::vec3(0, 1, 0));
        mats.push_back(m);
    }
    return mats;
}

    // 행성, 달 렌더 함수
    void renderScene(
        Shader& shader, Model& mercury, Model& venus, Model& earth, Model& mars, Model& jupiter, Model& saturn, Model& uranus, Model& neptune, Model& moon) {
        struct Planet { Model& model; float orbitRadius, orbitSpeed, rotationSpeed, scale; };
        std::vector<Planet> planets = {
            // 각 행성에 대한 설정값 목록
            // orbitRadius = a * AU_SCALE
            // scale       = radius_km * PLANET_RADIUS_SCALE
            { mercury, planetInfo[0].a * AU_SCALE, 4.15f,    6.2f,   planetInfo[0].radius * PLANET_RADIUS_SCALE },
            { venus,   planetInfo[1].a * AU_SCALE, 1.62f,   -1.481f, planetInfo[1].radius * PLANET_RADIUS_SCALE },
            { earth,   planetInfo[2].a * AU_SCALE, 1.00f,   360.0f,  planetInfo[2].radius * PLANET_RADIUS_SCALE },
            { mars,    planetInfo[3].a * AU_SCALE, 0.53f,   370.8f,  planetInfo[3].radius * PLANET_RADIUS_SCALE },
            { jupiter, planetInfo[4].a * AU_SCALE, 0.08f,   881.6f,  planetInfo[4].radius * PLANET_RADIUS_SCALE },
            { saturn,  planetInfo[5].a * AU_SCALE, 0.03f,   822.9f,  planetInfo[5].radius * PLANET_RADIUS_SCALE },
            { uranus,  planetInfo[6].a * AU_SCALE, 0.0119f, -508.2f, planetInfo[6].radius * PLANET_RADIUS_SCALE },
            { neptune, planetInfo[7].a * AU_SCALE, 0.0061f,  56.47f, planetInfo[7].radius * PLANET_RADIUS_SCALE }
        };

        // 달 궤도 파라미터
        const float MOON_ORBIT_RADIUS = 3.0f;                   // 지구 반경 단위
        const float MOON_ORBIT_DEG_PER_DAY = 13.1764f;          // 일일 각도 이동량
        const float MOON_ECCENTRICITY = 0.0549f;                // 궤도 이심률
        const float MOON_INCLINATION = glm::radians(5.145f);    // 궤도 기울기

        // 각 행성 렌더링 루프
        for (int i = 0; i < planets.size(); ++i) {
            // 시뮬레이션 시간에 따른 궤도 위치 계산
            auto& p = planets[i];
            float angle = simulationTime * p.orbitSpeed;
            float rad = glm::radians(angle);
            glm::vec3 pos = glm::vec3(p.orbitRadius * cos(rad), 0.0f, p.orbitRadius * sin(rad));

            //현재 위치 저장
            currentPlanetPositions[i + 1] = pos;

            // 모델 변환: 이동 -> 회전 -> 스케일
            glm::mat4 model(1.0f);
            model = glm::translate(model, pos);
            model = glm::rotate(model, glm::radians(simulationTime * p.rotationSpeed), glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(p.scale));
            shader.setMat4("model", model);
            p.model.Draw(shader);

            //달 렌더링
            if (&p.model == &earth) {
                float mAngle = simulationTime * MOON_ORBIT_DEG_PER_DAY;
                float tM = glm::radians(mAngle);
                float rM = MOON_ORBIT_RADIUS * (1 - MOON_ECCENTRICITY * MOON_ECCENTRICITY) /
                    (1 + MOON_ECCENTRICITY * cos(tM));
                glm::vec3 moonOffset = glm::vec3(
                    cos(tM) * rM,
                    sin(tM) * sin(MOON_INCLINATION) * rM,
                    sin(tM) * cos(MOON_INCLINATION) * rM);
                glm::mat4 moonModel = glm::translate(glm::mat4(1.0f), pos + moonOffset);
                moonModel = glm::scale(moonModel, glm::vec3(0.1f));
                shader.setMat4("model", moonModel);
                moon.Draw(shader);
            }
        }
    }

    int main() {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 8);    //멀티샘플링
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

        GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System Shadow Mapping", nullptr, nullptr);
        if (window == NULL) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return -1;
        }
        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return -1;
        }

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);

        // Load shaders
        Shader planetShader("shaders/planet.vs", "shaders/planet.fs");
        Shader depthCubeShader("shaders/depth.vs", "shaders/depth.fs");
        Shader backgroundShader("shaders/background.vs", "shaders/background.fs");
        Shader orbitShader("shaders/orbit.vs", "shaders/orbit.fs");
        Shader ringShader("shaders/ring.vs", "shaders/ring.fs");
        ringShader.use();
        ringShader.setInt("diffuseMap", 0);

        // Load planet models
        Model sun("resources/objects/sun/sun.obj");
        Model mercury("resources/objects/mercury/mercury.obj");
        Model venus("resources/objects/venus/venus.obj");
        Model earth("resources/objects/earth/earth.obj");
        Model mars("resources/objects/mars/mars.obj");
        Model jupiter("resources/objects/jupiter/jupiter.obj");
        Model saturn("resources/objects/saturn/saturn.obj");
        Model uranus("resources/objects/uranus/uranus.obj");
        Model neptune("resources/objects/neptune/neptune.obj");
        Model moon("resources/objects/moon/moon.obj");
        Model rock("resources/objects/rock/rock.obj");

        // 링용 행성 인덱스 & 개수
        const unsigned RING_COUNT = 10000;

        // 링 매트릭스 미리 생성
        auto satMats = generateRing(currentPlanetPositions[6], planetInfo[5].radius * PLANET_RADIUS_SCALE * 1.2f, planetInfo[5].radius * PLANET_RADIUS_SCALE * 2.0f, RING_COUNT);
        auto jupMats = generateRing(currentPlanetPositions[5], planetInfo[4].radius * PLANET_RADIUS_SCALE * 1.1f, planetInfo[4].radius * PLANET_RADIUS_SCALE * 1.5f, RING_COUNT);
        auto uraMats = generateRing(currentPlanetPositions[7], planetInfo[6].radius * PLANET_RADIUS_SCALE * 1.1f, planetInfo[6].radius * PLANET_RADIUS_SCALE * 1.4f, RING_COUNT);
        auto nepMats = generateRing(currentPlanetPositions[8], planetInfo[7].radius * PLANET_RADIUS_SCALE * 1.1f, planetInfo[7].radius * PLANET_RADIUS_SCALE * 1.4f, RING_COUNT);

        // 전체 행렬 VBO에 업로드
        std::vector<glm::mat4> allMats;
        allMats.insert(allMats.end(), satMats.begin(), satMats.end());
        allMats.insert(allMats.end(), jupMats.begin(), jupMats.end());
        allMats.insert(allMats.end(), uraMats.begin(), uraMats.end());
        allMats.insert(allMats.end(), nepMats.begin(), nepMats.end());
        glGenBuffers(1, &ringMatrixVBO);
        glBindBuffer(GL_ARRAY_BUFFER, ringMatrixVBO);
        glBufferData(GL_ARRAY_BUFFER, allMats.size() * sizeof(glm::mat4), allMats.data(), GL_STATIC_DRAW);

        // 링 VAO 생성
        glGenVertexArrays(1, &ringVAO);
        glGenBuffers(1, &ringVBO);
        glGenBuffers(1, &ringEBO);
        glBindVertexArray(ringVAO);
        // 정점 + 인덱스
        glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
        glBufferData(GL_ARRAY_BUFFER, rock.meshes[0].vertices.size() * sizeof(Vertex), rock.meshes[0].vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
        glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ringEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, rock.meshes[0].indices.size() * sizeof(unsigned), rock.meshes[0].indices.data(), GL_STATIC_DRAW);
        // 매트릭스 속성
        glBindBuffer(GL_ARRAY_BUFFER, ringMatrixVBO);
        for (int i = 0; i < 4; ++i) {
            glEnableVertexAttribArray(3 + i);
            glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * i));
            glVertexAttribDivisor(3 + i, 1);
        }
        glBindVertexArray(0);
        // 궤도 반경만 담은 벡터 생성
            std::vector<float> orbitRadii;
            for (int i = 0; i < 8; ++i) {
                orbitRadii.push_back(planetInfo[i].a * AU_SCALE);
            }

            // 각 궤도 색상 설정
            std::vector<glm::vec3> orbitColors = {
                {1.0f, 0.5f, 0.5f},  // 수성 – 연한 빨강
                {1.0f, 0.8f, 0.4f},  // 금성 – 연한 주황
                {0.4f, 0.6f, 1.0f},  // 지구 – 연한 파랑
                {1.0f, 0.4f, 0.4f},  // 화성 – 붉은 주황
                {1.0f, 0.9f, 0.7f},  // 목성 – 크림색
                {0.9f, 0.8f, 0.5f},  // 토성 – 연한 노랑
                {0.6f, 0.8f, 0.9f},  // 천왕성 – 하늘색
                {0.5f, 0.7f, 0.9f}   // 해왕성 – 청록
            };

            // 궤도선 VAO/VBO 생성
            const int SEG = 1024;
            std::vector<unsigned int> orbitVAOs(orbitRadii.size()), orbitVBOs(orbitRadii.size());
            glGenVertexArrays((GLsizei)orbitRadii.size(), orbitVAOs.data());
            glGenBuffers((GLsizei)orbitRadii.size(), orbitVBOs.data());
            for (size_t i = 0; i < orbitRadii.size(); ++i) {
                float r = orbitRadii[i];
                std::vector<glm::vec3> pts;
                pts.reserve(SEG + 1);
                for (int j = 0; j <= SEG; ++j) {
                    float theta = glm::radians(360.0f * j / SEG);
                    pts.emplace_back(r * cos(theta), 0.0f, r * sin(theta));
                }
                glBindVertexArray(orbitVAOs[i]);
                glBindBuffer(GL_ARRAY_BUFFER, orbitVBOs[i]);
                glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(glm::vec3), pts.data(), GL_STATIC_DRAW);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
            }
            glBindVertexArray(0);


            // 배경 설정
            unsigned int bgVAO, bgVBO, bgTexture;
            {
                //화면 꽉 채우기
                float quad[] = {
                    -1,1,0,1, -1,-1,0,0, 1,-1,1,0,
                    -1,1,0,1, 1,-1,1,0, 1,1,1,1
                };

                glGenVertexArrays(1, &bgVAO);
                glGenBuffers(1, &bgVBO);
                glBindVertexArray(bgVAO);
                glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
                glGenTextures(1, &bgTexture);
                glBindTexture(GL_TEXTURE_2D, bgTexture);
                stbi_set_flip_vertically_on_load(false);
                int w, h, n;
                unsigned char* data = stbi_load("resources/textures/background_starmap.jpg", &w, &h, &n, 0);
                if (data) {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                    glGenerateMipmap(GL_TEXTURE_2D);
                }
                stbi_image_free(data);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }


            // 깊이맵 설정
            glGenFramebuffers(1, &depthCubemapFBO);
            glGenTextures(1, &depthCubemap);
            glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
            for (unsigned i = 0; i < 6; ++i) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glBindFramebuffer(GL_FRAMEBUFFER, depthCubemapFBO);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);


            // 셰이더 사용 및 유니폼 설정
            planetShader.use();
            planetShader.setInt("texture_diffuse", 0);
            planetShader.setInt("shadowMap", 1);

            glm::vec3 lightPos(0.0f);
            glm::mat4 shadowProjM = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);
            shadowTransforms = {
                shadowProjM * glm::lookAt(lightPos, lightPos + glm::vec3(1,0,0), glm::vec3(0,-1,0)),
                shadowProjM * glm::lookAt(lightPos, lightPos + glm::vec3(-1,0,0), glm::vec3(0,-1,0)),
                shadowProjM * glm::lookAt(lightPos, lightPos + glm::vec3(0,1,0), glm::vec3(0,0,1)),
                shadowProjM * glm::lookAt(lightPos, lightPos + glm::vec3(0,-1,0), glm::vec3(0,0,-1)),
                shadowProjM * glm::lookAt(lightPos, lightPos + glm::vec3(0,0,1), glm::vec3(0,-1,0)),
                shadowProjM * glm::lookAt(lightPos, lightPos + glm::vec3(0,0,-1), glm::vec3(0,-1,0))
            };

            while (!glfwWindowShouldClose(window)) {
                float cur = (float)glfwGetTime();
                deltaTime = cur - lastFrame;
                lastFrame = cur;
                processInput(window, deltaTime);
                if (!paused) simulationTime += deltaTime * timeMultiplier;

                // First pass: 그림자 맵에 깊이 정보 렌더
                glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
                glBindFramebuffer(GL_FRAMEBUFFER, depthCubemapFBO);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
                depthCubeShader.use();
                depthCubeShader.setVec3("lightPos", lightPos);
                depthCubeShader.setFloat("far_plane", far_plane);
                for (int i = 0; i < 6; ++i) {
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, depthCubemap, 0);
                    depthCubeShader.setMat4("shadowMatrix", shadowTransforms[i]);
                    glClear(GL_DEPTH_BUFFER_BIT);
                    renderScene(depthCubeShader, mercury, venus, earth, mars, jupiter, saturn, uranus, neptune, moon);
                }
                glCullFace(GL_BACK);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                // Second pass: 일반 렌더링
                glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                    (float)SCR_WIDTH / (float)SCR_HEIGHT,
                    0.1f, far_plane);

                // 배경 렌더
                glDepthMask(GL_FALSE);
                glDisable(GL_DEPTH_TEST);
                backgroundShader.use();
                backgroundShader.setInt("bgTex", 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, bgTexture);
                glBindVertexArray(bgVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glEnable(GL_DEPTH_TEST);
                glDepthMask(GL_TRUE);

                glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), float(SCR_WIDTH) / SCR_HEIGHT, 0.1f, 50000.0f);
                glm::mat4 view = camera.GetViewMatrix();

                // 궤도선 그리기
                orbitShader.use();
                orbitShader.setMat4("projection", proj);
                orbitShader.setMat4("view", view);

                for (size_t i = 0; i < orbitRadii.size(); ++i) {
                    orbitShader.setVec3("lineColor", glm::vec3(orbitColors[i]));
                    orbitShader.setMat4("model", glm::mat4(1.0f));
                    glBindVertexArray(orbitVAOs[i]);
                    glDrawArrays(GL_LINE_LOOP, 0, SEG + 1);
                }
                glBindVertexArray(0);


                // 행성 렌더
                planetShader.use();
                planetShader.setMat4("projection", proj);
                planetShader.setMat4("view", view);
                planetShader.setVec3("viewPos", camera.Position);
                planetShader.setVec3("lightPos", lightPos);
                planetShader.setFloat("far_plane", far_plane);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

                planetShader.setBool("isSun", true);
                glm::mat4 sunM = glm::scale(glm::mat4(1.0f), glm::vec3(696000.0f * 0.0005));
                planetShader.setMat4("model", sunM);
                sun.Draw(planetShader);

                planetShader.setBool("isSun", false);
                renderScene(planetShader, mercury, venus, earth, mars, jupiter, saturn, uranus, neptune, moon);

                // 링 렌더
                ringShader.use();
                ringShader.setMat4("projection", proj);
                ringShader.setMat4("view", view);
                glBindTexture(GL_TEXTURE_2D, rock.textures_loaded[0].id);
                glBindVertexArray(ringVAO);
                // 각 행성 센터에 맞춰 인스턴스 드로우
                const int ringPlanets[4] = { 5,6,7,8 };
                for (int i = 0; i < 4; ++i) {
                    ringShader.setVec3("planetCenter", currentPlanetPositions[ringPlanets[i]]);
                    glDrawElementsInstanced(GL_TRIANGLES, rock.meshes[0].indices.size(), GL_UNSIGNED_INT, nullptr, RING_COUNT);
                }

                glfwSwapBuffers(window);
                glfwPollEvents();
            }

            glfwTerminate();
            return 0;
}

    void processInput(GLFWwindow* w, float dt) {
        if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(w, true);
        const float cameraSpeedFactor = 3.0f; 
        float adjustedDt = dt * cameraSpeedFactor;

        if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, adjustedDt);
        if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, adjustedDt);
        if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, adjustedDt);
        if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, adjustedDt);

        static bool spaceDown = false;
        bool now = (glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS);
        if (now && !spaceDown) paused = !paused;
        spaceDown = now;

        if (glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS)
            timeMultiplier += 0.1f;
        if (glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS)
            timeMultiplier = std::max(0.1f, timeMultiplier - 0.1f);

        for (int key = GLFW_KEY_0; key <= GLFW_KEY_8; ++key) {
            if (glfwGetKey(w, key) == GLFW_PRESS) {
                int idx = key - GLFW_KEY_0;
                if (idx >= 0 && idx < currentPlanetPositions.size()) {
                    glm::vec3 target = currentPlanetPositions[idx];
                    float radius = (idx == 0) ? 696000.0f : planetInfo[idx - 1].radius;
                    float scaledRadius = radius * PLANET_RADIUS_SCALE;
                    float a = (idx == 0) ? 10.0f : planetInfo[idx - 1].a;
                    float scaledA = a * AU_SCALE;
                    float distance = scaledRadius + scaledA * 0.1f;
                    glm::vec3 camPos = target + glm::vec3(0.0f, distance * 0.3f, distance);

                    camera.Position = camPos;
                    camera.Front = glm::normalize(target - camPos);
                    updateCameraVectorsSafe();

                    //마우스 위치 동기화
                    double mx, my;
                    glfwGetCursorPos(w, &mx, &my);
                    lastX = (float)mx;
                    lastY = (float)my;
                }
            }
        }
    }

    void updateCameraVectorsSafe() {
        camera.Front = glm::normalize(camera.Front);
        camera.Right = glm::normalize(glm::cross(camera.Front, camera.WorldUp));
        camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
    }

    void framebuffer_size_callback(GLFWwindow* w, int w_, int h_) {
        glViewport(0, 0, w_, h_);
    }

    void mouse_callback(GLFWwindow* w, double x, double y) {
        if (firstMouse) {
            lastX = x; lastY = y; firstMouse = false;
        }
        float dx = x - lastX, dy = lastY - y;
        lastX = x; lastY = y;
        camera.ProcessMouseMovement(dx, dy);
    }

    void scroll_callback(GLFWwindow* w, double _, double y) {
        camera.ProcessMouseScroll((float)y);
    }
