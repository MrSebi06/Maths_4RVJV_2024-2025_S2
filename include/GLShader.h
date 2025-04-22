#ifndef GLSHADER_H
#define GLSHADER_H

#include <GL/glew.h>
#include <cstdint>

class GLShader {
public:
	GLShader() : m_Program(0), m_VertexShader(0), m_GeometryShader(0), m_FragmentShader(0) {}
	~GLShader() {}

	bool LoadVertexShader(const char* filename);
	bool LoadGeometryShader(const char* filename);
	bool LoadFragmentShader(const char* filename);
	bool Create();
	void Destroy();

	void Begin() { glUseProgram(m_Program); }
	void End() { glUseProgram(0); }

	GLuint GetProgram() const { return m_Program; }

	// Utilitaires pour uniformes
	void SetUniform(const char* name, float value) {
		GLint location = glGetUniformLocation(m_Program, name);
		if (location != -1) {
			glUniform1f(location, value);
		}
	}

	void SetUniform(const char* name, int value) {
		GLint location = glGetUniformLocation(m_Program, name);
		if (location != -1) {
			glUniform1i(location, value);
		}
	}

	void SetUniform(const char* name, float x, float y, float z) {
		GLint location = glGetUniformLocation(m_Program, name);
		if (location != -1) {
			glUniform3f(location, x, y, z);
		}
	}

private:
	GLuint m_Program;
	GLuint m_VertexShader;
	GLuint m_GeometryShader;
	GLuint m_FragmentShader;
};

#endif // GLSHADER_H