#ifndef _t_scope_h
#define _t_scope_h

#ifdef __cplusplus
extern "C" {
#endif 

void  scope_init();
void* frame_push();
void  frame_pop(void**);
void* t_scope_new(int size);
void  scope_dump();

#define __t_scope(n) void *t_scope_frame_##n __attribute__ ((unused,cleanup(frame_pop))) = frame_push();
#define _t_scope(n) __t_scope(n)
#define t_scope _t_scope(__LINE__)

#ifdef __cplusplus
} // extern "C"
#endif 

#endif // _t_scope_h

