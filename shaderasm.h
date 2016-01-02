#ifndef _SHADERASM_H
#define _SHADERASM_H
#include <stdint.h>
namespace ShaderASM {
enum ETextureSampleType {
	ETextureSampleType_2D = 1,
	ETextureSampleType_Cube,
	ETextureSampleType_Rect,
};

enum EShaderInstruction {
	//branch instructions
	EShaderInstruction_BranchBegin = 1,
	EShaderInstruction_BranchLE = EShaderInstruction_BranchBegin,
	EShaderInstruction_BranchGE,
	EShaderInstruction_BranchE,
	EShaderInstruction_BranchNE,
	EShaderInstruction_BranchZ,
	EShaderInstruction_BranchNZ,
	EShaderInstruction_BranchEnd = EShaderInstruction_BranchNZ,
	EShaderInstruction_Test,
	//Operation+Assignment
	EShaderInstruction_AddCpy,
	EShaderInstruction_SubCpy,
	EShaderInstruction_MulCpy,
	EShaderInstruction_DivCpy,
	//Operation
	EShaderInstruction_Add,
	EShaderInstruction_Sub,
	EShaderInstruction_Mul,
	EShaderInstruction_Div,
	EShaderInstruction_Mov,
	//special instructions
	EShaderInstruction_SampleTex,
	EShaderInstruction_Lerp,
};

/*
the offset is not considered here
*/
enum EShaderRegister {
	//input
	EShaderRegister_Texture = 1,
	EShaderRegister_UVW, //or UV
	EShaderRegister_Const, //user defined constant(vec4s)
	EShaderRegister_Col, //vertex colour
	EShaderRegister_Mat, //Material info
	//temp
	EShaderRegister_Vector,
	EShaderRegister_HiVector,
	EShaderRegister_LoVector,
	//output
	EShaderRegister_OutColour,

	//literal
	EShaderRegister_BeginLiteral,
	EShaderRegister_Float = EShaderRegister_BeginLiteral,
	EShaderRegister_UInt,
	EShaderRegister_EndLiteral = EShaderRegister_UInt,
};

enum EVectorFlags {
	EVectorFlags_Red = (1<<0),
	EVectorFlags_Green = (1<<1),
	EVectorFlags_Blue = (1<<2),
	EVectorFlags_Alpha = (1<<3),
	EMaterialDataType_TexCount = (1<<4),
	EMaterialDataType_Diffuse = (1<<5),
	EVectorFlags_Negate = (1<<6),
};

enum EPreProcessorType {
	EPreProcessor_UVLength = 1,
	EPreProcessor_TexSamplerType,
	EPreProcessor_Marker,
};
typedef struct {
	float multiplier; //0.0 = no mult
	float clamp_min;//0.0 - 0.0 = no clamp
	float clamp_max; 
} InstructionModifiers;

#define MAX_MATERIAL_TEXTURES 4
#define MAX_JUMPS 100
#define BUFFER_SIZE 1000

typedef struct {
	ETextureSampleType tex_types[MAX_MATERIAL_TEXTURES];
	uint8_t UVMode; //2, or 3(xy, xyz)

	char opcodeBuffer[BUFFER_SIZE];
	int opcodeWriteIDX;
	int instruction_count;
	int jump_index;
	
} ShaderASMState;


#define MAX_OPERANDS 4
typedef struct {
	EShaderRegister registers[MAX_OPERANDS];
	uint32_t accessors[MAX_OPERANDS];
	char indexes[MAX_OPERANDS];
	union {
		float f_val;
		uint32_t uintval;
	} values[MAX_OPERANDS][4]; //4 because of vectors
	uint8_t num_values[MAX_OPERANDS]; //how many values are written
	uint16_t jump_index; //where a branch should jump to
} EOperandInfo;
extern ShaderASMState g_asmState;

bool isbranch(EShaderInstruction instruction);
bool isliteral(EShaderRegister reg);

void preprocess_shader_from_file(FILE *fd);
void preprocess_shader_mem(void *data, int len);

void compile_shader_file(FILE *fd);
void compile_shader_mem(void *data, int len);

void run_shader_file(FILE *fd);
void run_shader_mem(void *data, int len);

}
#endif //_SHADERASM_H