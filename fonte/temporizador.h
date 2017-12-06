#ifndef _TEMPORIZADOR_H_ 
#define _TEMPORIZADOR_H_ 

void mysettimer(int milisegundos);

void timer_handler(int signum);

void mysethandler(void);

#endif /* _TEMPORIZADOR_H_ */