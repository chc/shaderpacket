#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>

#include "shaderasm.h"

#include "GLSLBuilder.h"

#define MAX_MATERIAL_TEXTURES 4
#define MAX_JUMPS 100
#define BUFFER_SIZE 1000

#define DefineSearchFunction(return_type, func_name, struct_name, struct_name_var, struct_return_var) return_type func_name(const char *name) { \
		for(int i=0;i<sizeof(struct_name)/sizeof(struct_name[0]);i++) { \
			if(!stricmp(struct_name[i].struct_name_var, name)) { \
				return struct_name[i].struct_return_var; \
			} \
		} \
		return (return_type)-1; \
}

struct {
	ETextureSampleType type;
	char *name;
} SamplerMap[] = {
	{ETextureSampleType_2D, "2D"},
	{ETextureSampleType_Cube, "Cube"},
	{ETextureSampleType_Rect, "Rect"},
};

struct {
	EShaderInstruction instruction;
	const char *name;
} InstructionMap[] = {

	//branch instructions
	{EShaderInstruction_BranchLE, "BLE"},
	{EShaderInstruction_BranchGE, "BGE"},
	{EShaderInstruction_BranchE, "BE"},
	{EShaderInstruction_BranchNE, "BNE"},
	{EShaderInstruction_BranchLE, "BLE"},
	{EShaderInstruction_BranchZ, "BZ"},
	{EShaderInstruction_BranchNZ, "BNZ"},
	{EShaderInstruction_Test, "TEST"},
	//Operator+Assignment
	{EShaderInstruction_AddCpy, "ADDCPY"},
	{EShaderInstruction_SubCpy, "SUBCPY"},
	{EShaderInstruction_MulCpy, "MULCPY"},
	{EShaderInstruction_DivCpy, "DIVCPY"},
	//assignment operations
	{EShaderInstruction_Add, "ADD"},
	{EShaderInstruction_Sub, "SUB"},
	{EShaderInstruction_Mul, "MUL"},
	{EShaderInstruction_Div, "DIV"},
	{EShaderInstruction_Mov, "MOV"},
	//special instructions
	{EShaderInstruction_SampleTex, "STEX"},
};


struct {
	EShaderRegister reg;
	const char *name;
} RegisterMap[] = {
	{EShaderRegister_Texture, "TEX"},
	{EShaderRegister_UVW, "UVW"},
	{EShaderRegister_UVW, "UV"},
	{EShaderRegister_Const, "CONST"},
	{EShaderRegister_Col, "VERTCOL"},
	{EShaderRegister_Mat, "MAT"},
	{EShaderRegister_Vector, "VEC"},
	{EShaderRegister_HiVector, "HVEC"},
	{EShaderRegister_LoVector, "LVEC"},
	{EShaderRegister_OutColour, "OUTCOL"},
};


struct {
	EVectorFlags flag;
	char ch;
} VertexAccessorMap[] = {
	{EVectorFlags_Red, 'R'},
	{EVectorFlags_Green, 'G'},
	{EVectorFlags_Blue, 'B'},
	{EVectorFlags_Alpha, 'A'},

	{EVectorFlags_Red, 'X'},
	{EVectorFlags_Green, 'Y'},
	{EVectorFlags_Blue, 'Z'},
	{EVectorFlags_Alpha, 'W'},
};

enum EMaterialDataType {
	EMaterialDataType_TexCount,
	EMaterialDataType_Diffuse,
};

struct {
	EMaterialDataType type;
	const char *name;
} MaterialAccessors[] = {
	{EMaterialDataType_TexCount, "TEXCOUNT"},
	{EMaterialDataType_Diffuse, "DIFFUSE"},
};


struct {
	EPreProcessorType type;
	const char *name;
} PreProcessorOptions[] = {
	{EPreProcessor_UVLength, "UVMODE"},
	{EPreProcessor_TexSamplerType, "TEXTYPE"},
	{EPreProcessor_Marker, "MARKER"},
};

struct {
	ETextureSampleType tex_types[MAX_MATERIAL_TEXTURES];
	uint8_t UVMode; //2, or 3(xy, xyz)

	char opcodeBuffer[BUFFER_SIZE];
	int opcodeWriteIDX;
	int instruction_count;
	int jump_index;
	
} ShaderASMState;

