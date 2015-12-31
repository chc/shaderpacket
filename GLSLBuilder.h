#ifndef _GLSLBUILDER_H
#define _GLSLBUILDER_H
#include "Builder.h"
namespace ShaderLib {
	typedef struct {
		EShaderInstruction instuction;
		uint8_t reg[3];
		uint8_t register_index[3];
		uint8_t accessor[3];
		uint16_t jump;
		union {
			float f_val;
			uint32_t uintval;
		} values[3][4]; //4 because of vectors
		uint8_t num_values[3];
	} InstructionPack;
	typedef struct {
		uint8_t mode;
		const char *name;
	} NameMap;

	class GLSLBuilder {
	public:
		GLSLBuilder();
		~GLSLBuilder();
		void build(char *out, int *out_len, const char *src, int src_len);
	private:
		void get_instruction(char *out, int *len, InstructionPack *data);
		void get_operand(InstructionPack *data, int index, char *out, int *out_len);
		static NameMap m_texname_map[3];
		int m_uv_mode[3];
	};
}
#endif //_GLSLBUILDER_H