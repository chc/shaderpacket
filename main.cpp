#include <stdio.h>
#include <stdint.h>
#include <memory.h>
enum {
	EVectorElement_Alpha = (1<<0),
	EVectorElement_Red = (1<<1),
	EVectorElement_Green = (1<<2),
	EVectorElement_Blue = (1<<3),
} EVectorElements;

enum EOperation {
	EOperation_Add,
	EOperation_Subtract,
	EOperation_Multiply,
	EOperation_Div,
	EOperation_Mod,
	EOperation_Dot,
	EOperation_Clamp_Zero_One,
	EOperation_Clamp_NegOne_One,
	EOperation_Assign,
};

enum EVariableType {
 	EVariableType_Float,
 	EVariableType_Uint32,
 	EVariableType_Texture,
	EVariableType_Vector,
};
typedef struct {
	float d[4];
	uint32_t flags;
} ShaderVector;
typedef struct {
	EVariableType type;
	union {
		float f_const;
		uint32_t ui32_const;
		int texunit;
		ShaderVector vector;
		EVariableType cast_type;
	};

} EOperationParameterPack;

typedef struct {
	EOperation operation;
	EOperationParameterPack operands[2];
} EShaderOperation;

void generate_operand(EOperationParameterPack *pack, char *out, int size) {
	switch(pack->type) {
		case EVariableType_Float: 
			sprintf_s(out, size, "%ff",pack->f_const);
		break;
		case EVariableType_Vector:
			//printf("
			break;
	}
}
void generate_operation(EShaderOperation *operation) {
	char ops[3][64];
	generate_operand(&operation->operands[0], (char *)&ops[0], 64);
	generate_operand(&operation->operands[1], (char *)&ops[1], 64);
	generate_operand(&operation->operands[2], (char *)&ops[2], 64);
	switch(operation->operation) {
		case EOperation_Add:
			break;
		case EOperation_Multiply:
			printf("%s = %s*%s;\n",ops[0],ops[1],ops[2]);
			break;
	}
}
void generate_shader(EShaderOperation *ops, uint32_t num_ops) {
	generate_operation(&ops[0]);
}
int main() {
	const uint32_t num_instructions = 1;
	EShaderOperation ops[num_instructions];
	memset(&ops,0,sizeof(ops));

	EOperationParameterPack packs[3];
	memset(&packs,0,sizeof(packs));
	packs[1].type = EVariableType_Float;
	packs[1].f_const = 1.0;
	packs[2].type = EVariableType_Float;
	packs[2].f_const = 2.0;
	ops[0].operation = EOperation_Multiply;
	ops[0].operands[0] = packs[0];
	ops[0].operands[1] = packs[1];

	generate_shader((EShaderOperation*)&ops, 1);
	return 0;
}