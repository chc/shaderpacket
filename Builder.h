#ifndef _BUILDER_H
#define _BUILDER_H
#include "shaderasm.h"
namespace ShaderLib {
	class Builder {
		public:
			Builder() { };
			virtual ~Builder();
			virtual void build(char *out, int *out_len, const char *src, int src_len) = 0;
	};
};
#endif //_BUILDER_H