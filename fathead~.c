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
  long buflen ; // length of buffer should be one second

  float *fhbufferL;
  float *fhbufferR;

  float * windowfunction;

  int lDelayWriteIndex;
  int rDelayWriteIndex;

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
  free(x->windowfunction);
}

void *fathead_new(void)
{
  t_fathead *x = (t_fathead *)pd_new(fathead_class);

  // set up two signal inlets and two signal outlets
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));

  x->buflen = sys_getsr(); // set buflen to sample rate to ensure one second of sound

  x->fhbufferL = (float *) calloc( x->buflen, sizeof(float) );
  x->fhbufferR = (float *) calloc( x->buflen, sizeof(float) );
  x->windowfunction = (float *) calloc( x->buflen, sizeof(float) );
  
  for ( int i = 0 ; i < x->buflen ; i++)
	*x->windowfunction = 1 - pow( (i - (((float)x->buflen-1)/2))/(((float)x->buflen + 1) / 2),2);
	
  
  return (x);
}

t_int *fathead_perform(t_int *w)
{

    t_fathead *x = (t_fathead *) (w[1]);
    t_float *inL = (t_float *)(w[2]);
    t_float *inR= (t_float *)(w[3]);
    t_float *out1 = (t_float *)(w[4]);
    t_float *out2 = (t_float *)(w[5]);
    int n = (int)(w[6]);
	
    long buflen = x->buflen;

    int i = n; //??
    
    float lgain = 0;
    float rgain = 0 ;
    float lRMS = 0;
    float rRMS = 0;

    for ( i ; i > 0 ; i--)
    {
        lRMS += inL[i] * inL[i];        
        rRMS += inR[i] * inR[i];
    }	
    lRMS = sqrt(lRMS / (float)buflen);
    rRMS = sqrt(rRMS / (float)buflen);

    if (lRMS == 0.0)
        lgain = 0.0;       
    else
        lgain = lRMS / rRMS; 

    if (rRMS == 0.0)
        rgain = 0.0; 
    else
        rgain = rRMS / lRMS;

    x->lDelayWriteIndex = (x->lDelayWriteIndex + ((int)(lgain * x->buflen)) )% x->buflen;
    x->rDelayWriteIndex = (x->rDelayWriteIndex + ((int)(rgain * x->buflen)) )% x->buflen;

	while (n--) { 
        x->fhbufferL[x->lDelayWriteIndex] = *inL++ * lgain * x->windowfunction[n];
        x->fhbufferR[x->rDelayWriteIndex] = *inR++ * rgain * x->windowfunction[n];
		
		*out1++ =  x->fhbufferL[x->lDelayWriteIndex]; 
		*out2++ =  x->fhbufferR[x->rDelayWriteIndex]; 

        x->lDelayWriteIndex = x->lDelayWriteIndex % x->buflen;
        x->rDelayWriteIndex = x->rDelayWriteIndex % x->buflen;
	}

	return (w+7);
}		

void fathead_dsp(t_fathead *x, t_signal **sp, short *count)
{
  dsp_add(fathead_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}
