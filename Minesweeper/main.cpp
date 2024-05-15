#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <iostream>


// --------------------------------- variables ---------------------------------
#define WIN_WIDTH 1000
#define WIN_HEIGHT 1000

#define FIELD_SIZE 8
#define NUM_MINES 8


const char* vertex_src = "#version 330 core\n"
"layout (location = 0) in vec2 aPosition;\n"
"layout (location = 1) in vec2 aTexCoord;\n"
"out vec2 uv;\n"
"uniform vec4 transform;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPosition * transform.zw + transform.xy, 0.0, 1.0);\n"
"   uv = aTexCoord;\n"
"}\0";

const char* fragment_src = "#version 330 core\n"
"in vec2 uv;\n"
"out vec4 fragColor;\n"
"uniform sampler2D tex;\n"
"void main()\n"
"{\n"
"   fragColor = texture(tex, uv);\n"
"}\0";


bool game_lost = false;

bool first_click = true;


const float quad_vertices[24] = {
    0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f
};

int transform_location;


#define NUM_TEXTURES 12

const char texture_id[NUM_TEXTURES][21] {
    "textures/field_e.png", // 0
    "textures/field_1.png", // 1
    "textures/field_2.png", // 2
    "textures/field_3.png", // 3
    "textures/field_4.png", // 4
    "textures/field_5.png", // 5
    "textures/field_6.png", // 6
    "textures/field_7.png", // 7
    "textures/field_8.png", // 8
    "textures/field_m.png", // 9
    "textures/field_g.png", // 10
    "textures/field_f.png"  // 11

};

unsigned int textures[NUM_TEXTURES];


struct Field {
    float x;
    float y;
    unsigned int type;
    unsigned int texture;
};

float field_scale = 1.0f;

unsigned int field_data_array[FIELD_SIZE][FIELD_SIZE];

Field field_obj_array[FIELD_SIZE * FIELD_SIZE];


void resetFieldData();
void resetFields();

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);


float mouse_x = 0.0f;
float mouse_y = 0.0f;


int main() {
    // --------------------------------- window init ---------------------------------
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Minesweeper", NULL, NULL);
    if (window == NULL) {
        std::cout << "ERROR::GLFW::WINDOW::INITIALIZATION_FAILED" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "ERROR::GLAD::INITIALIZATION_FAILED" << std::endl;
        glfwTerminate();
        return -1;
    }

    std::cout << "INFO::OPENGL::VERSION\n" << glGetString(GL_VERSION) << std::endl;


    // --------------------------------- shader compilation ---------------------------------
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_src, NULL);
    glCompileShader(vertex_shader);

    int success;
    char info_log[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << info_log << std::endl;
    }

    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_src, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << info_log << std::endl;
    }

    unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << info_log << std::endl;
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);


    // --------------------------------- quad VAO ---------------------------------
    unsigned int quad_VBO, quad_VAO;
    glGenVertexArrays(1, &quad_VAO);
    glGenBuffers(1, &quad_VBO);

    glBindVertexArray(quad_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, quad_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void*)8);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);


    // --------------------------------- load textures ---------------------------------
    stbi_set_flip_vertically_on_load(true);

    for (int i = 0; i < NUM_TEXTURES; i++) {
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int width, height, nr_channels;
        unsigned char* data = stbi_load(texture_id[i], &width, &height, &nr_channels, 3);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else {
            std::cout << "ERROR::TEXTURE::LOAD_FAILED" << std::endl;
        }
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);

        textures[i] = texture;
    }


    // --------------------------------- main application ---------------------------------
    srand((unsigned int)time(NULL));

    resetFieldData();
    resetFields();


    glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glUseProgram(shader_program);
    glBindVertexArray(quad_VAO);

    transform_location = glGetUniformLocation(shader_program, "transform");

    
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        for (int i = 0; i < (FIELD_SIZE * FIELD_SIZE); i++) {
            Field& cur_field = field_obj_array[i];
            glUniform4f(transform_location, cur_field.x, cur_field.y, field_scale, field_scale);
            glBindTexture(GL_TEXTURE_2D, textures[cur_field.texture]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glfwTerminate();
    return 0;
}


bool isMineAtPos(int x, int y) {
    if (-1 < x && x < FIELD_SIZE && -1 < y && y < FIELD_SIZE) {
        if (field_data_array[x][y] == 9)
            return true;
        else
            return false;
    }
    else
        return false;
}

void resetFieldData() {
    for (int x = 0; x < FIELD_SIZE; x++) {
        for (int y = 0; y < FIELD_SIZE; y++) {
            field_data_array[x][y] = 0;
        }
    }

    for (int i = 0; i < NUM_MINES; i++) {
        field_data_array[(int)(((float)rand() / RAND_MAX) * (float)FIELD_SIZE)]
                        [(int)(((float)rand() / RAND_MAX) * (float)FIELD_SIZE)] = 9;
    }

    for (int x = 0; x < FIELD_SIZE; x++) {
        for (int y = 0; y < FIELD_SIZE; y++) {
            if (!(field_data_array[x][y] == 9)) {
                unsigned int number = 0;

                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        if (isMineAtPos(x + i, y + j))
                            number += 1;
                    }
                }

                field_data_array[x][y] = number;
            }
        }
    }
}

