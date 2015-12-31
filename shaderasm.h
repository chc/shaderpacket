#ifndef _SHADERASM_H
#define _SHADERASM_H
#include <stdint.h>
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
};
enum EMaterialFlags { //material accessors
	EMaterialFlags_TexCount = (1<<0),
};

enum EPreProcessorType {
	EPreProcessor_UVLength = 1,
	EPreProcessor_TexSamplerType,
	EPreProcessor_Marker,
};
bool isbranch(EShaderInstruction instruction);
bool isliteral(EShaderRegister reg);
#endif //_SHADERASM_H