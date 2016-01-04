#include <stdio.h>
#include <memory.h>
#include <string.h>
#include "GLSLBuilder.h"
using namespace ShaderASM;
namespace ShaderLib {
		NameMap GLSLBuilder::m_texname_map[] = {
			{0, "texture2D"},
			{1, "textureRect"},
			{2, "textureCube"},
		};
		GLSLBuilder::GLSLBuilder() {
			memset(&m_uv_mode,0,sizeof(m_uv_mode));
		}
		GLSLBuilder::~GLSLBuilder() {
		}
		void GLSLBuilder::build(char *out, int *out_len, const char *src, int src_len) {
			int i = 0;
			InstructionPack pack;
			
			for(int i=0;i<src_len;i++) {
				memset(&pack,0,sizeof(pack));
				pack.instuction = (EShaderInstruction)src[i];
				
				if(isbranch(pack.instuction)) {
					memcpy(&pack.jump, &src[i+1], sizeof(uint16_t));
					i += sizeof(uint16_t);
				} else {
					memcpy(&pack.modifiers, &src[i+1], sizeof(InstructionModifiers));
					i += sizeof(InstructionModifiers);
					uint8_t operand_count = src[++i];
					for(int j=0;j<operand_count;j++) {
						pack.reg[j] = src[++i];
						if(isliteral((EShaderRegister)pack.reg[j])) {
							pack.num_values[j] = (uint8_t)src[++i];
							for(int k=0;k<pack.num_values[j];k++) {
								switch((EShaderRegister)pack.reg[j]) {
									case EShaderRegister_Float:
										memcpy(&pack.values[j][k].f_val,&src[++i],sizeof(float));
										i += sizeof(float)-1;
										//printf("Read literal: %f\n", pack.values[j][k].f_val);
										break;
									case EShaderRegister_UInt:
										memcpy(&pack.values[j][k].uintval,&src[++i],sizeof(uint32_t));
										//printf("Read literal: %d %d\n", pack.values[j][k].uintval, sizeof(uint32_t));
										i += sizeof(uint32_t)-1;
										break;

								}
							}
						} else {
							pack.register_index[j] = src[++i];
							pack.accessor[j] = src[++i];
						}
					}
					//printf("operand count: %d\n",operand_count);
				}
				char glsl_line[128];
				int line_len = 0;
				get_instruction((char *)&glsl_line, &line_len, &pack);
				
				strcat(out, glsl_line);
				strcat(out, "\n");
			}
			*out_len = strlen(out);
		}
		void GLSLBuilder::get_instruction(char *out, int *len, InstructionPack *data) {
			char operand[4][32];
			int operand_len[4];
			memset(&operand,0,sizeof(operand));
			*out = 0;
			for(int i=0;i<4;i++) {
				get_operand(data, i, (char *)&operand[i], &operand_len[i]);
			}
			switch(data->instuction) {
				case EShaderInstruction_Mov: { //mov(assignment) instruction
					*len = sprintf(out, "%s = %s;",operand[0],operand[1]);
					break;
				}
				case EShaderInstruction_SampleTex: {
					*len = sprintf(out, "%s = %s(%s, %s);",operand[0],m_texname_map[m_uv_mode[data->register_index[0]]].name,operand[1], operand[2]);
					break;
				}
				case EShaderInstruction_Lerp: {
					*len = sprintf(out, "%s = mix(%s, %s, %s);",operand[0],operand[1], operand[2], operand[3]);
					break;
				}
				case EShaderInstruction_MulCpy: {
					*len = sprintf(out, "%s *= %s;",operand[0],operand[1]);
					break;
				}
				case EShaderInstruction_Mul: {
					*len = sprintf(out, "%s = %s * %s;",operand[0],operand[1],operand[2]);
					break;
				}
				case EShaderInstruction_AddCpy: {
					*len = sprintf(out, "%s += %s;",operand[0],operand[1]);
					break;
				}
				case EShaderInstruction_SubCpy: {
					*len = sprintf(out, "%s -= %s;",operand[0],operand[1]);
					break;
				}
				case EShaderInstruction_DivCpy: {
					*len = sprintf(out, "%s /= %s;",operand[0],operand[1]);
					break;
				}
			}
			char tempbuf[128];
			memset(&tempbuf,0,sizeof(tempbuf));
			if(data->modifiers.multiplier != 0.0) {
				sprintf(tempbuf, "\n%s *= %.02f;", operand[0],data->modifiers.multiplier);
				strcat(out, tempbuf);
			}
			if(data->modifiers.clamp_min != 0.0 || data->modifiers.clamp_max != 0.0) {
				sprintf(tempbuf, "\n%s = clamp(%s,%.02f, %.02f);", operand[0],operand[0],data->modifiers.clamp_min,data->modifiers.clamp_max);
				strcat(out, tempbuf);
			}

			*len = strlen(out);
		}
		void GLSLBuilder::get_operand(InstructionPack *data, int index, char *out, int *out_len) {
			char temp[256];
			char valbuf[32];
			int len = 0;
			int tlen = 0;
			memset(&valbuf,0,sizeof(valbuf));
			memset(&temp,0,sizeof(temp));
			switch(data->reg[index]) {
				case EShaderRegister_OutColour:  {
					len += sprintf(temp, "out_Colour");
					if(data->accessor[index] & EVectorFlags_Negate) {
						strcat(out, "-");
						len++;
					}
					strcat(out, temp);
					break;
				}
				case EShaderRegister_Vector:
				{
					len += sprintf(temp, "vecs[%d]",data->register_index[index]);
					if(data->accessor[index] & EVectorFlags_Negate) {
						strcat(out, "-");
						len++;
					}
					strcat(out, temp);
					break;
				}
				case EShaderRegister_Mat:
				{
					len += sprintf(temp, "Material");
					if(data->accessor[index] & EVectorFlags_Negate) {
						strcat(out, "-");
						len++;
					}
					strcat(out, temp);
					break;
				}
				case EShaderRegister_Const:
				{
					len += sprintf(temp, "constants[%d]",data->register_index[index]);
					if(data->accessor[index] & EVectorFlags_Negate) {
						strcat(out, "-");
						len++;
					}
					strcat(out, temp);
					break;
				}
				case EShaderRegister_Col: {
					len += sprintf(temp, "ex_Color",data->register_index[index]);
					if(data->accessor[index] & EVectorFlags_Negate) {
						strcat(out, "-");
						len++;
					}
					strcat(out, temp);
					break;
				}
				case EShaderRegister_Texture: {
					len += sprintf(temp, "texture%d",data->register_index[index]);
					if(data->accessor[index] & EVectorFlags_Negate) {
						strcat(out, "-");
						len++;
					}
					strcat(out, temp);
					break;
				}
				case EShaderRegister_UVW: {
					len += sprintf(temp, "ex_UV%d",data->register_index[index]);
					if(data->accessor[index] & EVectorFlags_Negate) {
						strcat(out, "-");
						len++;
					}
					strcat(out, temp);
					break;
				}
				//immediates
				case EShaderRegister_UInt:
					len += sprintf(temp, "ivec%d(",data->num_values[index]);
					strcpy(out, temp);
					
					for(int i=0;i<data->num_values[index];i++) {
						sprintf(valbuf, "%d,",data->values[index][i].uintval);
						strcat(temp, valbuf);
					}
					tlen = strlen(temp);
					temp[tlen-1] = ')';
					temp[tlen] = 0;
					out[0] = 0;
					if(data->accessor[index] & EVectorFlags_Negate) {
						strcat(out, "-");
						len++;
					}
					strcat(out, temp);
					break;
				case EShaderRegister_Float:
					len += sprintf(temp, "vec%d(",data->num_values[index]);
					strcpy(out, temp);
					
					for(int i=0;i<data->num_values[index];i++) {
						sprintf(valbuf, "%f,",data->values[index][i].f_val);
						strcat(temp, valbuf);
					}
					tlen = strlen(temp);
					temp[tlen-1] = ')';
					temp[tlen] = 0;
					out[0] = 0;
					if(data->accessor[index] & EVectorFlags_Negate) {
						strcat(out, "-");
						len++;
					}
					strcat(out, temp);
					break;
			}
			if(data->accessor[index] != 0) {
				
				if(data->accessor[index] & EMaterialDataType_TexCount) {
					out[len++] = '.';
					strcat(out, "tex_count");
					len += 9;
				} else if(data->accessor[index] & EMaterialDataType_Diffuse) {
					out[len++] = '.';
					len += sprintf(temp, "diffuse[%d]",data->register_index[index]);
					strcat(out, temp);
				}
				if(data->accessor[index] & EVectorFlags_Red|EVectorFlags_Blue|EVectorFlags_Green|EVectorFlags_Alpha)
					out[len++] = '.';
						
				if(data->accessor[index] & EVectorFlags_Red) {
					out[len++] = 'r';
				}
				if(data->accessor[index] & EVectorFlags_Green) {
					out[len++] = 'g';
				}
				if(data->accessor[index] & EVectorFlags_Blue) {
					out[len++] = 'b';
				}
				if(data->accessor[index] & EVectorFlags_Alpha) {
					out[len++] = 'a';
				}
			} else if(data->reg[index] == EShaderRegister_Mat) {

			}
			*out_len = strlen(out);
			out[*out_len] = 0;
		}
}