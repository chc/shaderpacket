#include <stdio.h>
#include <memory.h>
#include <string.h>
#include "GLSLBuilder.h"
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
				printf("%s\n",glsl_line);
			}
		}
		void GLSLBuilder::get_instruction(char *out, int *len, InstructionPack *data) {
			char operand[3][32];
			int operand_len[3];
			memset(&operand,0,sizeof(operand));
			switch(data->instuction) {
				case EShaderInstruction_Mov: { //mov(assignment) instruction
					int idx = 0;
					get_operand(data, idx, (char *)&operand[idx], &operand_len[idx]);
					idx++;
					get_operand(data, idx, (char *)&operand[idx], &operand_len[idx]);
					*len = sprintf(out, "%s = %s;",operand[0],operand[1]);

					break;
				}
				case EShaderInstruction_SampleTex: {
					int idx = 0;
					get_operand(data, idx, (char *)&operand[idx], &operand_len[idx]);
					idx++;
					get_operand(data, idx, (char *)&operand[idx], &operand_len[idx]);
					idx++;
					get_operand(data, idx, (char *)&operand[idx], &operand_len[idx]);
					*len = sprintf(out, "%s = %s(%s, %s);",operand[0],m_texname_map[m_uv_mode[data->register_index[idx]]].name,operand[1], operand[2]);
					break;
				}
				case EShaderInstruction_MulCpy: {
					int idx = 0;
					get_operand(data, idx, (char *)&operand[idx], &operand_len[idx]);
					idx++;
					get_operand(data, idx, (char *)&operand[idx], &operand_len[idx]);
					*len = sprintf(out, "%s *= %s;",operand[0],operand[1]);
					break;
				}
				case EShaderInstruction_AddCpy: {
					int idx = 0;
					get_operand(data, idx, (char *)&operand[idx], &operand_len[idx]);
					idx++;
					get_operand(data, idx, (char *)&operand[idx], &operand_len[idx]);
					*len = sprintf(out, "%s += %s;",operand[0],operand[1]);
					break;
				}
				case EShaderInstruction_SubCpy: {
					int idx = 0;
					get_operand(data, idx, (char *)&operand[idx], &operand_len[idx]);
					idx++;
					get_operand(data, idx, (char *)&operand[idx], &operand_len[idx]);
					*len = sprintf(out, "%s -= %s;",operand[0],operand[1]);
					break;
				}
				case EShaderInstruction_DivCpy: {
					int idx = 0;
					get_operand(data, idx, (char *)&operand[idx], &operand_len[idx]);
					idx++;
					get_operand(data, idx, (char *)&operand[idx], &operand_len[idx]);
					*len = sprintf(out, "%s /= %s;",operand[0],operand[1]);
					break;
				}
			}
		}
		void GLSLBuilder::get_operand(InstructionPack *data, int index, char *out, int *out_len) {
			char temp[256];
			char valbuf[32];
			int len = 0;
			int tlen = 0;
			memset(&valbuf,0,sizeof(valbuf));
			memset(&temp,0,sizeof(temp));
			switch(data->reg[index]) {
				case EShaderRegister_Vector:
				{
					len += sprintf(temp, "vec%d",data->register_index[index]);
					strcpy(out, temp);						
					break;
				}
				case EShaderRegister_Texture: {
					len += sprintf(temp, "tex%d",data->register_index[index]);
					strcpy(out, temp);						
					break;
				}
				case EShaderRegister_UVW: {
					len += sprintf(temp, "texpos%d",data->register_index[index]);
					strcpy(out, temp);						
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
					strcpy(out, temp);
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
					strcpy(out, temp);
					break;
			}
			if(data->accessor[index] != 0) {
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
			}
			*out_len = strlen(out);
			out[*out_len] = 0;
		}
}