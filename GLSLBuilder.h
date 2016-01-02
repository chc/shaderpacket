#ifndef _GLSLBUILDER_H
#define _GLSLBUILDER_H
#include "Builder.h"
#include "shaderasm.h"
namespace ShaderLib {
	typedef struct {
		ShaderASM::EShaderInstruction instuction;
		uint8_t reg[MAX_OPERANDS];
		uint8_t register_index[MAX_OPERANDS];
		uint8_t accessor[MAX_OPERANDS];
		uint16_t jump;
		union {
			float f_val;
			uint32_t uintval;
		} values[3][4]; //4 because of vectors
		uint8_t num_values[3];
		ShaderASM::InstructionModifiers modifiers;
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