struct {
	char name[32];
	uint16_t index;
} JumpTable[MAX_JUMPS];

typedef struct {
	EShaderRegister registers[3];
	uint32_t accessors[3];
	char indexes[3];
	union {
		float f_val;
		uint32_t uintval;
	} values[3][4]; //4 because of vectors
	uint8_t num_values[3]; //how many values are written
	uint16_t jump_index; //where a branch should jump to
	
} EOperandInfo;
uint16_t find_jump_index_by_name(const char *name) {
	for(int i=0;i<MAX_JUMPS;i++) {
		if(!strcmp(name, JumpTable[i].name)) {
			return JumpTable[i].index;
		}
	}
	return -1;
}
void write_instruction(EShaderInstruction inst, EOperandInfo *operands) {
	ShaderASMState.opcodeBuffer[ShaderASMState.opcodeWriteIDX++] = (uint8_t)inst;
	ShaderASMState.instruction_count++;
	if(isbranch(inst)) {
		memcpy(&ShaderASMState.opcodeBuffer[ShaderASMState.opcodeWriteIDX], &operands->jump_index, sizeof(uint16_t));
		ShaderASMState.opcodeWriteIDX += sizeof(uint16_t);
		return;
	}
	uint8_t operand_count = 0;
	for(int i=0;i<3;i++) {
		if(operands->registers[i] != 0) {
			operand_count++;
		}
	}
	ShaderASMState.opcodeBuffer[ShaderASMState.opcodeWriteIDX++] = operand_count;
	for(int i=0;i<operand_count;i++) {
		ShaderASMState.opcodeBuffer[ShaderASMState.opcodeWriteIDX++] = (uint8_t)operands->registers[i];

		if(isliteral(operands->registers[i])) {
			ShaderASMState.opcodeBuffer[ShaderASMState.opcodeWriteIDX++] = (uint8_t)operands->num_values[i];
			switch(operands->registers[i]) {
				case EShaderRegister_Float:
				{
					for(int j=0;j<operands->num_values[i];j++) {
						memcpy(&ShaderASMState.opcodeBuffer[ShaderASMState.opcodeWriteIDX], &operands->values[i][j].f_val, sizeof(float));
						ShaderASMState.opcodeWriteIDX += sizeof(float);
					}
					break;
				}
				case EShaderRegister_UInt:
				{
					for(int j=0;j<operands->num_values[i];j++) {
						memcpy(&ShaderASMState.opcodeBuffer[ShaderASMState.opcodeWriteIDX], &operands->values[i][j].uintval, sizeof(uint32_t));
						ShaderASMState.opcodeWriteIDX += sizeof(uint32_t);
					}
					break;
				}
			}
		} else {
			ShaderASMState.opcodeBuffer[ShaderASMState.opcodeWriteIDX++] = (uint8_t)operands->indexes[i];
			ShaderASMState.opcodeBuffer[ShaderASMState.opcodeWriteIDX++] = (uint8_t)operands->accessors[i];
		}
	}
	
}
void parse_operand(char *string, EOperandInfo *operand, int index, EShaderInstruction instruction);
DefineSearchFunction(EPreProcessorType, find_preprocessor_token_by_name, PreProcessorOptions, name, type)
DefineSearchFunction(EMaterialDataType, find_material_accessor_by_name, MaterialAccessors, name, type)
DefineSearchFunction(ETextureSampleType, find_texture_sampler_by_name, SamplerMap, name, type)
DefineSearchFunction(EShaderRegister, find_register_by_name, RegisterMap, name, reg)
DefineSearchFunction(EShaderInstruction, find_instruction_by_name, InstructionMap, name, instruction)

uint32_t find_accessor_by_char(char ch) {
	for(int i=0;i<sizeof(VertexAccessorMap)/sizeof(VertexAccessorMap[0]);i++) {
		if(VertexAccessorMap[i].ch == toupper(ch)) {
			return VertexAccessorMap[i].flag;
		}
	}
	return 0;
}

void find_nth(char *p, int n, char *buff, int bufflen) {
	int x=n;
	char *c;
	char *tmpbuff=(char *)malloc(bufflen);
	if(tmpbuff == NULL) return;
	memcpy(tmpbuff,p,bufflen);
	int distance=0;
	memset(buff,0,bufflen);
	c = strtok(p," ");
	while(c != NULL) {
		x--;
		if(x<0) { 
			strncpy(buff,c,strlen(c)%bufflen);
			break;
		}
		c = strtok(NULL," ");
	}
	memcpy(p,tmpbuff,bufflen);
	free(tmpbuff);
}

