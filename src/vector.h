#define vec(t) t *

#define vec_setcap(vec,cap) ((size_t*)(vec))[-1] = cap
#define vec_setlen(vec,len) ((size_t*)(vec))[-2] = len

#define vec_cap(vec) ((vec) ? ((size_t*)(vec))[-1] : 0)
#define vec_len(vec) ((vec) ? ((size_t*)(vec))[-2] : 0)

#define vec_resize(vec,cap) \
	do { \
		size_t Vsize = (cap)*sizeof(*(vec)) + sizeof(size_t)*2; \
		size_t *Vnew; \
		if (!(vec)) { \
			Vnew = xmalloc(Vsize); \
			Vnew[0] = 0; \
		} else { \
			Vnew = xrealloc(&((size_t*)(vec))[-2], Vsize); \
		} \
		Vnew[1] = cap; \
		vec = (void*)&Vnew[2]; \
	} while (0) \

#define vec_push(vec,val) \
	do { \
		if (!(vec)) \
			vec_resize(vec, 16); \
		size_t Vcap = vec_cap(vec); \
		size_t Vlen = vec_len(vec); \
		if (Vcap <= Vlen) \
			vec_resize(vec, Vcap + Vcap/2); \
		(vec)[Vlen] = val; \
		vec_setlen(vec, Vlen+1); \
	} while (0) \

#define vec_erase(vec,idx) \
	do { \
		if (vec) { \
			size_t Vnewlen = vec_len(vec) - 1; \
			vec_setlen(vec, Vnewlen); \
			memmove(vec+idx, vec+idx+1, (Vnewlen-idx) * sizeof(*(vec))); \
		} \
	} while (0) \

#define vec_pop(vec) vec_setlen(vec, vec_len(vec) - 1)

#define vec_free(vec) free((vec) ? &((size_t*)(vec))[-2] : NULL)

#define vec_end(vec) ((vec) ? &((vec)[vec_len(vec)]) : NULL)
