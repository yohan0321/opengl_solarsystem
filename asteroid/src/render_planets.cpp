#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <learnopengl/shader.h>
#include <learnopengl/model.h>
#include <vector>

float simulationTime = 0.0f;

// Planet and moon rendering helper
void renderScene(
    Shader& shader,
    Model& mercury, Model& venus, Model& earth,
    Model& mars, Model& jupiter, Model& saturn,
    Model& uranus, Model& neptune, Model& moon)
{
    struct Planet { Model& model; float orbitRadius, orbitSpeed, rotationSpeed, scale; };
    std::vector<Planet> planets = {
        { mercury, 35.0f, 4.15f,    6.2f,     0.2f },
        { venus,   40.0f, 1.62f,   -1.481f,  0.2f },
        { earth,   50.0f, 1.00f,  360.0f,    0.2f },
        { mars,    60.0f, 0.53f,  370.8f,    0.2f },
        { jupiter, 80.0f, 0.08f,  881.632f,  0.2f },
        { saturn, 100.0f, 0.03f,  822.857f,  0.2f },
        { uranus, 120.0f, 0.0119f,-508.235f, 0.2f },
        { neptune,140.0f, 0.0061f, 56.47f,   0.2f }
    };

    const float MOON_ORBIT_RADIUS = 3.0f;
    const float MOON_ORBIT_DEG_PER_DAY = 13.1764f;
    const float MOON_ECCENTRICITY = 0.0549f;
    const float MOON_INCLINATION = glm::radians(5.145f);

    for (auto& p : planets) {
        float angle = simulationTime * p.orbitSpeed;
        float rad = glm::radians(angle);
        glm::vec3 pos = glm::vec3(
            p.orbitRadius * cos(rad),
            0.0f,
            p.orbitRadius * sin(rad)
        );

        // Planet transformation
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
        model = glm::rotate(
            model,
            glm::radians(simulationTime * p.rotationSpeed),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        model = glm::scale(model, glm::vec3(p.scale));
        shader.setMat4("model", model);
        p.model.Draw(shader);

        // Moon is rendered only for Earth
        if (&p.model == &earth) {
            float mAngle = simulationTime * MOON_ORBIT_DEG_PER_DAY;
            float tM = glm::radians(mAngle);
            float rM = MOON_ORBIT_RADIUS * (1 - MOON_ECCENTRICITY * MOON_ECCENTRICITY)
                / (1 + MOON_ECCENTRICITY * cos(tM));
            glm::vec3 moonOffset = glm::vec3(
                cos(tM) * rM,
                sin(tM) * sin(MOON_INCLINATION) * rM,
                sin(tM) * cos(MOON_INCLINATION) * rM
            );
            glm::mat4 moonModel = glm::translate(
                glm::mat4(1.0f), pos + moonOffset
            );
            moonModel = glm::scale(moonModel, glm::vec3(0.1f));
            shader.setMat4("model", moonModel);
            moon.Draw(shader);
        }
    }
}