void process_macro(const char *line) {
	const char *p = strchr(line, ' ');
	char inst[32];
	char param[32];
	memset(&inst,0,sizeof(inst));
	memset(&param,0,sizeof(param));
	int end = strlen(line);
	if(p) {
		end = p - line;
	}
	strncpy(inst, line, end);
	int uv_mode;
	ETextureSampleType sample_mode;
	EPreProcessorType token = find_preprocessor_token_by_name(&inst[1]);
	switch(token) {
		case EPreProcessor_UVLength:
			find_nth((char *)line, 1, param, sizeof(param));
			uv_mode = atoi(param);
			ShaderASMState.UVMode = uv_mode;
			//printf("Setting UV Mode to: %d\n",uv_mode);
			break;
		case EPreProcessor_TexSamplerType:
			find_nth((char *)line, 1, param, sizeof(param));
			uv_mode = atoi(param);
			memset(&param, 0, sizeof(param));
			find_nth((char *)line, 2, param, sizeof(param));
			//printf("Selected Tex: %d %s\n",uv_mode, line);
			sample_mode = find_texture_sampler_by_name(param);
			ShaderASMState.tex_types[uv_mode] = sample_mode;
			break;
		case EPreProcessor_Marker:
			break;
	}
}


void process_instruction(const char *line) {
	const char *p = strchr(line, ' ');
	char inst[32];
	char param[32];
	memset(&inst,0,sizeof(inst));
	memset(&param,0,sizeof(param));
	int end = strlen(line);
	if(p) {
		end = p - line;
	}
	strncpy(inst, line, end);
	EShaderInstruction instruction =  find_instruction_by_name(inst);
	EOperandInfo oper;
	memset(&oper,0,sizeof(oper));

	printf("Inst: %s\n", inst);

	char operandtxt[128];
	int idx = 1;

	EOperandInfo operand;
	memset(&operand,0,sizeof(operand));
	while(true) {
		memset(&operandtxt,0,sizeof(operand));
		find_nth((char *)line, idx++, operandtxt, sizeof(operand));
		if(strlen(operandtxt) < 1 || idx-1 > 3) break;
		parse_operand(operandtxt, &operand, idx-2, instruction);
		printf("oper: %s\n", operandtxt);
	}
	write_instruction(instruction, &operand);
	
}

void process_line(const char *line) {
	int len = strlen(line);
	if(line[0] == '#') {
		process_macro(line);
	} else {
		process_instruction(line);
	}
}
void process_marker(char *line, int instruction_number) {
	JumpTable[ShaderASMState.jump_index].index = instruction_number;
	char *p = strrchr(line, '#');
	if(p != NULL) {
		strcpy(JumpTable[ShaderASMState.jump_index].name, p+1);
		printf("%s is marker line\n",p+1);
	}
}
void compile_shader(FILE *fd) {
	char line[128];
	memset(&line,0,sizeof(line));
	int i =0;
	char c;
	while(c = fgetc(fd)) {
		if(c == EOF) {
			line[i] = 0;
			if(line[0] != '#')
				process_line(line); 
			//printf("Last line: %s\n",line);
			break;
		} else if(c == '\t' || c=='\r') {
		} else if(c == '\n') {
			line[i] = 0;
			i = 0;
			if(line[0] != '#')
				process_line(line);
			//printf("Parse line: %s\n",line);
		}else {
			line[i++] = c;
		}
	}
}
void preprocess_shader(FILE *fd) {
	char line[128];
	memset(&line,0,sizeof(line));
	int i =0;
	char c;
	int inst_idx = 0;
	while(c = fgetc(fd)) {
		if(c == EOF) {
			line[i] = 0;
			if(line[0] == '#') {
				process_line(line);
			} else if(strchr(line, '#') != NULL) {
				printf("process line is: %d\n",inst_idx);
				process_marker(line, inst_idx);
				inst_idx++;
			} else {
				inst_idx++;
			}
			//printf("Last line: %s\n",line);
			break;
		} else if(c == '\t' || c=='\r') {
		} else if(c == '\n') {
			line[i] = 0;
			i = 0;
			if(line[0] == '#') {
				process_line(line);
			} else if(strchr(line, '#') != NULL) {
				printf("process line is: %d\n",inst_idx);
				process_marker(line, inst_idx);
				
				inst_idx++;
			} else {
				inst_idx++;
			}
			//printf("Parse line: %s\n",line);
		}else {
			line[i++] = c;
		}
	}
}

