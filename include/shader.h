#ifndef _SHADER_H_
#define _SHADER_H_

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstddef>
#include <map>
#include <vector>

#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1

#define _USE_MATH_DEFINES
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE
//#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "stb_image_write.h"
#define TINYGLTF_ENABLE_DRACO
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "tiny_gltf.h"

#if 0
    #define EXCEPTIONS
#endif
#if 0
	#define OPTIMIZE
#endif

#ifndef uint
#define uint unsigned int
#endif

// 0 for FPS 1 for Turntable
//#define CAMERA 1
enum TYPE_OF_CAMERA
{
	FPS,
	TRACK_BALL
};

//TYPE_OF_CAMERA CAMERA = FPS;

// Shader class taken from, there are some functions that are implemented by me: 
// https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/shader_m.h

class Shader
{
public:
	unsigned int ID;
	int Width,
		Height;
	// constructor generates the shader on the fly
	// ------------------------------------------------------------------------
	Shader()
	{}
	Shader(const char* vertexPath, const char* fragmentPath, const std::vector<std::string>& attribLocationsNames, const std::string& GLSLVersion)
	{
		// 1. retrieve the vertex/fragment source code from filePath
		std::string vertexCode   = GLSLVersion;
		std::string fragmentCode = GLSLVersion;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;
		// ensure ifstream objects can throw exceptions:
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		#ifdef EXCEPTIONS
		try
		{
			// open files
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;
			// read file's buffer contents into streams
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			// close file handlers
			vShaderFile.close();
			fShaderFile.close();
			// convert stream into string
			vertexCode += vShaderStream.str();
			fragmentCode += fShaderStream.str();
		}
		catch (std::ifstream::failure& e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}
		#else
		// open files
		vShaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
		std::stringstream vShaderStream, fShaderStream;
		// read file's buffer contents into streams
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		// close file handlers
		vShaderFile.close();
		fShaderFile.close();
		// convert stream into string
		vertexCode += vShaderStream.str();
		fragmentCode += fShaderStream.str();
		/*std::string addVersionVertexCode = "#version 300 es\n" + vertexCode;
		std::cout << "Vertex code: " << addVersionVertexCode << std::endl;*/
		#endif
		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();
		// 2. compile shaders
		unsigned int vertex, fragment;
		// vertex shader
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		checkCompileErrors(vertex, "VERTEX");
		// fragment Shader
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		checkCompileErrors(fragment, "FRAGMENT");
		// shader Program
		ID = glCreateProgram();
        setAttributeLocations(attribLocationsNames);
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		glLinkProgram(ID);
		checkCompileErrors(ID, "PROGRAM");
		// delete the shaders as they're linked into our program now and no longer necessery
		glDeleteShader(vertex);
		glDeleteShader(fragment);

	}
	// activate the shader
	// ------------------------------------------------------------------------
	void use() const
	{
		glUseProgram(ID);
	}
    // Set attribute location
    // ------------------------------------------------------------------------
    void setAttributeLocation(const int index, const std::string& name)
    {
        glBindAttribLocation(ID, index, name.c_str());
    }
    void setAttributeLocations(const std::vector<std::string>& names)
    {
        for (uint8_t i = 0; i < names.size(); ++i)
        {
            setAttributeLocation((int) i, names[i]);
        }
    }
    // ------------------------------------------------------------------------
	// utility uniform functions
	// ------------------------------------------------------------------------
	void setBool(const std::string& name, bool value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
	}
	// ------------------------------------------------------------------------
	void setInt(const std::string& name, int value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}
	// ------------------------------------------------------------------------
	void setFloat(const std::string& name, float value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}
	// ------------------------------------------------------------------------
	void setVec2(const std::string& name, const glm::vec2& value) const
	{
		glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec2(const std::string& name, float x, float y) const
	{
		glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
	}
	// ------------------------------------------------------------------------
	void setVec3(const std::string& name, const glm::vec3& value) const
	{
		glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec3(const std::string& name, float x, float y, float z) const
	{
		glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
	}
	// ------------------------------------------------------------------------
	void setVec4(const std::string& name, const glm::vec4& value) const
	{
		glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec4(const std::string& name, float x, float y, float z, float w) const
	{
		glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
	}
	// ------------------------------------------------------------------------
	void setMat2(const std::string& name, const glm::mat2& mat) const
	{
		glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat3(const std::string& name, const glm::mat3& mat) const
	{
		glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat4(const std::string& name, const glm::mat4& mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

    enum TEXTURE_WRAP_PARAMS
    {
        REPEAT,
        CLAMP_TO_EDGE,
        CLAMP_TO_BORDER
    };
    
    enum TEXTURE_SAMPLE_PARAMS
    {
        LINEAR,
        LINEAR_MIPS,
        NEAREST,
        NEAREST_MIPS
    };
    
    /**
     * @brief This function calls the apropiate methods for different wrapping options.
     *
     * To be able to call it you must pass an enum TEXTURE_WRAP_PARAMS that defines the wrapping parameters.
     * 0 = repeat
     * 1 = clamp to edge
     * 2 = camp to border
     *
     * @param wrapParam This parameter controls the wrapping.
     * @return Returns void, so no result.
     */
    void textureWrap(const TEXTURE_WRAP_PARAMS& wrapParam)
    {
        GLint textureWrap;
        switch (wrapParam)
        {
            case 0:
            {
                textureWrap = GL_REPEAT;
                break;
            }
            case 1:
            {
                textureWrap = GL_CLAMP_TO_EDGE;
                break;
            }
            case 2:
            {
#ifdef __EMSCRIPTEN__
				std::cout << "Not supported!" << std::endl;
#else
                textureWrap = GL_CLAMP_TO_BORDER;
#endif
                break;
            }
            default:
            {
                textureWrap = GL_REPEAT;
                break;
            }
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureWrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureWrap);
    }
    
    /**
     * @brief This function calls the apropiate methods for different sampling options.
     *
     * To be able to call it you must pass an enum TEXTURE_SAMPLE_PARAMS that defines the sampling parameters.
     * 0 = linear
     * 1 = linear with mip maps
     * 2 = nearest
     * 3 = nearest with mip maps
     *
     * @param sampleParam This parameter controls the sampling.
     * @return Returns void, so no result.
     */
    void textureSample(const TEXTURE_SAMPLE_PARAMS& sampleParam)
    {
        GLint textureSampleMin;
        GLint textureSampleMag;
        switch (sampleParam)
        {
            case 0:
            {
                textureSampleMin = GL_LINEAR;
                textureSampleMag = GL_LINEAR;
                break;
            }
            case 1:
            {
                textureSampleMin = GL_LINEAR_MIPMAP_LINEAR;
                textureSampleMag = GL_LINEAR;
                break;
            }
            case 2:
            {
                textureSampleMin = GL_NEAREST;
                textureSampleMag = GL_NEAREST;
                break;
            }
            case 3:
            {
                textureSampleMin = GL_NEAREST;
                textureSampleMag = GL_NEAREST_MIPMAP_LINEAR;
                break;
            }
            default:
            {
                textureSampleMin = GL_LINEAR;
                textureSampleMag = GL_LINEAR;
                break;
            }
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureSampleMin);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureSampleMag);
    }
    
	void createTexture(unsigned int* texture, std::string fileName, const TEXTURE_WRAP_PARAMS& wrapParam, const TEXTURE_SAMPLE_PARAMS& sampleParam, std::string samplerName,
		int uniform, GLint& format
	)
	{
		glGenTextures(1, texture);
		glBindTexture(GL_TEXTURE_2D, *texture);

		// In an ideal world this should be exposed as input params to the function.
		// Texture wrapping params.
        textureWrap(wrapParam);
		// Texture filtering params.
        textureSample(sampleParam);

		// Load image, texture creating and generation of mipmaps.
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true); // Yes... I am talking to you Vulkan!

		unsigned char* data = stbi_load(fileName.c_str(), &width, &height, &channels, 0);
		Width  = width;
		Height = height;

		#ifdef OPTIMIZE
		#else
		std::cout << "Texture path: " << fileName << std::endl;
		std::cout << "Original texture data: " << +data[0] << std::endl;
		#endif
		// Get the texture format automatically.
		format = GL_RGBA;
		if (channels == 1)
		{
			format = GL_RED;
		}
		else if (channels == 3)
		{
			format = GL_RGB;
		}
		else if (channels == 4)
		{
			format = GL_RGBA;
		}
		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			if (sampleParam == LINEAR_MIPS)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
			}
		}
		else
		{
			#ifdef EXCEPTIONS
			throw std::runtime_error("Failed to load texture!");
			#else
			std::cout << "Failed to load texture!" << std::endl;
			#endif
		}
		// Clear the data.
		stbi_image_free(data);

		// Bind the uniform sampler.
		this->use();
		this->setInt(samplerName, uniform);
	}

	void createTextureFromFile(unsigned int* texture, uint8_t* buffer, int width, int height, std::string samplerName,
		int uniform)
	{
		if (buffer)
        {
            glGenTextures(1, texture);
            glBindTexture(GL_TEXTURE_2D, *texture);

            // In an ideal world this should be exposed as input params to the function.
            // Texture wrapping params.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            // Texture filtering params.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Get the texture format automatically.
            auto format = GL_RGBA;
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, buffer);
			glGenerateMipmap(GL_TEXTURE_2D);

            // Bind the uniform sampler.
            // Bind the uniform sampler.
			this->use();
			this->setInt(samplerName, uniform);
        }
		else
		{
			#ifdef EXCEPTIONS
			throw std::runtime_error("Failed to load texture!");
			#else
			std::cout << "Failed to load texture!" << std::endl;
			#endif
		}
	}

private:
	// utility function for checking shader compilation/linking errors.
	// ------------------------------------------------------------------------
	void checkCompileErrors(GLuint shader, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if (type != "PROGRAM")
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}
};
#endif