void resetFields() {
    unsigned int index = 0;
    for (int x = 0; x < FIELD_SIZE; x++) {
        for (int y = 0; y < FIELD_SIZE; y++) {
            field_obj_array[index] = Field();
                
            field_obj_array[index].x = ((float)x / (float)FIELD_SIZE) * 2.0f - 1.0f;
            field_obj_array[index].y = ((float)y / (float)FIELD_SIZE) * 2.0f - 1.0f;
            field_obj_array[index].type = field_data_array[x][y];
            field_obj_array[index].texture = 10;

            index += 1;
        }
    }

    field_scale = 2.0f / (float)FIELD_SIZE;
}

unsigned int getFieldIndexAtPos(float x, float y) {
    for (int i = 0; i < FIELD_SIZE * FIELD_SIZE; i++) {
        Field& cur_field = field_obj_array[i];

        if (x > cur_field.x && x < cur_field.x + field_scale) {
            if (y > cur_field.y && y < cur_field.y + field_scale)
                return i;
        }
    }
    return -1;
}

unsigned int sampleFieldData(float x, float y) {
    return field_data_array[(int)((x * 0.5f + 0.5f) * (float)FIELD_SIZE)][(int)((y * 0.5f + 0.5f) * (float)FIELD_SIZE)];
}


void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    mouse_x = ((float)xpos / (float)WIN_WIDTH) * 2.0f - 1.0f;
    mouse_y = (1.0f - (float)ypos / (float)WIN_HEIGHT) * 2.0f - 1.0f;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (first_click) {
            if (sampleFieldData(mouse_x, mouse_y) != 0) {
                for (int i = 0; i < 100; i++) {
                    resetFieldData();
                    if (sampleFieldData(mouse_x, mouse_y) == 0) {
                        resetFields();
                        first_click = false;
                        break;
                    }
                }
            }
            else
                first_click = false;
        }

        unsigned int index = getFieldIndexAtPos(mouse_x, mouse_y);
        if (index != -1) {
            Field& cur_field = field_obj_array[index];
            if (cur_field.texture != 11) {
                cur_field.texture = cur_field.type;
                if (cur_field.type == 9)
                    game_lost = true;
            }
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        int index = getFieldIndexAtPos(mouse_x, mouse_y);
        if (index != -1) {
            Field& cur_field = field_obj_array[index];
            if (cur_field.texture == 10)
                cur_field.texture = 11;
            else if (cur_field.texture == 11)
                cur_field.texture = 10;
        }
    }
}