char *find_first_nonalpha(char *name) {
	char *p = name;
	int len = strlen(name);
	for(int i=0;i<len;i++) {
		if(!isalpha(*p)) {
			return p;
		}
		p++;
	}
	return NULL;
}
uint32_t parse_accessors(const char *string) {
	int len =strlen(string);
	uint32_t flags = 0;
	for(int i=0;i<len;i++) {
		flags |= find_accessor_by_char(string[i]);
	}
	return flags;
}
void parse_literal(EOperandInfo *operand, int index, char *string) {
 char * pch;
  pch = strtok (string,",");
  int i = 0;
  while (pch != NULL)
  {
	bool is_float = strchr(pch, '.');
	if(is_float) {
		operand->registers[index] = EShaderRegister_Float;
		operand->values[index][i++].f_val = atof(pch);
	} else {
		operand->registers[index] = EShaderRegister_UInt;
		operand->values[index][i++].uintval = atoi(pch);
	}
    pch = strtok (NULL, ",");
  }
  operand->num_values[index] = i;
}
void parse_operand(char *string, EOperandInfo *operand, int index, EShaderInstruction instruction) {
	int operidx = 0;
	char original_string[128];
	memset(&original_string,0, sizeof(original_string));
	strcpy(original_string, string);
	char *num = find_first_nonalpha(string);
	char reg[32];
	memset(&reg,0,sizeof(reg));
	if(num != NULL) { //"vec1"
		char *accessor = strchr(num, '.');
		if(accessor != NULL) { //"vec0.rgba"
			*(accessor) = 0;
			operidx = atoi(num);
			accessor++;
			char *non_alpha = find_first_nonalpha(string);
			if(non_alpha) {
				*non_alpha = 0;
			}
			EShaderRegister reg = find_register_by_name(string);
			operand->registers[index] = reg;
			operand->accessors[index] = parse_accessors(accessor);
			operand->indexes[index] = operidx;
		} else { //"vec0"
			int regnum = atoi(num);
			operand->indexes[index] = regnum;
			operand->accessors[index] = 0;//EVectorFlags_Red|EVectorFlags_Blue|EVectorFlags_Green|EVectorFlags_Alpha;
			*num = 0;
			EShaderRegister reg = find_register_by_name(string);
			operand->registers[index] = reg;
		}
	}
	operand->registers[index] = find_register_by_name(string);
	if(instruction >= EShaderInstruction_BranchBegin && instruction <= EShaderInstruction_BranchEnd) {
		operand->jump_index = find_jump_index_by_name(original_string);
		//printf("read branch\n");
		return;
	}
	if(operand->registers[index] == -1) { //literal value
		if(original_string[0] == '#') { //skip marker definition
		} else {
			parse_literal(operand, index, original_string);
			//printf("some literal: %s\n",original_string);
		}		
	}
}

bool isbranch(EShaderInstruction instruction) {
	return instruction >= EShaderInstruction_BranchBegin && instruction <= EShaderInstruction_BranchEnd;
}
bool isliteral(EShaderRegister reg) {
	return reg >= EShaderRegister_BeginLiteral && reg <= EShaderRegister_EndLiteral;
}

int main() {
	FILE *fd = fopen("test.txt","rb");
	if(!fd) {
		return -1;
	}
	memset(&ShaderASMState,0,sizeof(ShaderASMState));
	preprocess_shader(fd);
	fseek(fd,0,SEEK_SET);
	compile_shader(fd);


	FILE *shaderout = fopen("shader.bin","wb");
	
	fwrite(ShaderASMState.opcodeBuffer,ShaderASMState.opcodeWriteIDX,1,shaderout);
	fclose(shaderout);

	ShaderLib::GLSLBuilder *builder = new ShaderLib::GLSLBuilder();
	char out[2048];
	int outlen;
	builder->build((char *)&out,&outlen,ShaderASMState.opcodeBuffer,ShaderASMState.opcodeWriteIDX);
	return 0;
}