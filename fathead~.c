#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <strings.h>
#include <string.h>

#include "m_pd.h"
#define atom_getsymarg atom_getsymbolarg

static t_class *fathead_class;

#ifndef TWOPI
#define TWOPI 6.2831853072
#endif

typedef struct _fathead
{
  t_object x_obj;
  long buflen ; // length of buffer

  float *fhbufferL;
  float *fhbufferR;

  float dummy; // used to appease pd's insistence of sending floats to left inlet

} t_fathead;

void *fathead_new(); /// removed floatarg. is this right? 
t_int *fathead_perform(t_int *w);
void fathead_dsp(t_fathead *x, t_signal **sp, short *count);
void fathead_dsp_free(t_fathead *x);


void fathead_tilde_setup(void){
  fathead_class = class_new(gensym("fathead~"), (t_newmethod)fathead_new, 
			    (t_method)fathead_dsp_free ,sizeof(t_fathead), 0,0,0);
  CLASS_MAINSIGNALIN(fathead_class, t_fathead , dummy); 
  class_addmethod(fathead_class,(t_method)fathead_dsp,gensym("dsp"),0);
}

void fathead_dsp_free(t_fathead *x)
{
  free(x->fhbufferL);
  free(x->fhbufferR);
}

void *fathead_new(void)
{
  t_fathead *x = (t_fathead *)pd_new(fathead_class);

  // set up two signal inlets and two signal outlets

  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));

  x->buflen = sys_getsr();

  x->fhbufferL = (float *) calloc( x->buflen, sizeof(float) );
  x->fhbufferR = (float *) calloc( x->buflen, sizeof(float) );
    
  return (x);
}

t_int *fathead_perform(t_int *w)
{
    int Lindex = 0;
    int Rindex = 0;
	
	t_fathead *x = (t_fathead *) (w[1]);
	t_float *inL = (t_float *)(w[2]);
	t_float *inR= (t_float *)(w[3]);
	t_float *out1 = (t_float *)(w[4]);
	t_float *out2 = (t_float *)(w[5]);
	int n = (int)(w[6]);
	
	long buflen = x->buflen;
	
	while (n--) { 

		
		*out1++ =  x->fhbufferL[Lindex]; 
		*out2++ =  x->fhbufferR[Rindex]; 
	}

	return (w+7);
}		

void fathead_dsp(t_fathead *x, t_signal **sp, short *count)
{
  dsp_add(fathead_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}

